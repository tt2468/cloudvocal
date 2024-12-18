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

#include "utils/ssl-utils.h"
#include "utils/curl-helper.h"

AWSTranscribePresignedURL::AWSTranscribePresignedURL(const std::string &access_key,
						     const std::string &secret_key,
						     const std::string &region)
	: access_key_(access_key),
	  secret_key_(secret_key),
	  region_(region),
	  method_("GET"),
	  service_("transcribe"),
	  canonical_uri_("/stream-transcription-websocket"),
	  signed_headers_("host"),
	  algorithm_("AWS4-HMAC-SHA256")
{
	endpoint_ = "wss://transcribestreaming." + region_ + ".amazonaws.com:8443";
	host_ = "transcribestreaming." + region_ + ".amazonaws.com";
}

std::string AWSTranscribePresignedURL::get_request_url(int sample_rate,
						       const std::string &language_code,
						       const std::string &media_encoding,
						       int number_of_channels,
						       bool enable_channel_identification)
{
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
	std::string CANONICAL_QUERY_STRING = CreateCanonicalQueryString(
		TIMESTAMP, CREDENTIAL_SCOPE, "en-US", "pcm", "8000", AWS_ACCESS_KEY);

	std::string CANONICAL_REQUEST = METHOD + "\n" + CANONICAL_URI + "\n" +
					CANONICAL_QUERY_STRING + "\n" + CANONICAL_HEADERS + "\n" +
					SIGNED_HEADERS + "\n" + PAYLOAD_HASH;

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

std::string AWSTranscribePresignedURL::CreateCanonicalQueryString(
	const std::string &dateTimeString, const std::string &credentialScope,
	const std::string &languageCode = "en-US", const std::string &mediaEncoding = "pcm",
	const std::string &sampleRate = "8000", const std::string &accessKey = "")
{
	std::string accessKeyId = accessKey; // Replace with actual access key ID
	std::string credentials = accessKeyId + "/" + credentialScope;
	std::map<std::string, std::string> params = {{"X-Amz-Algorithm", "AWS4-HMAC-SHA256"},
						     {"X-Amz-Credential", credentials},
						     {"X-Amz-Date", dateTimeString},
						     {"X-Amz-Expires", "300"},
						     {"X-Amz-SignedHeaders", "host"},
						     {"enable-channel-identification", "true"},
						     {"language-code", languageCode},
						     {"media-encoding", mediaEncoding},
						     {"number-of-channels", "2"},
						     {"sample-rate", sampleRate}};
	std::ostringstream result;
	bool first = true;
	for (const auto &param : params) {
		if (!first) {
			result << "&";
		}
		first = false;
		result << CurlHelper::urlEncode(param.first) << "="
		       << CurlHelper::urlEncode(param.second);
	}
	return result.str();
}
