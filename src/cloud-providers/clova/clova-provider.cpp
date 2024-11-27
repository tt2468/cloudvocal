#include "clova-provider.h"
#include <iostream>

void ClovaProvider::init()
{
	// Initialize the Clova provider
	std::cout << "Initializing Clova Provider" << std::endl;
	// Add your initialization code here
}

void ClovaProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	// Send the audio buffer to Clova for transcription
	std::cout << "Sending audio buffer to Clova for transcription" << std::endl;
	// Add your code to send the audio buffer to Clova's transcription service here
}
