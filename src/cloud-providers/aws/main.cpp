#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include "presigned_url.h"
#include "eventstream.h"
#include <algorithm>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

// Sound settings
const std::string language_code = "en-US";
const std::string media_encoding = "pcm";
const std::string websocket_key;
const int sample_rate = 8000;
const int number_of_channels = 1;
const bool channel_identification = true;
const int bytes_per_sample = 2; // 16 bit audio
const int chunk_size = (sample_rate * 2 * number_of_channels / 10) * 2; // roughly 100ms of audio data
const std::string file_name = "example_call_2_channel.wav";

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient>
{
public:
    explicit WebSocketClient(net::io_context& ioc, ssl::context& ctx,
                             const std::string& host, const std::string& port,
                             const std::string& target)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc), ctx)
        , host_(host)
        , port_(port)
        , target_(target)
    {
    }

    void run()
    {
        resolver_.async_resolve(
            host_,
            port_,
            beast::bind_front_handler(
                &WebSocketClient::on_resolve,
                shared_from_this()));
    }
private:
    std::ifstream wav_file;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");
        if(!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
            return fail(beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()), "set SNI Hostname");
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &WebSocketClient::on_connect,
                shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
    {
        if(ec)
            return fail(ec, "connect");
        ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                &WebSocketClient::on_ssl_handshake,
                shared_from_this()));
    }

    void on_ssl_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "ssl_handshake");
        //std::cout << "SSL Handshake successful" << std::endl;
        beast::get_lowest_layer(ws_).expires_never();
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async");
                req.set(http::field::connection, "Upgrade");
                req.set(http::field::upgrade, "websocket");
                //req.set(http::field::origin, "ec2-44-203-201-70.compute-1.amazonaws.com");
                req.set(http::field::origin, "localhost");
                req.set(http::field::sec_websocket_version, "13");



            }));
        ws_.async_handshake(host_, target_,
            beast::bind_front_handler(
                &WebSocketClient::on_handshake,
                shared_from_this()));
    }

    void on_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");
        std::cout << "WebSocket connection established" << std::endl;
        send();
        receive();
    }
   
    void log_hex(const std::vector<uint8_t>& data, const std::string& label) {
        std::cout << label << " (size: " << data.size() << "): ";
        for (size_t i = 0; i < std::min(data.size(), size_t(32)); ++i) {
            printf("%02x ", data[i]);
        }
        std::cout << std::endl;
    }

    void skip_wav_header(std::ifstream& file) {

        if (!file.is_open()) {
            throw std::runtime_error("File is not open");
        }

        // Read RIFF header
        char riff_header[4];
        file.read(riff_header, 4);
        if (std::string(riff_header, 4) != "RIFF") {
            throw std::runtime_error("Not a valid WAV file: RIFF header missing");
        }

        // Skip file size
        file.seekg(4, std::ios::cur);

        // Read WAVE header
        char wave_header[4];
        file.read(wave_header, 4);
        if (std::string(wave_header, 4) != "WAVE") {
            throw std::runtime_error("Not a valid WAV file: WAVE header missing");
        }

        // Find "fmt " chunk
        while (true) {
            char chunk_header[4];
            file.read(chunk_header, 4);
            
            uint32_t chunk_size;
            file.read(reinterpret_cast<char*>(&chunk_size), 4);

            if (std::string(chunk_header, 4) == "fmt ") {
                // Skip the rest of the fmt chunk
                file.seekg(chunk_size, std::ios::cur);
                break;
            } else {
                // Skip this chunk
                file.seekg(chunk_size, std::ios::cur);
            }
        }

        // Find "data" chunk
        while (true) {
            char chunk_header[4];
            file.read(chunk_header, 4);
            
            uint32_t chunk_size;
            file.read(reinterpret_cast<char*>(&chunk_size), 4);
            if (std::string(chunk_header, 4) == "data") {
                // We've found the data chunk, don't skip it
                file.seekg(-8, std::ios::cur);  // Move back to start of "data" chunk
                break;
            } else {
                // Skip this chunk
                file.seekg(chunk_size, std::ios::cur);
            }
        }
    }


