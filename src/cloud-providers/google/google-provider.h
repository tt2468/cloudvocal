#pragma once

#include "cloud-providers/cloud-provider.h"

class GoogleProvider : public CloudProvider {
public:
	GoogleProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
		: CloudProvider(callback, gf_)
	{
	}

	virtual bool init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;
	virtual void readResultsFromTranscription() override;
	virtual void shutdown() override;

private:
	bool initialized;
};
