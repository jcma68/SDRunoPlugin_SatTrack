#include "sat_tools.h"

#include <fstream>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>

#pragma comment(lib, "Urlmon.lib")

#endif


#ifdef _WIN32
bool download_tle_file(const std::wstring& url, const std::wstring& dest) {
	return (S_OK == URLDownloadToFile(NULL, url.c_str(), dest.c_str(), 0, NULL));
}
#endif

void create_default_config(const std::string& filename) {
	using namespace json_utils;

	json_value opt_list;

	json_value current;
	current.add_pair("name", "NOAA 15");
	current.add_pair("tle_file", "weather.txt");
	current.add_pair("comment", "Weather");
	current.add_pair("map_size", 0);

	opt_list.add_pair("current", current);

	json_value loc;
	loc.add_pair("name", "Greenwich");
	loc.add_pair("latitude", 51.482578);
	loc.add_pair("longitude", -0.007659);
	loc.add_pair("elevation", 6.09);

	opt_list.add_pair("location", loc);

	json_value satellites;

	json_value sat;
	sat.add_pair("modulation", "FM");
	sat.add_pair("downlink", 137.62);
	sat.add_pair("mode", "APT");
	sat.add_pair("bandwidth", 38000.0);
	sat.add_pair("bauds", 1700.0);
	satellites.add_pair("NOAA 15", sat);

	sat["downlink"] = 137.9125;
	satellites.add_pair("NOAA 18", sat);

	sat["downlink"] = 137.1;
	satellites.add_pair("NOAA 19", sat);

	sat["downlink"] = 137.1;
	sat["mode"] = "LRPT";
	sat["bandwidth"] = 150000.0;
	sat["bauds"] = 80000;
	satellites.add_pair("METEOR-M 2", sat);

	opt_list.add_pair("satellites", satellites);

	json_value filter;
	filter.add_pair("sat_elevation", json_utils::json_value{ 0.0 });

	opt_list.add_pair("selections", filter);

	opt_list.save_to(filename);
}

void create_sat_entry(json_utils::json_value& satellites, const std::string& name) {
	using namespace json_utils;

	json_value sat;
	sat.add_pair("modulation", "FM");
	sat.add_pair("downlink", 137.1);
	sat.add_pair("mode", "APT");
	sat.add_pair("bandwidth", 38000.0);
	sat.add_pair("bauds", 1700.0);
	satellites.add_pair(name, sat);
}
