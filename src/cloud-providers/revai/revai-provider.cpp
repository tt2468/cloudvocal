#include "revai-provider.h"

#include <algorithm>
#include <string>
#include <functional>
#include <queue>
#include <memory>
#include <vector>
#include <iostream>

#include "nlohmann/json.hpp"
#include "language-codes/language-codes.h"

namespace http = beast::http;
using json = nlohmann::json;

// Response structure definitions
struct TranscriptElement {
	std::string type;  // "text" or "punctuation"
	std::string value; // The actual text or punctuation mark
	double ts;         // Start time in seconds
	double end_ts;     // End time in seconds
	double confidence; // Confidence score
};

struct TranscriptResponse {
	std::string type;                        // "connected", "partial", or "final"
	std::string id;                          // Job ID (only in connected message)
	double ts;                               // Start time in seconds
	double end_ts;                           // End time in seconds
	std::vector<TranscriptElement> elements; // Elements in the transcript
};

// JSON conversion functions
void from_json(const json &j, TranscriptElement &e)
{
	j.at("type").get_to(e.type);
	j.at("value").get_to(e.value);
	if (j.contains("ts"))
		j.at("ts").get_to(e.ts);
	if (j.contains("end_ts"))
		j.at("end_ts").get_to(e.end_ts);
	if (j.contains("confidence"))
		j.at("confidence").get_to(e.confidence);
}

void from_json(const json &j, TranscriptResponse &r)
{
	j.at("type").get_to(r.type);
	if (j.contains("id"))
		j.at("id").get_to(r.id);
	if (j.contains("ts"))
		j.at("ts").get_to(r.ts);
	if (j.contains("end_ts"))
		j.at("end_ts").get_to(r.end_ts);
	if (j.contains("elements"))
		j.at("elements").get_to(r.elements);
}

RevAIProvider::RevAIProvider(TranscriptionCallback callback, cloudvocal_data *gf)
	: CloudProvider(callback, gf),
	  is_connected(false),
	  ctx_(ssl::context::tlsv12_client),
	  ws_(ioc_, ctx_)
{
	is_connected = false;
}

bool RevAIProvider::init()
{
	// Initialize SSL context
	ctx_.set_verify_mode(ssl::verify_peer);
	ctx_.set_default_verify_paths();

	// These objects perform our I/O
	tcp::resolver resolver{ioc_};

	// Look up the domain name
	auto const results = resolver.resolve(host_, "443");

	// Make the connection on the IP address we get from a lookup
	auto ep = net::connect(get_lowest_layer(ws_), results);

	// Set SNI Hostname (many hosts need this to handshake successfully)
	if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
		throw beast::system_error(beast::error_code(static_cast<int>(::ERR_get_error()),
							    net::error::get_ssl_category()),
					  "Failed to set SNI Hostname");

	// Update the host string. This will provide the value of the
	// Host HTTP header during the WebSocket handshake.
	std::string host = host_ + ':' + std::to_string(ep.port());

	// Perform the SSL handshake
	ws_.next_layer().handshake(ssl::stream_base::client);

	// Perform the websocket handshake
	std::string query =
		target_ + "?access_token=" + this->gf->cloud_provider_api_key +
		"&content_type=audio/x-raw;layout=interleaved;rate=16000;format=S16LE;channels=1;" +
		"language=" + language_codes_from_underscore[gf->language];

	ws_.set_option(websocket::stream_base::decorator([&host](websocket::request_type &req) {
		req.set(http::field::host, host);
		req.set(http::field::user_agent, "RevAI-CPP-Client");
	}));

	ws_.handshake(host, query);
	return true;
}

void RevAIProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	// Convert audio buffer to S16LE
	std::vector<int16_t> converted = convertFloatToS16LE(audio_buffer);

	// Send audio buffer to Rev.ai
	ws_.write(net::buffer(converted));
}

// Receive and handle messages
void RevAIProvider::readResultsFromTranscription()
{
	beast::flat_buffer buffer;

	try {
		// Read a message
		ws_.read(buffer);

		// Handle the message
		std::string msg = beast::buffers_to_string(buffer.data());
		obs_log(LOG_INFO, "Received: %s", msg.c_str());

		auto j = json::parse(msg);
		TranscriptResponse response = j.get<TranscriptResponse>();

		DetectionResultWithText result;
		bool send_result = false;

		// Store job ID if this is a connected message
		if (response.type == "connected") {
			obs_log(LOG_INFO, "Connected to Rev AI. Job ID: %s", response.id.c_str());
		}
		// Handle partial transcripts
		else if (response.type == "partial") {
			result.text = "";
			result.result = DetectionResult::DETECTION_RESULT_PARTIAL;
			for (const auto &element : response.elements) {
				if (element.type == "text") {
					result.text += element.value + " ";
				} else {
					result.text += element.value;
				}
			}
			send_result = true;
		}
		// Handle final transcripts
		else if (response.type == "final") {
			result.text = "";
			result.result = DetectionResult::DETECTION_RESULT_SPEECH;
			for (const auto &element : response.elements) {
				if (element.type == "text") {
					result.text += element.value + " ";
				} else {
					result.text += element.value;
				}
			}
			send_result = true;
		} else {
			obs_log(LOG_WARNING, "Unknown message type: %s", response.type.c_str());
		}

		if (send_result) {
			result.language = language_codes_from_underscore[gf->language];
			result.start_timestamp_ms = (uint64_t)response.ts;
			result.end_timestamp_ms = (uint64_t)response.end_ts;
			this->transcription_callback(result);
		}

		buffer.consume(buffer.size());
	} catch (beast::system_error const &se) {
		// This indicates the connection was closed
		if (se.code() != websocket::error::closed) {
			obs_log(LOG_ERROR, "Error: %s", se.code().message().c_str());
		}
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error: %s", e.what());
	}
}

void RevAIProvider::shutdown()
{
	// Send EOS to signal end of stream
	ws_.write(net::buffer(std::string("EOS")));

	// Close the WebSocket connection
	ws_.close(websocket::close_code::normal);
}

std::vector<int16_t> RevAIProvider::convertFloatToS16LE(const std::deque<float> &audio_buffer)
{
	std::vector<int16_t> converted;
	converted.reserve(audio_buffer.size());

	for (float sample : audio_buffer) {
		// Clamp to [-1.0, 1.0] and convert to S16LE
		sample = std::fmaxf(-1.0f, std::fminf(1.0f, sample));
		converted.push_back(static_cast<int16_t>(sample * 32767.0f));
	}
	return converted;
}
