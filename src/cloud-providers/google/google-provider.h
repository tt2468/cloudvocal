#pragma once

#include "cloud-providers/cloud-provider.h"
#include <grpcpp/grpcpp.h>
#include "google/cloud/speech/v1/cloud_speech.grpc.pb.h"

class GoogleProvider : public CloudProvider {
public:
	GoogleProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
		: CloudProvider(callback, gf_),
		  initialized(false),
		  channel(nullptr),
		  stub(nullptr),
		  reader_writer(nullptr),
		  chunk_id(1)
	{
	}

	virtual bool init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;
	virtual void readResultsFromTranscription() override;
	virtual void shutdown() override;

private:
	std::shared_ptr<grpc::Channel> channel;
	std::unique_ptr<google::cloud::speech::v1::Speech::Stub> stub;
	grpc::ClientContext context;
	std::unique_ptr<
		::grpc::ClientReaderWriter<::google::cloud::speech::v1::StreamingRecognizeRequest,
					   ::google::cloud::speech::v1::StreamingRecognizeResponse>>
		reader_writer;
	bool initialized;
	uint64_t chunk_id;
};
