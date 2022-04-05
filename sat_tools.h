#pragma once

#include <string>
#include <vector>

#define TLE_LIST	"celestrak_legacy.list"

struct tle_list_line_t {
	std::string url;
	std::string filename;
	std::string comment;
};

tle_list_line_t split_tle_list_line(const std::string& str);
bool download_tle_file(const std::wstring& url, const std::wstring& dest);
std::vector<tle_list_line_t> load_tle_file_list(const std::string& filename);