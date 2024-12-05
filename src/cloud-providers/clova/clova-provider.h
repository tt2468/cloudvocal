#pragma once

#include "cloud-providers/cloud-provider.h"
#include <deque>
#include <string>
#include <map>
#include <chrono>
#include <memory>

#include <grpcpp/grpcpp.h>

#include "cloud-providers/clova/nest.grpc.pb.h"
#include "nlohmann/json.hpp"

using grpc::ClientReaderWriter;
using grpc::Channel;
using grpc::ClientContext;

using NestRequest = com::nbp::cdncp::nest::grpc::proto::v1::NestRequest;
using NestResponse = com::nbp::cdncp::nest::grpc::proto::v1::NestResponse;
using NestService = com::nbp::cdncp::nest::grpc::proto::v1::NestService;
typedef grpc::ClientReaderWriter<NestRequest, NestResponse> ClovaReaderWriter;

class ClovaProvider : public CloudProvider {
public:
	ClovaProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
		: CloudProvider(callback, gf_),
		  chunk_id(1),
		  reader_writer(nullptr),
		  channel(nullptr),
		  stub(nullptr),
		  initialized(false),
		  current_sentence("")
	{
		needs_results_thread = true;
	}

	virtual bool init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;
	virtual void readResultsFromTranscription() override;
	virtual void shutdown() override;

private:
	std::map<uint64_t, std::chrono::steady_clock::time_point> chunk_start_times;
	uint64_t chunk_id;
	std::unique_ptr<ClovaReaderWriter> reader_writer;
	std::shared_ptr<grpc::Channel> channel;
	std::unique_ptr<NestService::Stub> stub;
	ClientContext context;
	std::map<int, double> chunk_latencies;
	std::string current_sentence;
	bool initialized;
};
