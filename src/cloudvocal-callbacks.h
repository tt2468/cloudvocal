#pragma once

#include <string>

#include "cloudvocal-data.h"

enum DetectionResult {
	DETECTION_RESULT_UNKNOWN = 0,
	DETECTION_RESULT_SPEECH = 1,
	DETECTION_RESULT_SILENCE = 2,
	DETECTION_RESULT_PARTIAL = 3
};

struct DetectionResultWithText {
	uint64_t start_timestamp_ms;
	uint64_t end_timestamp_ms;
	std::string text;
	std::string language;
	enum DetectionResult result;
};

void send_caption_to_source(const std::string &target_source_name, const std::string &str_copy,
			    struct transcription_filter_data *gf);
std::string send_sentence_to_translation(const std::string &sentence,
					 struct transcription_filter_data *gf);

void audio_chunk_callback(struct transcription_filter_data *gf, const float *pcm32f_data,
			  size_t frames, int vad_state, const DetectionResultWithText &result);

void set_text_callback(struct transcription_filter_data *gf,
		       const DetectionResultWithText &resultIn);

void clear_current_caption(transcription_filter_data *gf_);

void recording_state_callback(enum obs_frontend_event event, void *data);

void media_play_callback(void *data_, calldata_t *cd);
void media_started_callback(void *data_, calldata_t *cd);
void media_pause_callback(void *data_, calldata_t *cd);
void media_restart_callback(void *data_, calldata_t *cd);
void media_stopped_callback(void *data_, calldata_t *cd);
void enable_callback(void *data_, calldata_t *cd);
