#include "eventstream.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

// Platform-independent byte swapping functions
inline uint16_t swap_uint16(uint16_t val)
{
	return (val << 8) | (val >> 8);
}

inline uint32_t swap_uint32(uint32_t val)
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}

// Use these instead of ntohl and htonl
#define ntohl(x) swap_uint32(x)
#define htonl(x) swap_uint32(x)
#define ntohs(x) swap_uint16(x)
#define htons(x) swap_uint16(x)

std::string bytes_to_hex(const std::vector<uint8_t> &bytes)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (const auto &byte : bytes) {
		ss << std::setw(2) << static_cast<int>(byte);
	}
	return ss.str();
}

EventData decode_event(const std::vector<uint8_t> &message)
{
	if (message.size() < 16) {
		throw std::runtime_error("Message too short");
	}
	// Extract the prelude, headers, payload and CRC
	uint32_t total_length, headers_length;
	std::memcpy(&total_length, message.data(), 4);
	std::memcpy(&headers_length, message.data() + 4, 4);
	total_length = ntohl(total_length);
	headers_length = ntohl(headers_length);
	uint32_t prelude_crc;
	std::memcpy(&prelude_crc, message.data() + 8, 4);
	prelude_crc = ntohl(prelude_crc);
	std::vector<uint8_t> headers(message.begin() + 12, message.begin() + 12 + headers_length);
	std::vector<uint8_t> payload(message.begin() + 12 + headers_length, message.end() - 4);
	uint32_t message_crc;
	std::memcpy(&message_crc, message.data() + message.size() - 4, 4);
	message_crc = ntohl(message_crc);

	// Check the CRCs
	uint32_t calculated_prelude_crc = crc32(0, message.data(), 8) & 0xffffffff;
	if (prelude_crc != calculated_prelude_crc) {
		throw std::runtime_error("Prelude CRC check failed");
	}

	uint32_t calculated_message_crc = crc32(0, message.data(), message.size() - 4) & 0xffffffff;
	if (message_crc != calculated_message_crc) {
		throw std::runtime_error("Message CRC check failed");
	}

	// Parse the headers
	std::unordered_map<std::string, std::string> headers_dict;
	size_t offset = 0;
	while (offset < headers.size()) {
		uint8_t name_len = headers[offset++];
		std::string name(headers.begin() + offset, headers.begin() + offset + name_len);
		offset += name_len;
		uint8_t value_type = headers[offset++];
		uint16_t value_len;
		std::memcpy(&value_len, headers.data() + offset, 2);
		value_len = ntohs(value_len);
		offset += 2;
		std::string value(headers.begin() + offset, headers.begin() + offset + value_len);
		offset += value_len;
		headers_dict[name] = value;
	}

	/*
    std::cout << "Headers:" << std::endl;
    for (const auto& pair : headers_dict) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    */

	return {headers_dict, json::parse(payload.begin(), payload.end())};
}

std::vector<uint8_t> create_audio_event(const std::vector<uint8_t> &payload)
{
	// Build our headers
	auto contentTypeHeader = get_headers(":content-type", "application/octet-stream");
	auto eventTypeHeader = get_headers(":event-type", "AudioEvent");
	auto messageTypeHeader = get_headers(":message-type", "event");
	std::vector<uint8_t> headers;
	headers.insert(headers.end(), contentTypeHeader.begin(), contentTypeHeader.end());
	headers.insert(headers.end(), eventTypeHeader.begin(), eventTypeHeader.end());
	headers.insert(headers.end(), messageTypeHeader.begin(), messageTypeHeader.end());

	// Calculate total byte length and headers byte length
	uint32_t totalByteLength = headers.size() + payload.size() +
				   16; // 16 accounts for 8 byte prelude, 2x 4 byte crcs.
	uint32_t headersByteLength = headers.size();

	uint32_t totalByteLengthNetworkOrder = htonl(totalByteLength);
	uint32_t headersByteLengthNetworkOrder = htonl(headersByteLength);

	// Build the prelude
	std::vector<uint8_t> prelude(8);
	std::memcpy(prelude.data(), &totalByteLengthNetworkOrder, 4);
	std::memcpy(prelude.data() + 4, &headersByteLengthNetworkOrder, 4);

	// Calculate checksum for prelude
	uint32_t preludeCRC = crc32(0, prelude.data(), prelude.size()) & 0xffffffff;
	uint32_t preludeCRCNetworkOrder = htonl(preludeCRC);

	// Construct the message
	std::vector<uint8_t> message;
	message.insert(message.end(), prelude.begin(), prelude.end());
	message.insert(message.end(), reinterpret_cast<uint8_t *>(&preludeCRCNetworkOrder),
		       reinterpret_cast<uint8_t *>(&preludeCRCNetworkOrder) + 4);
	message.insert(message.end(), headers.begin(), headers.end());
	message.insert(message.end(), payload.begin(), payload.end());

	// Calculate checksum for message
	uint32_t messageCRC = crc32(0, message.data(), message.size()) & 0xffffffff;
	uint32_t messageCRCNetworkOrder = htonl(messageCRC);

	// Add message checksum
	message.insert(message.end(), reinterpret_cast<uint8_t *>(&messageCRCNetworkOrder),
		       reinterpret_cast<uint8_t *>(&messageCRCNetworkOrder) + 4);
	return message;
}

std::vector<uint8_t> get_headers(const std::string &headerName, const std::string &headerValue)
{
	std::vector<uint8_t> headerList;
	uint8_t nameByteLength = headerName.length();
	headerList.push_back(nameByteLength);
	headerList.insert(headerList.end(), headerName.begin(), headerName.end());
	uint8_t valueType = 7; // 7 represents a string
	headerList.push_back(valueType);
	uint16_t valueByteLength = htons(headerValue.length());
	headerList.insert(headerList.end(), reinterpret_cast<uint8_t *>(&valueByteLength),
			  reinterpret_cast<uint8_t *>(&valueByteLength) + 2);
	headerList.insert(headerList.end(), headerValue.begin(), headerValue.end());
	return headerList;
}