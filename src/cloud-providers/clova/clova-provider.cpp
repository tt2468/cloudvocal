#include "clova-provider.h"
#include <iostream>

#include "plugin-support.h"

#include <grpcpp/grpcpp.h>

#include "cloud-providers/clova/nest.grpc.pb.h"
#include "nlohmann/json.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReaderWriter;
using grpc::Status;

using NestService = com::nbp::cdncp::nest::grpc::proto::v1::NestService;
using NestRequest = com::nbp::cdncp::nest::grpc::proto::v1::NestRequest;
using NestResponse = com::nbp::cdncp::nest::grpc::proto::v1::NestResponse;
using NestConfig = com::nbp::cdncp::nest::grpc::proto::v1::NestConfig;
using NestData = com::nbp::cdncp::nest::grpc::proto::v1::NestData;
using RequestType = com::nbp::cdncp::nest::grpc::proto::v1::RequestType;
using json = nlohmann::json;

class ResponseObserver {
public:
	ResponseObserver(const std::string &source_lang,
			 std::map<int, std::chrono::steady_clock::time_point> &chunk_start_times)
		: source_lang(source_lang),
		  chunk_start_times(chunk_start_times)
	{
	}
	void OnNext(const NestResponse &response)
	{
		auto end_time = std::chrono::steady_clock::now();
		json json_response_data = json::parse(response.contents());
		if (json_response_data.contains("transcription") &&
		    json_response_data["transcription"].contains("text")) {
			std::string text_value = json_response_data["transcription"]["text"];
			int seq_id = json_response_data["transcription"].value("seqId", -1);
			if (seq_id != -1 && chunk_start_times.count(seq_id)) {
				auto start_time = chunk_start_times[seq_id];
				double latency =
					std::chrono::duration<double>(end_time - start_time).count();
				chunk_latencies[seq_id] = latency;
				std::cout << "Chunk " << seq_id << " latency: " << (latency * 1000)
					  << " milliseconds" << std::endl;
			}
			if (text_value.empty() && !sentence_sofar.empty()) {
				std::cout << "Complete sentence: " << sentence_sofar << std::endl;
				transcription += sentence_sofar + "\n";
				sentence_sofar.clear();
			} else {
				sentence_sofar += text_value;
				std::cout << "Partial transcription: " << text_value << std::endl;
			}
		}
	}

	void OnError(const grpc::Status &status)
	{
		std::cerr << "Error received: " << status.error_message() << std::endl;
	}

	void OnCompleted()
	{
		std::cout << "Stream completed" << std::endl;
		std::cout << "---------- Chunk Latencies ----------" << std::endl;
		for (const auto &entry : chunk_latencies) {
			int seq_id = entry.first;
			double latency = entry.second;
			std::cout << "Chunk " << seq_id << " latency: " << (latency * 1000)
				  << " milliseconds" << std::endl;
		}
		double avg_latency = 0;
		if (!chunk_latencies.empty()) {
			avg_latency = std::accumulate(chunk_latencies.begin(),
						      chunk_latencies.end(), 0.0,
						      [](double sum, const auto &p) {
							      return sum + p.second;
						      }) /
				      chunk_latencies.size();
		}
		std::cout << "Average latency: " << avg_latency << " seconds" << std::endl;
		std::cout << "Average latency: " << (avg_latency * 1000) << " milliseconds"
			  << std::endl;
		if (!sentence_sofar.empty()) {
			std::cout << sentence_sofar << std::endl;
			transcription += sentence_sofar + "\n";
		}
	}

private:
	std::string source_lang;
	std::string sentence_sofar;
	std::string transcription;
	std::map<int, std::chrono::steady_clock::time_point> &chunk_start_times;
	std::map<int, double> chunk_latencies;
};

void ClovaProvider::init()
{
	// Initialize the Clova provider
	obs_log(LOG_INFO, "Initializing Clova provider");
	// Add your initialization code here
	chunk_id = 1;

	grpc::SslCredentialsOptions ssl_opts;
	auto channel = grpc::CreateChannel("clovaspeech-gw.ncloud.com:50051",
					   grpc::SslCredentials(ssl_opts));
	std::unique_ptr<NestService::Stub> stub = NestService::NewStub(channel);
	ClientContext context;
	context.AddMetadata("authorization", "Bearer xxx");
	reader_writer = stub->recognize(&context);
	if (!reader_writer) {
		obs_log(LOG_ERROR, "Failed to create reader writer for Clova");
		return;
	}
}

void ClovaProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	if (!reader_writer) {
		obs_log(LOG_ERROR, "Reader writer is not initialized");
		return;
	}
	// Send the audio buffer to Clova for transcription
	obs_log(LOG_INFO, "Sending audio buffer (%d) to Clova for transcription",
		audio_buffer.size());
	NestRequest config_request;
	config_request.set_type(RequestType::CONFIG);
	config_request.mutable_config()->set_config("{\"transcription\":{\"language\":\"" +
						    gf->language + "\"}}");

	bool success = reader_writer->Write(config_request);
	if (!success) {
		obs_log(LOG_ERROR, "Failed to send config request to Clova");
		return;
	}

	chunk_start_times[chunk_id] = std::chrono::steady_clock::now();
	NestRequest data_request;
	data_request.set_type(RequestType::DATA);

	// copy buffer to vector
	std::vector<float> audio_buffer_vector(audio_buffer.begin(), audio_buffer.end());

	data_request.mutable_data()->set_chunk(audio_buffer_vector.data(),
					       audio_buffer_vector.size() * sizeof(float));
	data_request.mutable_data()->set_extra_contents("{\"seqId\": " + std::to_string(chunk_id) +
							", \"epFlag\": true}");

	success = reader_writer->Write(data_request);
	if (!success) {
		obs_log(LOG_ERROR, "Failed to send data request to Clova");
		return;
	}

	chunk_id++;
}
