#include "google-provider.h"

bool GoogleProvider::init()
{
	// Stub implementation
	initialized = true;
	return initialized;
}

void GoogleProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	// Stub implementation
}

void GoogleProvider::readResultsFromTranscription()
{
	// Stub implementation
}

void GoogleProvider::shutdown()
{
	// Stub implementation
	initialized = false;
}
