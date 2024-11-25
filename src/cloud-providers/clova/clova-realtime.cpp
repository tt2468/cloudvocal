#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <map>
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
	ResponseObserver(const std::string &file_path, const std::string &source_lang,
			 std::map<int, std::chrono::steady_clock::time_point> &chunk_start_times)
		: file_path(file_path),
		  source_lang(source_lang),
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
		/*
        std::cout << "---------- transcription ----------" << std::endl;
        std::ofstream outfile(file_path + ".transcript", std::ios::out | std::ios::trunc);
        if (outfile.is_open()) {
            outfile << transcription;
            outfile.close();
        } else {
            std::cerr << "Unable to open file for writing transcription" << std::endl;
        }
        */
	}

private:
	std::string file_path;
	std::string source_lang;
	std::string sentence_sofar;
	std::string transcription;
	std::map<int, std::chrono::steady_clock::time_point> &chunk_start_times;
	std::map<int, double> chunk_latencies;
};

class RequestGenerator {
public:
	RequestGenerator(const std::string &file_path, const std::string &source_lang)
		: file_path(file_path),
		  source_lang(source_lang)
	{
	}
	void
	GenerateRequests(std::function<void(const NestRequest &)> yield,
			 std::map<int, std::chrono::steady_clock::time_point> &chunk_start_times)
	{
		NestRequest config_request;
		config_request.set_type(RequestType::CONFIG);
		config_request.mutable_config()->set_config("{\"transcription\":{\"language\":\"" +
							    source_lang + "\"}}");
		yield(config_request);
		std::ifstream file(file_path, std::ios::binary);
		int chunk_id = 1;
		std::vector<char> buffer(32000);
		while (file) {
			file.read(buffer.data(), buffer.size());
			std::streamsize bytes_read = file.gcount();
			if (bytes_read > 0) {
				chunk_start_times[chunk_id] = std::chrono::steady_clock::now();
				NestRequest data_request;
				data_request.set_type(RequestType::DATA);
				data_request.mutable_data()->set_chunk(buffer.data(), bytes_read);
				data_request.mutable_data()->set_extra_contents(
					"{\"seqId\": " + std::to_string(chunk_id) +
					", \"epFlag\": true}");
				yield(data_request);
				chunk_id++;
			}
		}
		// Wait for 5 seconds before completing
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

private:
	std::string file_path;
	std::string source_lang;
};

void RunRecognition(const std::string &file_path, const std::string &source_lang)
{
	grpc::SslCredentialsOptions ssl_opts;
	auto channel = grpc::CreateChannel("clovaspeech-gw.ncloud.com:50051",
					   grpc::SslCredentials(ssl_opts));
	std::unique_ptr<NestService::Stub> stub = NestService::NewStub(channel);
	ClientContext context;
	context.AddMetadata("authorization", "Bearer xxx");
	std::map<int, std::chrono::steady_clock::time_point> chunk_start_times;
	ResponseObserver response_observer(file_path, source_lang, chunk_start_times);
	RequestGenerator request_generator(file_path, source_lang);
	auto reader_writer = stub->recognize(&context);
	std::thread writer([&]() {
		request_generator.GenerateRequests(
			[&](const NestRequest &request) { reader_writer->Write(request); },
			chunk_start_times);
		reader_writer->WritesDone();
	});
	NestResponse response;
	while (reader_writer->Read(&response)) {
		response_observer.OnNext(response);
	}
	writer.join();
	Status status = reader_writer->Finish();
	if (!status.ok()) {
		response_observer.OnError(status);
	}
	response_observer.OnCompleted();
}

// int main(int argc, char *argv[])
// {
// 	if (argc != 3) {
// 		std::cerr << "Usage: " << argv[0] << " <file_path> <source_language>" << std::endl;
// 		return 1;
// 	}
// 	RunRecognition(argv[1], argv[2]);
// 	return 0;
// }
