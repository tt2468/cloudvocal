#ifndef LANGUAGE_CODES_H
#define LANGUAGE_CODES_H

#include <map>
#include <string>

extern std::map<std::string, std::string> language_codes;
extern std::map<std::string, std::string> language_codes_reverse;
extern std::map<std::string, std::string> language_codes_to_underscore;
extern std::map<std::string, std::string> language_codes_from_underscore;
extern std::map<std::string, std::string> language_codes_to_locale;

inline bool isLanguageSupported(const std::string &lang_code)
{
	return language_codes.find(lang_code) != language_codes.end() ||
	       language_codes_to_underscore.find(lang_code) != language_codes_to_underscore.end();
}

inline std::string getLanguageName(const std::string &lang_code)
{
	auto it = language_codes.find(lang_code);
	if (it != language_codes.end()) {
		return it->second;
	}
	// check if it's a underscore language code
	it = language_codes_to_underscore.find(lang_code);
	if (it != language_codes_to_underscore.end()) {
		// convert to the language code
		const std::string &underscore_code = it->second;
		it = language_codes.find(underscore_code);
		if (it != language_codes.end()) {
			return it->second;
		}
	}
	return lang_code; // Return the code itself if no mapping exists
}

inline std::string getLanguageLocale(const std::string &lang_code)
{
	auto it = language_codes_to_locale.find(lang_code);
	if (it != language_codes_to_locale.end()) {
		return it->second;
	}
	// check if it's a underscore language code
	it = language_codes_from_underscore.find(lang_code);
	if (it != language_codes_from_underscore.end()) {
		// convert to the language code
		const std::string &language_code = it->second;
		it = language_codes_to_locale.find(language_code);
		if (it != language_codes_to_locale.end()) {
			return it->second;
		}
	}
	return lang_code; // Return the code itself if no mapping exists
}

#endif // LANGUAGE_CODES_H
