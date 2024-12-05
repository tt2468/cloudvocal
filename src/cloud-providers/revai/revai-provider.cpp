#include "revai-provider.h"

#include <algorithm>

RevAIProvider::RevAIProvider(TranscriptionCallback callback, cloudvocal_data *gf)
	: CloudProvider(callback, gf),
	  is_connected(false)
{
	needs_results_thread = false;

	// Configure websocketpp
	ws_client.clear_access_channels(websocketpp::log::alevel::all);
	ws_client.clear_error_channels(websocketpp::log::elevel::all);

	ws_client.init_asio();

	// Set up callbacks
	ws_client.set_message_handler(
		[this](ConnectionHdl hdl, MessagePtr msg) { this->onMessage(hdl, msg); });

	ws_client.set_open_handler([this](ConnectionHdl hdl) { this->onOpen(hdl); });

	ws_client.set_close_handler([this](ConnectionHdl hdl) { this->onClose(hdl); });
}

RevAIProvider::~RevAIProvider()
{
	shutdown();
}

bool RevAIProvider::init()
{
	websocketpp::lib::error_code ec;

	// Create connection
	Client::connection_ptr con =
		ws_client.get_connection("wss://api.rev.ai/speechtotext/v1/stream", ec);

	if (ec) {
		return false;
	}

	// Add custom headers
	con->append_header("Authorization", "Bearer " + std::string(gf->cloud_provider_api_key));
	con->append_header("Content-Type",
			   "audio/x-raw;layout=interleaved;rate=16000;format=S16LE;channels=1");

	// Establish connection
	ws_client.connect(con);

	// Run the ASIO io_service in a separate thread
	ws_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(
		[this] { ws_client.run(); });

	return true;
}

void RevAIProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	if (!is_connected) {
		return;
	}

	// Convert audio to S16LE format
	auto converted_audio = convertFloatToS16LE(audio_buffer);

	// Send binary audio data
	websocketpp::lib::error_code ec;
	ws_client.send(connection, converted_audio.data(), converted_audio.size() * sizeof(int16_t),
		       websocketpp::frame::opcode::binary, ec);
}

void RevAIProvider::readResultsFromTranscription()
{
	// Results are handled asynchronously in onMessage callback
}

void RevAIProvider::shutdown()
{
	if (is_connected) {
		// Send EOS message
		websocketpp::lib::error_code ec;
		ws_client.send(connection, "EOS", websocketpp::frame::opcode::text, ec);

		// Close connection
		ws_client.close(connection, websocketpp::close::status::normal,
				"Shutdown requested", ec);

		// Stop ASIO io_service
		ws_client.stop();

		if (ws_thread && ws_thread->joinable()) {
			ws_thread->join();
		}

		is_connected = false;
	}
}

void RevAIProvider::onOpen(ConnectionHdl hdl)
{
	connection = hdl;
	is_connected = true;
}

void RevAIProvider::onMessage(ConnectionHdl hdl, MessagePtr msg)
{
	if (msg->get_opcode() == websocketpp::frame::opcode::text) {
		// Handle text messages (like "connected" message and transcription results)
		std::string payload = msg->get_payload();

		// Parse JSON response
		// Note: You'll need to include a JSON library like nlohmann::json
		// and implement proper JSON parsing here

		// For the "connected" message, extract the job ID
		// For transcription results, call the callback function
		// callback(transcription_text);
	}
}

void RevAIProvider::onClose(ConnectionHdl hdl)
{
	is_connected = false;
	// Handle any cleanup or reconnection logic
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
