#pragma once

#include "cloudvocal-data.h"

/**
 * @brief Extracts audio data from the buffer, resamples it, and updates timestamp offsets.
 *
 * This function extracts audio data from the input buffer, resamples it to 16kHz, and updates
 * gf->resampled_buffer with the resampled data.
 *
 * @param gf Pointer to the transcription filter data structure.
 * @param start_timestamp_offset_ns Reference to the start timestamp offset in nanoseconds.
 * @param end_timestamp_offset_ns Reference to the end timestamp offset in nanoseconds.
 * @return Returns 0 on success, 1 if the input buffer is empty.
 */
int get_data_from_buf_and_resample(cloudvocal_data *gf, uint64_t &start_timestamp_offset_ns,
				   uint64_t &end_timestamp_offset_ns);
