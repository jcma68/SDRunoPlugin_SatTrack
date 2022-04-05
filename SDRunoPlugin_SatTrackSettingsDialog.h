#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/checkbox.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/gui/dragger.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <iunoplugincontroller.h>

#include "sat_tools.h"

// TODO: Change these numbers to the height and width of your form
#define dialogFormWidth (350)
#define dialogFormHeight (260)

class SDRunoPlugin_SatTrackForm;

class SDRunoPlugin_SatTrackSettingsDialog : public nana::form
{

public:

	SDRunoPlugin_SatTrackSettingsDialog(SDRunoPlugin_SatTrackForm& parent, IUnoPluginController& controller);
	~SDRunoPlugin_SatTrackSettingsDialog();

	void Run();

private:

	void Setup();
	int LoadX();
	int LoadY();

	// TODO: Now add your UI controls here

	std::vector<tle_list_line_t> flist{};

	void update_tle_file(const std::string& newFile);
	void update_all_tle_files();
	void update_sat(const std::string& newSat);
	void update_map_size(size_t sz);

	// TODO: Now add your UI controls here
	nana::label lb_path{ *this, nana::rectangle(10, 20, 80, 20) };
	nana::textbox tb_path{ *this, nana::rectangle{ 90, 20, 230, 20 } };
	nana::button btn_path{ *this, nana::rectangle(325, 20, 20, 20) };

	nana::label lb_file{ *this, nana::rectangle(10, 60, 80, 20) };
	nana::combox cb_file{ *this, nana::rectangle{ 90, 60, 120, 20 } };
	nana::label lb_comment{ *this, nana::rectangle(215, 60, 115, 20) };

	nana::label lb_sat{ *this, nana::rectangle(10, 85, 80, 20) };
	nana::combox cb_sat{ *this, nana::rectangle{ 90, 85, 120, 20 } };

	nana::label lb_name{ *this,      nana::rectangle(90, 115, 80, 20) };

	nana::label lb_obs{ *this,      nana::rectangle(10, 130, 80, 20) };
	nana::textbox tb_obs_name{ *this,   nana::rectangle(90, 130, 240, 20) };

	nana::label lb_obs_hlon{ *this, nana::rectangle(90, 155, 80, 20) };
	nana::textbox tb_obs_lon{ *this,   nana::rectangle(90, 170, 70, 20) };

	nana::label lb_obs_hlat{ *this, nana::rectangle(170, 155, 80, 20) };
	nana::textbox tb_obs_lat{ *this,   nana::rectangle(170, 170, 70, 20) };

	nana::label lb_obs_helev{ *this, nana::rectangle(250, 155, 80, 20) };
	nana::textbox tb_obs_elev{ *this,   nana::rectangle(250, 170, 70, 20) };

	nana::label lb_freq{ *this, nana::rectangle(10, 200, 80, 20) };
	nana::textbox tb_freq{ *this,   nana::rectangle(90, 200, 80, 20) };

	nana::label lb_map{ *this, nana::rectangle(10, 225, 80, 20) };
	nana::combox cb_map{ *this, nana::rectangle{ 90, 225, 120, 20 } };

	nana::button btn_update{ *this, nana::rectangle((dialogFormWidth - 100) / 2, 265, 100, 20) };

	SDRunoPlugin_SatTrackForm& m_parent;
	IUnoPluginController & m_controller;
};

