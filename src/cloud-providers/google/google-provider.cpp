#include "google-provider.h"
#include "language-codes/language-codes.h"

using namespace google::cloud::speech::v1;

bool GoogleProvider::init()
{
	initialized = false;

	grpc::SslCredentialsOptions ssl_opts;
	this->channel =
		grpc::CreateChannel("speech.googleapis.com", grpc::SslCredentials(ssl_opts));
	this->stub = Speech::NewStub(channel);
	this->context.AddMetadata("x-goog-api-key", gf->cloud_provider_api_key);
	this->reader_writer = this->stub->StreamingRecognize(&context);
	if (!reader_writer) {
		obs_log(LOG_ERROR, "Failed to create reader writer for Google");
		return false;
	}

	// Send the config request
	obs_log(gf->log_level, "Sending config request to Google");
	StreamingRecognizeRequest config_request;
	StreamingRecognitionConfig *streaming_config = config_request.mutable_streaming_config();
	RecognitionConfig *config = streaming_config->mutable_config();
	config->set_language_code(getLanguageLocale(gf->language));
	config->set_sample_rate_hertz(16000);
	config->set_audio_channel_count(1);
	config->set_encoding(RecognitionConfig_AudioEncoding_LINEAR16);
	streaming_config->set_single_utterance(false);
	streaming_config->set_interim_results(true);
	if (!reader_writer->Write(config_request)) {
		obs_log(LOG_ERROR, "Failed to send config request to Google");
		return false;
	}
	obs_log(gf->log_level, "Config request sent to Google");

	initialized = true;
	return initialized;
}

void GoogleProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	if (!reader_writer) {
		obs_log(LOG_ERROR, "Reader writer is not initialized");
		return;
	}
	if (audio_buffer.empty()) {
		obs_log(LOG_WARNING, "Audio buffer is empty");
		return;
	}

	// Send the audio buffer to Clova for transcription
	obs_log(gf->log_level,
		"Sending audio buffer (%d) to Google for transcription. Chunk ID %llu",
		audio_buffer.size(), chunk_id);

	// convert from float [-1,1] to int16
	std::vector<int16_t> audio_buffer_int16(audio_buffer.size());
	std::transform(audio_buffer.begin(), audio_buffer.end(), audio_buffer_int16.begin(),
		       [](float f) { return static_cast<int16_t>(f * 32767); });

	StreamingRecognizeRequest request;
	request.set_audio_content(reinterpret_cast<const char *>(audio_buffer_int16.data()),
				  audio_buffer_int16.size() * sizeof(int16_t));

	try {
		bool success = reader_writer->Write(request);
		if (!success) {
			obs_log(LOG_ERROR, "Failed to send data request to Google");
			this->stop_requested = true;
			return;
		}
		chunk_id++;
	} catch (...) {
		obs_log(LOG_ERROR, "Exception caught while sending data request to Google");
	}
}

void GoogleProvider::readResultsFromTranscription()
{
	if (!reader_writer || !initialized) {
		return;
	}
	StreamingRecognizeResponse response;
	if (reader_writer->Read(&response)) {
		if (response.has_error()) {
			obs_log(LOG_ERROR, "Google response Error: %s",
				response.error().message().c_str());
			return;
		}

		std::string overall_transcript;
		bool is_final = false;

		for (int i = 0; i < response.results_size(); i++) {
			const StreamingRecognitionResult &result = response.results(i);
			obs_log(gf->log_level,
				"Google Result %d. stability %.3f. is_final %d. duration %d ns", i,
				result.stability(), result.is_final(),
				result.result_end_time().nanos());
			if (!result.is_final() && result.stability() < 0.5) {
				obs_log(gf->log_level, "Google Result %d. Stability too low", i);
				continue;
			}
			if (result.alternatives_size() == 0) {
				obs_log(gf->log_level, "Google Result %d. No alternatives", i);
				continue;
			}
			const SpeechRecognitionAlternative &alternative = result.alternatives(0);
			std::string transcript = alternative.transcript();
			obs_log(gf->log_level, "Google Transcription: '%s'", transcript.c_str());
			overall_transcript += transcript;
			is_final = result.is_final();
		}

		DetectionResultWithText result;
		result.text = overall_transcript;
		result.result = DETECTION_RESULT_SPEECH;
		result.language = language_codes_from_underscore[gf->language];
		this->transcription_callback(result);
	}
}

void GoogleProvider::shutdown()
{
	// Shutdown the Clova provider
	obs_log(gf->log_level, "Shutting down Google provider");
	if (reader_writer && initialized) {
		reader_writer->WritesDone();
	}
	initialized = false;
}
