#include "revai-provider.h"

#include <algorithm>
#include <string>
#include <functional>
#include <queue>
#include <memory>
#include <vector>
#include <iostream>

RevAIProvider::RevAIProvider(TranscriptionCallback callback, cloudvocal_data *gf)
	: CloudProvider(callback, gf)
{
	is_connected = false;
}

bool RevAIProvider::init()
{
	std::string url =
		"wss://api.rev.ai/speechtotext/v1/stream"
		"?access_token=" +
		this->gf->cloud_provider_api_key +
		"&content_type=audio/x-raw;layout=interleaved;rate=16000;format=S16LE;channels=1";

	return true;
}

void RevAIProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{

}

void RevAIProvider::shutdown()
{
}

std::vector<int16_t> RevAIProvider::convertFloatToS16LE(const std::deque<float> &audio_buffer)
{
	std::vector<int16_t> converted;
	converted.reserve(audio_buffer.size());

	for (float sample : audio_buffer) {
		// Clamp to [-1.0, 1.0] and convert to S16LE
		sample = std::fmaxf(-1.0f, std::fminf(1.0f, sample));
		converted.push_back(static_cast<int16_t>(sample * 32767.0f));
	}
	return converted;
}
