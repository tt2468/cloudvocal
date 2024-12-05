#pragma once

#include "cloud-providers/cloud-provider.h"
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <deque>
#include <string>

// Define websocketpp client types
using Client = websocketpp::client<websocketpp::config::asio_client>;
using MessagePtr = websocketpp::config::asio_client::message_type::ptr;
using ConnectionHdl = websocketpp::connection_hdl;

class RevAIProvider : public CloudProvider {
public:
	RevAIProvider(TranscriptionCallback callback, cloudvocal_data *gf);
	virtual ~RevAIProvider() override;

	virtual bool init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;
	virtual void readResultsFromTranscription() override;
	virtual void shutdown() override;

private:
	// WebSocket handlers
	void onOpen(ConnectionHdl hdl);
	void onMessage(ConnectionHdl hdl, MessagePtr msg);
	void onClose(ConnectionHdl hdl);

	// Utility functions
	std::vector<int16_t> convertFloatToS16LE(const std::deque<float> &audio_buffer);

	// Member variables
	Client ws_client;
	ConnectionHdl connection;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> ws_thread;
	bool is_connected;
	std::string job_id;
};
