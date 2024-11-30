#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <string>

#include "cloudvocal-processing.h"
#include "cloudvocal-data.h"
#include "plugin-support.h"

class CloudProvider {
public:
	using TranscriptionCallback = std::function<void(const DetectionResultWithText &)>;

	CloudProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
		: transcription_callback(callback),
		  running(false),
		  gf(gf_)
	{
	}

	virtual ~CloudProvider() { stop(); }

	virtual bool init() = 0;

	void start()
	{
		running = true;
		transcription_thread = std::thread(&CloudProvider::processAudio, this);
		results_thread = std::thread(&CloudProvider::processResults, this);
	}

	void stop()
	{
		running = false;
		if (transcription_thread.joinable()) {
			transcription_thread.join();
		}
		if (results_thread.joinable()) {
			results_thread.join();
		}
	}

	bool isRunning() const { return running; }

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) = 0;
	virtual void readResultsFromTranscription() = 0;
	virtual void shutdown() = 0;

	void processAudio()
	{
		// Initialize the cloud provider
		if (!init()) {
			obs_log(LOG_ERROR, "Failed to initialize cloud provider");
			running = false;
			return;
		}

		uint64_t start_timestamp_offset_ns = 0;
		uint64_t end_timestamp_offset_ns = 0;

		while (running) {
			get_data_from_buf_and_resample(gf, start_timestamp_offset_ns,
						       end_timestamp_offset_ns);

			if (gf->resampled_buffer.empty()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			sendAudioBufferToTranscription(gf->resampled_buffer);

			gf->resampled_buffer.clear();

			// sleep until the next audio packet is ready
			// wait for notificaiton from the audio buffer condition variable
			std::unique_lock<std::mutex> lock(gf->input_buffers_mutex);
			gf->input_buffers_cv.wait(lock, [this] {
				return !(gf->input_buffers[0]).empty() || !running;
			});
		}

		// Shutdown the cloud provider
		shutdown();
	}

	void processResults()
	{
		while (running) {
			readResultsFromTranscription();
		}
	}

	cloudvocal_data *gf;
	std::atomic<bool> running;
	TranscriptionCallback transcription_callback;

private:
	std::thread transcription_thread;
	std::thread results_thread;
};

std::shared_ptr<CloudProvider> createCloudProvider(const std::string &providerType,
						   CloudProvider::TranscriptionCallback callback,
						   cloudvocal_data *gf);

void restart_cloud_provider(cloudvocal_data *gf);
