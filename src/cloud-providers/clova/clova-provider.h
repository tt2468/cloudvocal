#pragma once

#include "cloud-providers/cloud-provider.h"
#include <deque>
#include <string>
#include <map>
#include <chrono>

#include <grpcpp/grpcpp.h>

#include "cloud-providers/clova/nest.grpc.pb.h"
#include "nlohmann/json.hpp"

using grpc::ClientReaderWriter;
using NestRequest = com::nbp::cdncp::nest::grpc::proto::v1::NestRequest;
using NestResponse = com::nbp::cdncp::nest::grpc::proto::v1::NestResponse;

class ClovaProvider : public CloudProvider {
public:
	ClovaProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
		: CloudProvider(callback, gf_)
	{
	}

	virtual void init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;

private:
	std::map<int, std::chrono::steady_clock::time_point> chunk_start_times;
	int chunk_id;
	std::unique_ptr<grpc::ClientReaderWriter<NestRequest, NestResponse>> reader_writer;
};
