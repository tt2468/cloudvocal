#include "deepgram-provider.h"
#include <nlohmann/json.hpp>

#include "language-codes/language-codes.h"

using json = nlohmann::json;

namespace http = beast::http;

DeepgramProvider::DeepgramProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
	: CloudProvider(callback, gf_),
	  ioc(),
	  ssl_ctx(ssl::context::tlsv12_client),
	  resolver(ioc),
	  ws(ioc, ssl_ctx)
{
	needs_results_thread = true; // We need a separate thread for reading results
}

bool DeepgramProvider::init()
{
	try {
		// Setup SSL context
		ssl_ctx.set_verify_mode(ssl::verify_peer);
		ssl_ctx.set_default_verify_paths();

		// Resolve the Deepgram endpoint
		auto const results = resolver.resolve("api.deepgram.com", "443");

		// Connect to Deepgram
		net::connect(get_lowest_layer(ws), results);

		// Set SNI hostname (required for TLS)
		if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(),
					      "api.deepgram.com")) {
			throw beast::system_error(
				beast::error_code(static_cast<int>(::ERR_get_error()),
						  net::error::get_ssl_category()),
				"Failed to set SNI hostname");
		}

		// Perform SSL handshake
		ws.next_layer().handshake(ssl::stream_base::client);

		// Set up WebSocket handshake with API key
		ws.set_option(
			websocket::stream_base::decorator([this](websocket::request_type &req) {
				req.set(http::field::sec_websocket_protocol,
					"token, " + std::string(gf->cloud_provider_api_key));
			}));

		std::string query = std::string("/v1/listen?encoding=linear16&sample_rate=16000") +
				    "&language=" + language_codes_from_underscore[gf->language];
		// Perform WebSocket handshake
		ws.handshake("api.deepgram.com", query);

		obs_log(LOG_INFO, "Connected to Deepgram WebSocket successfully");
		return true;
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error initializing Deepgram connection: %s", e.what());
		return false;
	}
}

void DeepgramProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	if (audio_buffer.empty())
		return;

	try {
		// Convert float audio to int16_t (linear16 format)
		std::vector<int16_t> pcm_data;
		pcm_data.reserve(audio_buffer.size());

		for (float sample : audio_buffer) {
			// Clamp and convert to int16
			float clamped = std::max(-1.0f, std::min(1.0f, sample));
			pcm_data.push_back(static_cast<int16_t>(clamped * 32767.0f));
		}

		// Send binary message
		ws.write(net::buffer(pcm_data.data(), pcm_data.size() * sizeof(int16_t)));

	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error sending audio to Deepgram: %s", e.what());
		running = false;
	}
}

void DeepgramProvider::readResultsFromTranscription()
{
	try {
		// Read message into buffer
		beast::flat_buffer buffer;
		ws.read(buffer);

		// Convert to string and parse JSON
		std::string msg = beast::buffers_to_string(buffer.data());
		json result = json::parse(msg);

		// Check if this is a transcription result
		if (result["type"] == "Results" && !result["channel"]["alternatives"].empty()) {
			DetectionResultWithText detection_result;

			// Fill the detection result structure
			detection_result.text = result["channel"]["alternatives"][0]["transcript"];
			detection_result.result = result["is_final"] ? DETECTION_RESULT_SPEECH
								     : DETECTION_RESULT_PARTIAL;

			// If there are words with timestamps
			if (!result["channel"]["alternatives"][0]["words"].empty()) {
				auto &words = result["channel"]["alternatives"][0]["words"];
				detection_result.start_timestamp_ms = words[0]["start"];
				detection_result.end_timestamp_ms = words[words.size() - 1]["end"];
			}

			// Send result through callback
			transcription_callback(detection_result);
		}
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error reading from Deepgram: %s", e.what());
	}
}

void DeepgramProvider::shutdown()
{
	try {
		// Send close message
		ws.write(net::buffer(R"({"type":"CloseStream"})"));

		// Close WebSocket connection
		ws.close(websocket::close_code::normal);

		obs_log(LOG_INFO, "Deepgram connection closed successfully");
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error during Deepgram shutdown: %s", e.what());
	}
}
