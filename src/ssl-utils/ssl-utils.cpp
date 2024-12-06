#include "ssl-utils.h"

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/err.h>

#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

#include <util/bmem.h>

#include "plugin-support.h"

void init_openssl()
{
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
}

// HMAC SHA-256 function
std::string hmacSha256(const std::string &key, const std::string &data, bool isHexKey)
{
	unsigned char *digest = (unsigned char *)bzalloc(EVP_MAX_MD_SIZE);
	size_t len = EVP_MAX_MD_SIZE;

	// Prepare the key
	std::vector<unsigned char> keyBytes;
	if (isHexKey) {
		for (size_t i = 0; i < key.length(); i += 2) {
			std::string byteString = key.substr(i, 2);
			unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
			keyBytes.push_back(byte);
		}
	} else {
		keyBytes.assign(key.begin(), key.end());
	}

	if (!HMAC(EVP_sha256(), keyBytes.data(), keyBytes.size(), (unsigned char *)data.c_str(),
		  data.length(), digest, (unsigned int *)&len)) {
		obs_log(LOG_ERROR, "hmacSha256 failed during HMAC operation");
		return {};
	}

	std::stringstream ss;
	for (size_t i = 0; i < len; ++i) {
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
	}

	bfree(digest);
	return ss.str();
}

std::string sha256(const std::string &data)
{
	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int lengthOfHash = 0;

	EVP_MD_CTX *context = EVP_MD_CTX_new();

	if (context != nullptr) {
		if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr)) {
			if (EVP_DigestUpdate(context, data.c_str(), data.length())) {
				if (EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
					EVP_MD_CTX_free(context);

					std::stringstream ss;
					for (unsigned int i = 0; i < lengthOfHash; ++i) {
						ss << std::hex << std::setw(2) << std::setfill('0')
						   << (int)hash[i];
					}
					return ss.str();
				}
			}
		}
		EVP_MD_CTX_free(context);
	}

	return "";
}

std::string getCurrentTimestamp()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::gmtime(&in_time_t), "%Y%m%dT%H%M%SZ");
	return ss.str();
}

std::string getCurrentDate()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::gmtime(&in_time_t), "%Y%m%d");
	return ss.str();
}
