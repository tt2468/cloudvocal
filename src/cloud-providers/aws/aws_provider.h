#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "cloud-providers/cloud-provider.h"

namespace Aws {
namespace TranscribeStreamingService {
class TranscribeStreamingServiceClient;
namespace Model {
class StartStreamTranscriptionRequest;
class StartStreamTranscriptionHandler;
} // namespace Model
} // namespace TranscribeStreamingService
} // namespace Aws

class AWSProvider : public CloudProvider {
public:
	AWSProvider(TranscriptionCallback callback, cloudvocal_data *gf)
		: CloudProvider(callback, gf)
	{
		needs_results_thread = false;
	}

	virtual ~AWSProvider() {}

	virtual bool init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;

	virtual void readResultsFromTranscription() override;

	virtual void shutdown() override;

private:
	std::shared_ptr<Aws::TranscribeStreamingService::TranscribeStreamingServiceClient> client;
	std::shared_ptr<Aws::TranscribeStreamingService::Model::StartStreamTranscriptionRequest>
		request;
	std::shared_ptr<Aws::TranscribeStreamingService::Model::StartStreamTranscriptionHandler>
		handler;

	std::queue<std::vector<float>> audio_buffer_queue;
	std::mutex audio_buffer_queue_mutex;
	std::condition_variable audio_buffer_queue_cv;
	std::atomic<bool> stream_open = false;
};