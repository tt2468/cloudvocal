#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>

#define BOOST_URL_NO_LIB 1
#include <boost/url/parse.hpp>

namespace urls = boost::urls;

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <deque>

#include <util/base.h>

#include "presigned_url.h"
#include "eventstream.h"
#include "utils/ssl-utils.h"
#include "plugin-support.h"
#include "aws_provider.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

// Sound settings
const std::string language_code = "en-US";
const std::string media_encoding = "pcm";
const std::string websocket_key;
const int sample_rate = 16000;
const int number_of_channels = 1;
const bool channel_identification = true;
const int bytes_per_sample = 2; // 16 bit audio
const int chunk_size =
	(sample_rate * 2 * number_of_channels / 10) * 2; // roughly 100ms of audio data

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
	explicit WebSocketClient(net::io_context &ioc, ssl::context &ctx, const std::string &host,
				 const std::string &port, const std::string &target)
		: resolver_(net::make_strand(ioc)),
		  ws_(net::make_strand(ioc), ctx),
		  host_(host),
		  port_(port),
		  target_(target),
		  buffer_(),
		  closed_(true)
	{
	}

	void run()
	{
		resolver_.async_resolve(host_, port_,
					beast::bind_front_handler(&WebSocketClient::on_resolve,
								  shared_from_this()));
	}

	void close()
	{
		if (closed_) {
			return;
		}
		closed_ = true;
		ws_.async_close(websocket::close_code::normal,
				beast::bind_front_handler(&WebSocketClient::on_close,
							  shared_from_this()));
	}

	bool isClosed() const { return closed_; }
	bool isStopRequested() const { return stop_requested; }

private:
	void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
	{
		if (ec)
			return fail(ec, "resolve");
		if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
			return fail(beast::error_code(static_cast<int>(::ERR_get_error()),
						      net::error::get_ssl_category()),
				    "set SNI Hostname");
		beast::get_lowest_layer(ws_).async_connect(
			results, beast::bind_front_handler(&WebSocketClient::on_connect,
							   shared_from_this()));
	}

	void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
	{
		if (ec)
			return fail(ec, "connect");
		ws_.next_layer().async_handshake(
			ssl::stream_base::client,
			beast::bind_front_handler(&WebSocketClient::on_ssl_handshake,
						  shared_from_this()));
	}

	void on_ssl_handshake(beast::error_code ec)
	{
		if (ec)
			return fail(ec, "ssl_handshake");

		beast::get_lowest_layer(ws_).expires_never();
		ws_.set_option(
			websocket::stream_base::timeout::suggested(beast::role_type::client));
		ws_.set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
			req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) +
								 " websocket-client-async");
			req.set(http::field::connection, "Upgrade");
			req.set(http::field::upgrade, "websocket");
			req.set(http::field::origin, "localhost");
			req.set(http::field::sec_websocket_version, "13");
		}));
		ws_.async_handshake(host_, target_,
				    beast::bind_front_handler(&WebSocketClient::on_handshake,
							      shared_from_this()));
	}

	void on_handshake(beast::error_code ec)
	{
		if (ec) {
			fail(ec, "handshake");
			return;
		}
		obs_log(LOG_INFO, "WebSocket connection established");
		closed_ = false;
	}

public:
	void send_audio_chunk(const std::deque<float> &audio_buffer)
	{
		if (closed_) {
			obs_log(LOG_ERROR, "WebSocket connection is closed");
			return;
		}
		obs_log(LOG_INFO, "Sending audio chunk");
		if (!audio_buffer.empty()) {
			std::vector<uint8_t> audio_chunk;
			audio_chunk.reserve(audio_buffer.size() * bytes_per_sample);
			for (auto sample : audio_buffer) {
				int16_t sample_int = static_cast<int16_t>(sample * 32767);
				audio_chunk.push_back(sample_int & 0xFF);
				audio_chunk.push_back((sample_int >> 8) & 0xFF);
			}

			std::vector<uint8_t> audio_event = create_audio_event(audio_chunk);

			ws_.binary(true);
			ws_.async_write(net::buffer(audio_event),
					beast::bind_front_handler(&WebSocketClient::on_write,
								  shared_from_this()));
		} else {
			obs_log(LOG_INFO, "Audio chunk is empty");
		}
	}

	void receive()
	{
		if (closed_) {
			obs_log(LOG_ERROR, "receive: WebSocket connection is closed");
			// sleep for a while before trying to read again
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			return;
		}
		beast::error_code ec;
		ws_.read(buffer_, ec);
		if (ec) {
			fail(ec, "read");
			return;
		}
		obs_log(LOG_INFO, "Received message");
		on_read(ec, buffer_.size());
	}

