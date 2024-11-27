#pragma once

#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <obs-module.h>
#include <media-io/audio-resampler.h>

#include "cloud-translation/translation-cloud.h"

#define TRANSCRIPTION_SAMPLE_RATE 16000

// Audio packet info
struct cloudvocal_audio_info {
	uint32_t frames;
	uint64_t timestamp_offset_ns; // offset (since start of processing) timestamp in ns
};

struct cloudvocal_data {
	int log_level;
	bool active;
	bool initial_creation;
	bool source_signals_set;
	obs_source_t *context;

	size_t channels;
	int sample_rate;
	std::deque<float> input_buffers[8];
	std::deque<cloudvocal_audio_info> info_buffer;
	std::deque<float> resampled_buffer;
	uint32_t last_num_frames;

	// File output options
	bool save_only_while_recording;
	bool truncate_output_file;
	bool save_srt;
	bool save_to_file;
	std::string output_file_path;
	uint64_t sentence_number;
	uint64_t start_timestamp_ms;
	bool rename_file_to_match_recording;

	// Transcription options
	std::string language;
	std::string text_source_name;
	bool caption_to_stream;
	bool process_while_muted;
	uint64_t last_sub_render_time;
	bool cleared_last_sub;
	std::string last_transcription_sentence;
	audio_resampler_t *resampler;
	int min_sub_duration;
	int max_sub_duration;
	bool log_words;

	std::map<std::string, std::string> filter_words_replace;

	// Translation options
	bool translate_only_full_sentences;
	bool translate;
	std::string translation_output;
	std::string target_lang;
	std::string last_text_for_translation;
	std::string last_text_translation;
	CloudTranslatorConfig translate_cloud_config;

	std::mutex input_buffers_mutex;
	std::condition_variable input_buffers_cv;
};
