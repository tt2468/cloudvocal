#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <vector>

#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

#include <curl/curl.h>
#include "presigned_url.h"

AWSTranscribePresignedURL::AWSTranscribePresignedURL(const std::string& access_key, const std::string& secret_key,
                                                     const std::string& session_token, const std::string& region)
    : access_key_(access_key), secret_key_(secret_key), session_token_(session_token), region_(region),
      method_("GET"), service_("transcribe"), canonical_uri_("/stream-transcription-websocket"),
      signed_headers_("host"), algorithm_("AWS4-HMAC-SHA256") {
    endpoint_ = "wss://transcribestreaming." + region_ + ".amazonaws.com:8443";
    host_ = "transcribestreaming." + region_ + ".amazonaws.com";
}

// HMAC SHA-256 function
std::string AWSTranscribePresignedURL::hmacSha256(const std::string &key, const std::string &data, bool isHexKey = false)
{
	unsigned char *digest;
	size_t len = EVP_MAX_MD_SIZE;
	digest = (unsigned char *)calloc(1, len);

	EVP_PKEY *pkey = nullptr;
	if (isHexKey) {
		// Convert hex string to binary data
		std::vector<unsigned char> hexKey;
		for (size_t i = 0; i < key.length(); i += 2) {
			std::string byteString = key.substr(i, 2);
			unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
			hexKey.push_back(byte);
		}
		pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, hexKey.data(), (int)hexKey.size());
	} else {
		pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, (unsigned char *)key.c_str(),
					    (int)key.length());
	}

	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey);
	EVP_DigestSignUpdate(ctx, data.c_str(), data.length());
	EVP_DigestSignFinal(ctx, digest, &len);

	EVP_PKEY_free(pkey);
	EVP_MD_CTX_free(ctx);

	std::stringstream ss;
	for (size_t i = 0; i < len; ++i) {
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
	}
	free(digest);
	return ss.str();
}

std::string AWSTranscribePresignedURL::sha256(const std::string &data)
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

std::string AWSTranscribePresignedURL::getCurrentTimestamp()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::gmtime(&in_time_t), "%Y%m%dT%H%M%SZ");
	return ss.str();
}

std::string AWSTranscribePresignedURL::getCurrentDate()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::gmtime(&in_time_t), "%Y%m%d");
	return ss.str();
}

std::string AWSTranscribePresignedURL::get_request_url(int sample_rate,
                                                       const std::string& language_code,
                                                       const std::string& media_encoding,
                                                       int number_of_channels,
                                                       bool enable_channel_identification) {
	std::string AWS_ACCESS_KEY = access_key_;
	std::string AWS_SECRET_KEY = secret_key_;
	std::string REGION = region_;
	std::string ENDPOINT = endpoint_;

	std::string SERVICE = "transcribe";
	std::string HOST = "transcribestreaming." + REGION + ".amazonaws.com";
	std::string CANONICAL_HEADERS = "host:" + HOST + "\n";
	std::string CANONICAL_URI = "/stream-transcription-websocket";
	std::string DATE = getCurrentDate();
	std::string TIMESTAMP = getCurrentTimestamp();
	std::string PAYLOAD_HASH = sha256("");
	std::string SIGNED_HEADERS = "host";
	std::string METHOD = "GET";
	std::string CREDENTIAL_SCOPE = DATE + "/" + REGION + "/" + SERVICE + "/aws4_request";
	std::string CANONICAL_QUERY_STRING = CreateCanonicalQueryString(TIMESTAMP, CREDENTIAL_SCOPE, "en-US", "pcm", "8000", AWS_ACCESS_KEY);

	std::string CANONICAL_REQUEST = METHOD + "\n" + CANONICAL_URI + "\n" + CANONICAL_QUERY_STRING + "\n" + CANONICAL_HEADERS + "\n" + SIGNED_HEADERS + "\n" + PAYLOAD_HASH;

	std::string HASHED_CANONICAL_REQUEST = sha256(CANONICAL_REQUEST);
	std::string ALGORITHM = "AWS4-HMAC-SHA256";
	std::ostringstream stringToSign;


	stringToSign << ALGORITHM << "\n"
		     << TIMESTAMP << "\n"
		     << CREDENTIAL_SCOPE << "\n"
		     << HASHED_CANONICAL_REQUEST;
	std::string STRING_TO_SIGN = stringToSign.str();

	std::string KEY = "AWS4" + AWS_SECRET_KEY;
	std::string DATE_KEY = hmacSha256(KEY, DATE);
	std::string REGION_KEY = hmacSha256(DATE_KEY, REGION, true);
	std::string SERVICE_KEY = hmacSha256(REGION_KEY, SERVICE, true);
	std::string SIGNING_KEY = hmacSha256(SERVICE_KEY, "aws4_request", true);
	std::string SIGNATURE = hmacSha256(SIGNING_KEY, STRING_TO_SIGN, true);
	CANONICAL_QUERY_STRING += "&X-Amz-Signature=" + SIGNATURE;

	std::ostringstream request_url_temp;
	request_url_temp << ENDPOINT << CANONICAL_URI << "?" << CANONICAL_QUERY_STRING;
	request_url_ = request_url_temp.str();
    return request_url_;
}

std::string AWSTranscribePresignedURL::to_hex(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (unsigned char byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

std::string AWSTranscribePresignedURL::CreateCanonicalQueryString(
    const std::string& dateTimeString,
    const std::string& credentialScope,
    const std::string& languageCode = "en-US",
    const std::string& mediaEncoding = "pcm",
    const std::string& sampleRate = "8000",
	const std::string& accessKey = ""
	)
{
    std::string accessKeyId = accessKey; // Replace with actual access key ID
    std::string credentials = accessKeyId + "/" + credentialScope;
    std::map<std::string, std::string> params = {
        {"X-Amz-Algorithm", "AWS4-HMAC-SHA256"},
        {"X-Amz-Credential", credentials},
        {"X-Amz-Date", dateTimeString},
        {"X-Amz-Expires", "300"},
        {"X-Amz-SignedHeaders", "host"},
		{"enable-channel-identification", "true"},
        {"language-code", languageCode},
        {"media-encoding", mediaEncoding},
		{"number-of-channels", "2"},
        {"sample-rate", sampleRate}
    };
    std::ostringstream result;
    bool first = true;
    for (const auto& param : params) {
        if (!first) {
            result << "&";
        }
        first = false;
        result << UrlEncode(param.first) << "=" << UrlEncode(param.second);
    }
    return result.str();
}

// Helper function for URL encoding
std::string AWSTranscribePresignedURL::UrlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }
    return escaped.str();
}