#pragma once

#include <string>
#include <vector>

#include "json_parser.h"

#define TLE_LIST	"celestrak_legacy.json"
#define CONFIG_FILE	"satrack_config.json"

struct tle_list_line_t {
	std::string url;
	std::string filename;
	std::string comment;
};

bool download_tle_file(const std::wstring& url, const std::wstring& dest);
void create_default_config(const std::string& filename);
void create_sat_entry(json_utils::json_value& satellites, const std::string& name);