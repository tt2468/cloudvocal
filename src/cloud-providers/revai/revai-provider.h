#pragma once

#include "cloud-providers/cloud-provider.h"

#include <deque>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <queue>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

class RevAIProvider : public CloudProvider {
public:
	RevAIProvider(TranscriptionCallback callback, cloudvocal_data *gf);

	virtual bool init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;
	virtual void readResultsFromTranscription() override;
	virtual void shutdown() override;

private:
	// Utility functions
	std::vector<int16_t> convertFloatToS16LE(const std::deque<float> &audio_buffer);

	// Member variables
	bool is_connected;
	std::string job_id;

	net::io_context ioc_;
	ssl::context ctx_;
	websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
	const std::string host_ = "api.rev.ai";
	const std::string target_ = "/speechtotext/v1/stream";
};
