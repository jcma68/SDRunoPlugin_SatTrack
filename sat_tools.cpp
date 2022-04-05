#include "sat_tools.h"

#include <fstream>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>

#pragma comment(lib, "Urlmon.lib")

#endif

static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}

static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	 }).base(), s.end());
}

static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

tle_list_line_t split_tle_list_line(const std::string& str) {
	tle_list_line_t res{};

	std::size_t bar = str.rfind('|');
	if (bar == std::string::npos)
		return res; // malformed

	res.url = str.substr(0, bar);
	trim(res.url);

	res.comment = str.substr(bar + 1);
	trim(res.comment);

	std::size_t slash = res.url.rfind('/');
	if (slash != std::string::npos) {
		res.filename = res.url.substr(slash + 1);
		trim(res.filename);
	}

	return res;
}

#ifdef _WIN32
bool download_tle_file(const std::wstring& url, const std::wstring& dest) {
	return (S_OK == URLDownloadToFile(NULL, url.c_str(), dest.c_str(), 0, NULL));
}
#endif

std::vector<tle_list_line_t> load_tle_file_list(const std::string& filename) {
	std::vector<tle_list_line_t>  list;

	std::ifstream in(filename.c_str());
	if (!in) {
		return list;
	}

	std::string str;
	while (true) {
		if (!std::getline(in, str))
			break;

		rtrim(str);
		if (str.size() == 0)
			continue;

		tle_list_line_t l = split_tle_list_line(str);
		list.push_back(l);
	}

	in.close();

	return list;
}