private:
	void on_write(beast::error_code ec, std::size_t bytes_transferred)
	{
		if (ec) {
			return fail(ec, "write");
		}
	}

	void on_read(beast::error_code ec, std::size_t bytes_transferred)
	{
		if (ec) {
			fail(ec, "read");
			return;
		}
		std::vector<uint8_t> message_bytes(boost::asio::buffers_begin(buffer_.data()),
						   boost::asio::buffers_end(buffer_.data()));
		// Decode the event
		auto [header, payload] = decode_event(message_bytes);
		if (header[":message-type"] == "event") {
			if (!payload["Transcript"]["Results"].empty()) {
				obs_log(LOG_INFO, "Transcript: %s",
					payload["Transcript"]["Results"][0]["Alternatives"][0]
					       ["Transcript"]
						       .get<std::string>()
						       .c_str());
			}
		} else if (header[":message-type"] == "exception") {
			obs_log(LOG_ERROR, "Exception: %s",
				payload["Message"].get<std::string>().c_str());
			close();
			this->stop_requested = true;
		}
		buffer_.consume(buffer_.size());
	}

	void on_close(beast::error_code ec)
	{
		if (ec) {
			fail(ec, "close");
			return;
		}
		obs_log(LOG_INFO, "WebSocket connection closed");
		closed_ = true;
	}

	void fail(beast::error_code ec, char const *what)
	{
		obs_log(LOG_ERROR, "Error: %s: %s", what, ec.message().c_str());
	}

	tcp::resolver resolver_;
	websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
	beast::flat_buffer buffer_;
	std::string host_;
	std::string port_;
	std::string target_;
	bool closed_ = false;
	std::atomic<bool> stop_requested = false;
};

bool AWSProvider::init()
{
	obs_log(LOG_INFO, "Initializing AWS provider");

	if (gf->cloud_provider_api_key.empty() || gf->cloud_provider_secret_key.empty()) {
		obs_log(LOG_ERROR, "AWS provider requires API key and secret key");
		return false;
	}

	try {
		// Configure access
		AWSTranscribePresignedURL transcribe_url_generator(gf->cloud_provider_api_key,
								   gf->cloud_provider_secret_key);
		// Generate signed url to connect to
		std::string request_url = transcribe_url_generator.get_request_url(
			sample_rate, language_code, media_encoding, number_of_channels,
			channel_identification);

		// Generate random websocket key
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, 61);
		const char charset[] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
		std::string websocket_key(20, 0);
		for (auto &c : websocket_key) {
			c = charset[dis(gen)];
		}

		// Parse the URL
		boost::system::result<urls::url_view> result = urls::parse_uri(request_url);

		if (result.has_error()) {
			obs_log(LOG_ERROR, "Failed to parse URL: %s",
				result.error().message().c_str());
			return false;
		}

		const urls::url_view &url = result.value();
		// Parse the URL to get host, port, and target
		std::string host = url.host();
		std::string port = url.port();
		if (port.empty()) {
			port = "443";
		}
		std::string target = url.path();

		const std::string cert = PEMrootCerts();
		ctx.add_certificate_authority(boost::asio::buffer(cert.data(), cert.size()));

		obs_log(LOG_INFO, "Connecting to %s:%s", host.c_str(), port.c_str());

		ctx.set_verify_mode(ssl::verify_peer);
		ctx.set_verify_callback([](bool preverified, ssl::verify_context &ctx) {
			if (!preverified) {
				X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
				char subject_name[256];
				X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
				obs_log(LOG_ERROR, "Verification failed for: %s", subject_name);
			} else {
				obs_log(LOG_INFO, "Verification succeeded");
			}
			return preverified;
		});
		ctx.set_default_verify_paths();
		// Create and run the WebSocket client
		ws_client_ = std::make_shared<WebSocketClient>(ioc, ctx, host, port, target);

		ws_client_->run();
		// Run the I/O service
		ioc.run();
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error: %s", e.what());
		return false;
	}
	obs_log(LOG_INFO, "AWS provider initialized");
	return true;
}

void AWSProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	if (ws_client_) {
		if (ws_client_->isStopRequested()) {
			obs_log(LOG_ERROR, "WebSocket stop requested");
			this->stop_requested = true;
			return;
		}
		ws_client_->send_audio_chunk(audio_buffer);
	}
}

void AWSProvider::readResultsFromTranscription()
{
	if (ws_client_) {
		if (ws_client_->isStopRequested()) {
			obs_log(LOG_ERROR, "WebSocket stop requested");
			this->stop_requested = true;
			return;
		}
		ws_client_->receive();
	}
}

void AWSProvider::shutdown()
{
	if (ws_client_) {
		ws_client_->close();
		ws_client_.reset();
	}
}
