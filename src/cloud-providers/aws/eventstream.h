#ifndef EVENTSTREAM_H
#define EVENTSTREAM_H
#include <vector>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <zlib.h>

using json = nlohmann::json;
struct EventData {
    std::unordered_map<std::string, std::string> headers;
    json payload;
};

// Platform-independent byte swapping functions
uint16_t swap_uint16(uint16_t val);
uint32_t swap_uint32(uint32_t val);

// Decodes an event message
EventData decode_event(const std::vector<uint8_t>& message);

// Creates an audio event message
//std::vector<uint8_t> create_audio_event(const std::vector<uint8_t>& payload);
std::vector<uint8_t> create_audio_event(const std::vector<uint8_t>& payload);

// Generates headers for the audio event
std::vector<uint8_t> get_headers(const std::string& headerName, const std::string& headerValue);

#endif // AWS_TRANSCRIBE_AUDIO_FRAME_HPP