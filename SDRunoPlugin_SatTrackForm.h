#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/gui/dragger.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <iunoplugincontroller.h>

#include "sat_tools.h"
#include "sattrack_widget.h"

// Shouldn't need to change these
#define topBarHeight (27)
#define bottomBarHeight (8)
#define sideBorderWidth (8)

// TODO: Change these numbers to the height and width of your form
#define default_formWidth (500)
#define default_formHeight (250)

class SDRunoPlugin_SatTrackUI;

class SDRunoPlugin_SatTrackForm : public nana::form {


public:
	SDRunoPlugin_SatTrackForm(SDRunoPlugin_SatTrackUI& parent, IUnoPluginController& controller);
	~SDRunoPlugin_SatTrackForm();

	void Run();

	void SetDatasFolder(const std::string& name);

	void SetTLEFile(const std::string& name);
	void SetSatName(const std::string& name);
	void SetLocationName(const std::string& name);
	void SetLatitude(double value);
	void SetLongitude(double value);
	void SetElevation(double value);
	void SetDownlinkFreq(double value);
	void SetModulation(const std::string& value);
	void SetBandwidth(double value);

	void SetMapSize(e_map_type sz);

	std::string GetDatasFolder() const {
		return data_dir_;
	}

	std::string GetMapsFolder() const {
		return maps_dirs_;
	}

	std::string GetTLEFilesFolder() const {
		return tle_files_dirs_;
	}

	std::string GetTLEListName() const {
		return tle_list_;
	}

	std::string GetTLEFile() const;
	std::string GetSatName() const;
	std::string GetLocationName() const;
	double GetLatitude() const;
	double GetLongitude() const;
	double GetElevation() const;
	double GetDownlinkFreq();
	std::string GetModulation();
	double GetBandwidth();
	e_map_type GetMapSize() const;

	void SetFormX(int x) {
		formX_ = x;
	}

	void SetFormY(int y) {
		formY_ = y;
	}

	int GetFormX() const {
		return formX_;
	}

	int GetFormY() const {
		return formY_;
	}

	 void SetSettingsX(int x) {
		settingsX_ = x;
	}

	void SetSettingsY(int y) {
		settingsY_ = y;
	}

	int GetSettingsX() const {
		return settingsX_;
	}

	int GetSettingsY() const {
		return settingsY_;
	}

	void SetPredictX(int x) {
		predictX_ = x;
	}

	void SetPredictY(int y) {
		predictY_ = y;
	}

	int GetPredictX() const {
		return predictX_;
	}

	int GetPredictY() const {
		return predictY_;
	}

	json_utils::json_value& GetSelections();

	void SavePos();
	void LoadPos();

private:

	// settings
	std::string data_dir_{};
	std::string maps_dirs_{};
	std::string tle_files_dirs_{};

	std::string tle_list_{};
	std::string config_file_{};

	json_utils::json_value config_{};

	int formX_{};
	int formY_{};

	int settingsX_{};
	int settingsY_{};
	int predictX_{};
	int predictY_{};

	nana::timer dopplerTimer_;
	void DopplerTick();

	void Setup();
	void LoadSettings();
	void ResizeWindow(e_map_type map_type);

	void SatChanged();

	// The following is to set up the panel graphic to look like a standard SDRuno panel
	nana::picture bg_border{ *this, nana::rectangle(0, 0, default_formWidth, default_formHeight) };
	nana::picture bg_inner{ bg_border, nana::rectangle(sideBorderWidth, topBarHeight, default_formWidth - (2 * sideBorderWidth), default_formHeight - topBarHeight - bottomBarHeight) };
	nana::picture header_bar{ *this, true };
	nana::label title_bar_label{ *this, true };
	nana::dragger form_dragger;
	nana::label form_drag_label{ *this, nana::rectangle(0, 0, default_formWidth, default_formHeight) };
	nana::paint::image img_min_normal;
	nana::paint::image img_min_down;
	nana::paint::image img_close_normal;
	nana::paint::image img_close_down;
	nana::paint::image img_header;
	nana::picture close_button{ *this, nana::rectangle(0, 0, 20, 15) };
	nana::picture min_button{ *this, nana::rectangle(0, 0, 20, 15) };
	nana::label versionLbl{ *this, nana::rectangle(default_formWidth - 40, default_formHeight - 30, 30, 20) };

	// Uncomment the following 5 lines if you want a SETT button and panel
	nana::paint::image img_sett_normal;
	nana::paint::image img_sett_down;
	nana::picture sett_button{ *this, nana::rectangle(0, 0, 40, 15) };
	void SettingsButton_Click();
	void SettingsDialog_Closed();

	// TODO: Now add your UI controls here

	sattrack_widget sattrack_ctrl{ *this, "", {sideBorderWidth, topBarHeight}, e_map_type::small_size};

	SDRunoPlugin_SatTrackUI& m_parent;
	IUnoPluginController& m_controller;
};