std::vector<unsigned char> read_audio_chunk(std::ifstream& file, size_t chunk_size) {
    // Ensure chunk_size is even (for 16-bit samples)
    chunk_size = (chunk_size / 2) * 2;
    std::vector<int16_t> buffer(chunk_size / sizeof(int16_t));
    file.read(reinterpret_cast<char*>(buffer.data()), chunk_size);
    
    if (file.fail() && !file.eof()) {
        std::cerr << "Error: Failed to read from the file." << std::endl;
        std::cerr << "Error reading file: " << std::strerror(errno) << std::endl;
        return {};
    }
    
    std::streamsize bytes_read = file.gcount();

    //std::vector<uint8_t> audio_data;
    std::vector<unsigned char> audio_data;
    audio_data.reserve(bytes_read);
    
    for (size_t i = 0; i < bytes_read / sizeof(int16_t); ++i) {
        int16_t sample = buffer[i];

        // Little-endian byte order
        audio_data.push_back(static_cast<unsigned char>(sample & 0xFF));
        audio_data.push_back(static_cast<unsigned char>((sample >> 8) & 0xFF));
    }
    return audio_data;
}


    void send_next_chunk()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::vector<unsigned char> audio_chunk = read_audio_chunk(wav_file, chunk_size);

        if (!audio_chunk.empty()) {
            std::vector<uint8_t> audio_event = create_audio_event(audio_chunk);

            ws_.binary(true);
            ws_.async_write(
                net::buffer(audio_event),
                beast::bind_front_handler(
                    &WebSocketClient::on_write,
                    shared_from_this()));
        } else {
            std::cout << "Audio chunk is empty" << std::endl;
        }
    }
    

    void send()
    {
       wav_file.open(file_name, std::ios_base::binary);
       skip_wav_header(wav_file);

        if (!wav_file) {
            std::cerr << "Failed to open file: " << file_name << std::endl;
            return;
        }

        send_next_chunk();
    }


    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
            return fail(ec, "write");
        }

        send_next_chunk();
    }

    void receive()
    {
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &WebSocketClient::on_read,
                shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        if(ec)
            return fail(ec, "read");

        std::vector<uint8_t> message_bytes(boost::asio::buffers_begin(buffer_.data()),
                                           boost::asio::buffers_end(buffer_.data()));
        // Decode the event
        auto [header, payload] = decode_event(message_bytes);
        if (header[":message-type"] == "event") {
            //std::cout << "Results: " << payload["Transcript"] << std::endl;
            if (!payload["Transcript"]["Results"].empty()) {
                std::cout << "Transcript: " << payload["Transcript"]["Results"][0]["Alternatives"][0]["Transcript"] << std::endl;
            }
        } else if (header[":message-type"] == "exception") {
            std::cerr << "Exception: " << payload["Message"] << std::endl;
        }
        buffer_.consume(buffer_.size());
        receive();
    }

    void on_close(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "close");
        std::cout << "WebSocket connection closed" << std::endl;
    }

    void fail(beast::error_code ec, char const* what)
    {
        std::cerr << what << ": " << ec.message() << "\n";
    }
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    std::string target_;
};

void load_certificate_from_file(ssl::context& ctx, const std::string& filename)
{
    std::ifstream cert_file(filename);
    if (!cert_file.is_open()) {
        throw std::runtime_error("Failed to open certificate file");
    }

    std::stringstream cert_stream;
    cert_stream << cert_file.rdbuf();
    std::string cert = cert_stream.str();

    ctx.add_certificate_authority(
        boost::asio::buffer(cert.data(), cert.size())
    );
}

int main()
{
    try
    {
        // Configure access
        std::string access_key = std::getenv("AWS_ACCESS_KEY_ID") ? std::getenv("AWS_ACCESS_KEY_ID") : "";
        std::string secret_key = std::getenv("AWS_SECRET_ACCESS_KEY") ? std::getenv("AWS_SECRET_ACCESS_KEY") : "";

        std::string session_token = std::getenv("AWS_SESSION_TOKEN") ? std::getenv("AWS_SESSION_TOKEN") : "";
        std::string region = std::getenv("AWS_DEFAULT_REGION") ? std::getenv("AWS_DEFAULT_REGION") : "us-west-2";
        AWSTranscribePresignedURL transcribe_url_generator(access_key, secret_key, session_token, region);
        // Generate signed url to connect to
        std::string request_url = transcribe_url_generator.get_request_url(
            sample_rate, language_code, media_encoding, number_of_channels, channel_identification);
        
        // Generate random websocket key
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 61);
        const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::string websocket_key(20, 0);
        for (auto& c : websocket_key) {
            c = charset[dis(gen)];
        }

        // Parse the URL to get host, port, and target
        std::string host, port, target;
        std::string::size_type protocol_end = request_url.find("://");
        if (protocol_end != std::string::npos) {
            std::string::size_type host_start = protocol_end + 3;
            std::string::size_type host_end = request_url.find(':', host_start);
            if (host_end != std::string::npos) {
                host = request_url.substr(host_start, host_end - host_start);
                std::string::size_type port_end = request_url.find('/', host_end);
                port = request_url.substr(host_end + 1, port_end - host_end - 1);
                target = request_url.substr(port_end);
            } else {
                host_end = request_url.find('/', host_start);
                host = request_url.substr(host_start, host_end - host_start);
                port = "443"; // Default HTTPS port
                target = request_url.substr(host_end);
            }
        } else {
            throw std::runtime_error("Invalid URL format");
        }

        // The io_context is required for all I/O
        net::io_context ioc;
        // The SSL context is required for SSL
        ssl::context ctx{ssl::context::tlsv12_client};
        //ctx.set_default_verify_paths();

        load_certificate_from_file(ctx, "starfield_root_cert.cer");

        ctx.set_verify_mode(ssl::verify_peer);
        ctx.set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
            if (!preverified) {
                X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
                char subject_name[256];
                X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
                std::cout << "Verification failed for: " << subject_name << std::endl;
            }
            return preverified;
        });
        ctx.set_default_verify_paths();
        // Create and run the WebSocket client
        std::make_shared<WebSocketClient>(ioc, ctx, host, port, target)->run();
        // Run the I/O service
        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}