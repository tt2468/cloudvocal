#pragma once

#include "cloud-providers/cloud-provider.h"
#include <deque>
#include <string>

class ClovaProvider : public CloudProvider {
public:
	ClovaProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
		: CloudProvider(callback, gf_)
	{
	}

	virtual void init() override;

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;
};
