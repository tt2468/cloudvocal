#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <string>

#include "cloudvocal-processing.h"
#include "cloudvocal-data.h"

class CloudProvider {
public:
	using TranscriptionCallback = std::function<void(const std::string &)>;

	CloudProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
		: transcription_callback(callback),
		  running(false),
		  gf(gf_)
	{
	}

	virtual ~CloudProvider() { stop(); }

	virtual void init() = 0;

	void start()
	{
		running = true;
		transcription_thread = std::thread(&CloudProvider::processAudio, this);
	}

	void stop()
	{
		running = false;
		if (transcription_thread.joinable()) {
			transcription_thread.join();
		}
	}

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) = 0;

	void processAudio()
	{
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

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	void reportTranscriptionResult(const std::string &result)
	{
		if (transcription_callback) {
			transcription_callback(result);
		}
	}

private:
	TranscriptionCallback transcription_callback;
	std::thread transcription_thread;
	std::atomic<bool> running;
	cloudvocal_data *gf;
};
