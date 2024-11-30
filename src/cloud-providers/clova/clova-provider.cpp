#include "clova-provider.h"
#include <iostream>

#include "plugin-support.h"

#include <grpcpp/grpcpp.h>
#include "nlohmann/json.hpp"

#include "cloud-providers/clova/nest.grpc.pb.h"
#include "language-codes/language-codes.h"

using grpc::Status;

using RequestType = com::nbp::cdncp::nest::grpc::proto::v1::RequestType;
using json = nlohmann::json;

bool ClovaProvider::init()
{
	// Initialize the Clova provider
	obs_log(gf->log_level, "Initializing Clova provider");
	// Add your initialization code here
	chunk_id = 1;
	initialized = false;

	grpc::SslCredentialsOptions ssl_opts;
	this->channel = grpc::CreateChannel("clovaspeech-gw.ncloud.com:50051",
					    grpc::SslCredentials(ssl_opts));
	this->stub = NestService::NewStub(channel);
	context.AddMetadata("authorization", "Bearer " + gf->cloud_provider_api_key);
	reader_writer = this->stub->recognize(&context);
	if (!reader_writer) {
		obs_log(LOG_ERROR, "Failed to create reader writer for Clova");
		return false;
	}

	// check channel state
	grpc_connectivity_state state = this->channel->GetState(true);
	if (state != GRPC_CHANNEL_READY) {
		obs_log(LOG_ERROR, "Clova Channel is not ready");
		return false;
	}

	json config_payload = {
		{"transcription", {{"language", language_codes_from_underscore[gf->language]}}},
	};

	// Send the config request to Clova
	obs_log(gf->log_level, "Sending config request to Clova: %s",
		config_payload.dump().c_str());
	NestRequest config_request;
	config_request.set_type(RequestType::CONFIG);
	config_request.mutable_config()->set_config(config_payload.dump());
	if (!reader_writer->Write(config_request)) {
		obs_log(LOG_ERROR, "Failed to send config request to Clova");
		return false;
	}
	obs_log(gf->log_level, "Config request sent to Clova");

	initialized = true;

	return true;
}

void ClovaProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
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
		"Sending audio buffer (%d) to Clova for transcription. Chunk ID %llu",
		audio_buffer.size(), chunk_id);

	chunk_start_times[chunk_id] = std::chrono::steady_clock::now();
	NestRequest data_request;
	data_request.set_type(RequestType::DATA);

	// convert from float [-1,1] to int16
	std::vector<int16_t> audio_buffer_int16(audio_buffer.size());
	std::transform(audio_buffer.begin(), audio_buffer.end(), audio_buffer_int16.begin(),
		       [](float f) { return static_cast<int16_t>(f * 32767); });

	data_request.mutable_data()->set_chunk(
		reinterpret_cast<uint8_t *>(audio_buffer_int16.data()),
		audio_buffer_int16.size() * sizeof(int16_t));
	data_request.mutable_data()->set_extra_contents("{\"seqId\": " + std::to_string(chunk_id) +
							", \"epFlag\": false}");

	try {
		bool success = reader_writer->Write(data_request);
		if (!success) {
			obs_log(LOG_ERROR, "Failed to send data request to Clova");
			// get error status
			Status status = reader_writer->Finish();
			if (!status.ok()) {
				obs_log(LOG_ERROR, "Clova Error status: %s",
					status.error_message().c_str());
				this->running = false;
			}
			return;
		}
		chunk_id++;
	} catch (...) {
		obs_log(LOG_ERROR, "Exception caught while sending data request to Clova");
	}
}

void ClovaProvider::readResultsFromTranscription()
{
	if (!reader_writer) {
		return;
	}

	// Read the response from Clova
	NestResponse response;
	while (reader_writer->Read(&response)) {
		auto end_time = std::chrono::steady_clock::now();
		json json_response_data = json::parse(response.contents());
		if (json_response_data.contains("transcription") &&
		    json_response_data["transcription"].contains("text")) {
			std::string text_value = json_response_data["transcription"]["text"];
			int seq_id = json_response_data["transcription"].value("seqId", -1);
			// log the transcription
			obs_log(gf->log_level, "Transcription (seq_id %d): '%s'\n%s", seq_id,
				text_value.c_str(), response.contents().c_str());

			if (text_value.empty() && !this->current_sentence.empty()) {
				DetectionResultWithText result;
				result.text = this->current_sentence;
				result.result = DETECTION_RESULT_SPEECH;
				result.language = language_codes_from_underscore[gf->language];
				this->transcription_callback(result);
				this->current_sentence.clear();
			} else {
				if (!text_value.empty()) {
					this->current_sentence += text_value;
					DetectionResultWithText result;
					result.text = this->current_sentence;
					result.result = DETECTION_RESULT_PARTIAL;
					result.language =
						language_codes_from_underscore[gf->language];
					this->transcription_callback(result);
				}
			}
		} else {
			// check if this is a config response
			if (json_response_data.contains("config") &&
			    json_response_data["config"].contains("status")) {
				std::string status = json_response_data["config"]["status"];
				if (status == "Success") {
					obs_log(gf->log_level,
						"Config response received from Clova");
				} else {
					obs_log(LOG_ERROR, "Config response failed: %s",
						json_response_data.dump().c_str());
				}
			} else {
				obs_log(LOG_ERROR, "No transcription found in the response: %s",
					response.contents().c_str());
			}
		}
	}
}

void ClovaProvider::shutdown()
{
	// Shutdown the Clova provider
	obs_log(gf->log_level, "Shutting down Clova provider");
	if (reader_writer && initialized) {
		reader_writer->WritesDone();
	}
}
