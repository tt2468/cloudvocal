#pragma once

#include "cloud-providers/cloud-provider.h"

#include <deque>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <queue>
#include <string>

class RevAIProvider : public CloudProvider {
public:
	RevAIProvider(TranscriptionCallback callback, cloudvocal_data *gf);

	virtual bool init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;
	virtual void readResultsFromTranscription() override;
	virtual void shutdown() override;

private:
	// Utility functions
	std::vector<int16_t> convertFloatToS16LE(const std::deque<float> &audio_buffer);

	// Member variables
	bool is_connected;
	std::string job_id;

};
