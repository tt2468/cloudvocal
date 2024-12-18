
#include "aws_provider.h"

#include <aws/core/Aws.h>
#include <aws/core/utils/threading/Semaphore.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/transcribestreaming/TranscribeStreamingServiceClient.h>
#include <aws/transcribestreaming/model/StartStreamTranscriptionHandler.h>
#include <aws/transcribestreaming/model/StartStreamTranscriptionRequest.h>
#include <aws/core/platform/FileSystem.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <array>

#include "utils/ssl-utils.h"

using namespace Aws;
using namespace Aws::TranscribeStreamingService;
using namespace Aws::TranscribeStreamingService::Model;

static const int SAMPLE_RATE = 16000; // for the file above

bool AWSProvider::init()
{
	if (this->stop_requested) {
		obs_log(LOG_INFO, "AWS Init: AWS Provider stop requested.");
		return false;
	}

	obs_log(LOG_INFO, "Initializing AWS provider");

	if (gf->cloud_provider_api_key.empty() || gf->cloud_provider_secret_key.empty()) {
		obs_log(LOG_ERROR, "AWS provider requires API key and secret key");
		return false;
	}

	obs_log(LOG_INFO, "AWS Provider Initializing...");

	Aws::SDKOptions options;
	Aws::InitAPI(options);

	Aws::Client::ClientConfiguration config;
#ifdef _WIN32
	config.caFile = PEMrootCertsPath();
#endif
	config.region = Aws::Region::US_EAST_1;
	config.httpLibOverride = Aws::Http::TransferLibType::WIN_INET_CLIENT;

	// set credentials
	Aws::Auth::AWSCredentials credentials(gf->cloud_provider_api_key,
					      gf->cloud_provider_secret_key);

	this->client.reset(new TranscribeStreamingServiceClient(credentials, config));

	this->handler.reset(new StartStreamTranscriptionHandler());
	handler->SetOnErrorCallback(
		[this](const Aws::Client::AWSError<TranscribeStreamingServiceErrors> &error) {
			obs_log(LOG_ERROR,
				"Start Stream Transcription ERROR (req ID '%s'): %s [Error code: %s]",
				error.GetRequestId().c_str(), error.GetMessage().c_str(),
				error.GetExceptionName().c_str());
			this->stop();
		});
	handler->SetTranscriptEventCallback([](const TranscriptEvent &ev) {
		for (auto &&r : ev.GetTranscript().GetResults()) {
			std::string output = "";
			if (r.GetIsPartial()) {
				output += "[partial] ";
			} else {
				output += "[Final] ";
			}
			for (auto &&alt : r.GetAlternatives()) {
				output += alt.GetTranscript();
			}
			obs_log(LOG_INFO, "Transcript: %s", output.c_str());
		}
	});

	this->request.reset(new StartStreamTranscriptionRequest());
	this->request->SetMediaSampleRateHertz(SAMPLE_RATE);
	this->request->SetLanguageCode(Aws::TranscribeStreamingService::Model::LanguageCode::en_US);
	// Aws::TranscribeStreamingService::Model::LanguageCodeMapper::GetLanguageCodeForName(
	// 	gf->language));
	this->request->SetMediaEncoding(MediaEncoding::pcm); // wav and aiff files are PCM formats.
	this->request->SetEventStreamHandler(*(handler.get()));

	auto OnStreamReady = [this](AudioStream &stream) {
		if (!stream) {
			obs_log(LOG_ERROR, "Failed to create a stream");
			this->stream_open = false;
			return;
		}
		obs_log(LOG_INFO, "AWS Provider Stream Ready...");
		this->stream_open = true;
		while (!this->stop_requested) {
			obs_log(LOG_INFO,
				"AWS Provider Stream Loop. Wait for audio buffer mutex...");
			// wait for a signal on the condition variable
			std::unique_lock<std::mutex> lock(this->audio_buffer_queue_mutex);
			this->audio_buffer_queue_cv.wait_for(
				lock, std::chrono::milliseconds(100), [this] {
					return !this->audio_buffer_queue.empty() ||
					       this->stop_requested;
				});
			// if we're stopping, break out of the loop
			if (this->stop_requested) {
				obs_log(LOG_INFO, "AWS Provider Stream stop requested.");
				lock.unlock();
				break;
			}
			if (this->audio_buffer_queue.empty()) {
				lock.unlock();
				continue;
			}
			// get the audio buffer
			std::vector<float> audio_buffer = this->audio_buffer_queue.front();
			this->audio_buffer_queue.pop();
			lock.unlock();

			// convert the audio buffer to a byte array
			std::vector<uint8_t> audio_chunk;
			audio_chunk.reserve(audio_buffer.size() * sizeof(int16_t));
			for (auto sample : audio_buffer) {
				int16_t sample_int = static_cast<int16_t>(sample * 32767);
				audio_chunk.push_back(sample_int & 0xFF);
				audio_chunk.push_back((sample_int >> 8) & 0xFF);
			}

			// write the audio chunk to the stream
			Aws::Vector<unsigned char> bits{audio_chunk.begin(), audio_chunk.end()};
			AudioEvent event(std::move(bits));
			if (!stream.WriteAudioEvent(event)) {
				obs_log(LOG_ERROR, "Failed to write an audio event");
				break;
			}
		}

		obs_log(LOG_INFO, "AWS Provider Stream Stopping...");
		// close the stream
		if (!stream.WriteAudioEvent(AudioEvent())) {
			obs_log(LOG_ERROR, "Failed to send an empty audio event");
		}
		stream.Close();
		this->stream_open = false;
	};

	// Aws::Utils::Threading::Semaphore signaling(0 /*initialCount*/, 1 /*maxCount*/);
	auto OnResponseCallback =
		[](const TranscribeStreamingServiceClient * /*unused*/,
		   const Model::StartStreamTranscriptionRequest & /*unused*/,
		   const Model::StartStreamTranscriptionOutcome &outcome,
		   const std::shared_ptr<const Aws::Client::AsyncCallerContext> & /*unused*/) {
			if (!outcome.IsSuccess()) {
				obs_log(LOG_ERROR, "Transcribe streaming error: %s",
					outcome.GetError().GetMessage().c_str());
			}
		};

	obs_log(LOG_INFO, "AWS Provider Starting...");
	client->StartStreamTranscriptionAsync(*(request.get()), OnStreamReady, OnResponseCallback,
					      nullptr /*context*/);

	obs_log(LOG_INFO, "AWS provider initialized");
	return true;
}

void AWSProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	if (this->stop_requested || !this->stream_open) {
		return;
	}
	// queue up the audio buffer
	std::lock_guard<std::mutex> lock(audio_buffer_queue_mutex);
	audio_buffer_queue.push(std::vector<float>(audio_buffer.begin(), audio_buffer.end()));
	audio_buffer_queue_cv.notify_one();
}

void AWSProvider::readResultsFromTranscription() {}

void AWSProvider::shutdown()
{
	obs_log(LOG_INFO, "AWS Provider Shutting Down...");
	audio_buffer_queue_cv.notify_all();
	// wait until the stream is closed
	while (this->stream_open) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	request.reset();
	client.reset();
	Aws::SDKOptions options;
	Aws::ShutdownAPI(options);
	obs_log(LOG_INFO, "AWS provider shutdown.");
}
