#include <sstream>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "SDRunoPlugin_SatTrackSettingsDialog.h"
#include "SDRunoPlugin_SatTrackForm.h"
#include "PredictDialog.h"
#include "resource.h"
#include <io.h>
#include <shlobj.h>

// Form constructor with handles to parent and uno controller - launches form SatTrackForm
SDRunoPlugin_SatTrackSettingsDialog::SDRunoPlugin_SatTrackSettingsDialog(SDRunoPlugin_SatTrackForm& parent, IUnoPluginController& controller) :
	nana::form(nana::API::make_center(dialogFormWidth, dialogFormHeight), nana::appearance(true, false, true, false, false, false, false)),
	m_parent(parent),
	m_controller(controller) {
	Setup();
}

// Form deconstructor
SDRunoPlugin_SatTrackSettingsDialog::~SDRunoPlugin_SatTrackSettingsDialog() {
	// **This Should not be necessary, but just in case - we are going to remove all event handlers
	// previously assigned to the "destroy" event to avoid memory leaks;
	this->events().destroy.clear();
}

// Start Form and start Nana UI processing
void SDRunoPlugin_SatTrackSettingsDialog::Run() {
	show();
	nana::exec();
}

// Create the settings dialog form
void SDRunoPlugin_SatTrackSettingsDialog::Setup() {
	// TODO: Form code starts here

	// Load X and Y locations for the dialog from the ini file (if exists)
	move(m_parent.GetSettingsX(), m_parent.GetSettingsY());
	events().move([&](const nana::arg_move& mov) {
		m_parent.SetSettingsX(mov.x);
		m_parent.SetSettingsY(mov.y);
				  });


	// This code sets the plugin size and title
	size(nana::size(dialogFormWidth, dialogFormHeight));
	caption("SDRuno SatTrack Plugin - Settings");

	// Set the forms back color to black to match SDRuno's settings dialogs
	this->bgcolor(nana::colors::black);

	// TODO: Extra form code goes here	

	lb_path.fgcolor(nana::colors::white);
	lb_path.transparent(true);
	lb_path.caption("Datas folder:");

	tb_path.caption(m_parent.GetDatasFolder());
	tb_path.multi_lines(false);
	tb_path.tooltip("Select the folder containing the 'maps' and 'tle' subfolders");
	tb_path.events().text_changed([&] {
		m_parent.SetDatasFolder(tb_path.text());
								  });

	btn_path.caption("...");
	btn_path.tooltip("Select the folder containing the 'maps' and 'tle' subfolders");
	btn_path.events().click([&]() {
		nana::folderbox fb_path(*this, m_parent.GetDatasFolder());
		fb_path.allow_multi_select(false);
		auto p = fb_path();
		if (p.size() > 0) {
			tb_path.caption(p[0].string());
		}
							});

	lb_file.fgcolor(nana::colors::white);
	lb_file.transparent(true);
	lb_file.caption("TLE file:");

	lb_comment.fgcolor(nana::colors::white);
	lb_comment.transparent(true);

	lb_sat.fgcolor(nana::colors::white);
	lb_sat.transparent(true);
	lb_sat.caption("Satellite:");

	lb_name.fgcolor(nana::colors::white);
	lb_name.transparent(true);
	lb_name.caption("Name");

	tb_obs_name.multi_lines(false);
	tb_obs_lon.multi_lines(false);
	tb_obs_lon.tooltip("Positive: East of the prime meridian, Negative: West of the prime meridian");
	tb_obs_lat.multi_lines(false);
	tb_obs_lat.tooltip("Positive: Above the equator (N), Negative: Below the equator (S)");
	tb_obs_elev.multi_lines(false);
	tb_freq.multi_lines(false);
	tb_bw.multi_lines(false);

	tb_obs_name.caption(m_parent.GetLocationName());
	tb_obs_name.events().text_changed([&]() { m_parent.SetLocationName(tb_obs_name.text()); });

	lb_obs_hlon.fgcolor(nana::colors::white);
	lb_obs_hlon.transparent(true);
	lb_obs_hlon.caption("Longitude");

	lb_obs_hlat.fgcolor(nana::colors::white);
	lb_obs_hlat.transparent(true);
	lb_obs_hlat.caption("Latitude");

	lb_obs_helev.fgcolor(nana::colors::white);
	lb_obs_helev.transparent(true);
	lb_obs_helev.caption("Elevation (m)");

	lb_obs.fgcolor(nana::colors::white);
	lb_obs.transparent(true);
	lb_obs.caption("Location : ");

	lb_hfreq.fgcolor(nana::colors::white);
	lb_hfreq.transparent(true);
	lb_hfreq.caption("Freq. (MHz)");

	lb_hmode.fgcolor(nana::colors::white);
	lb_hmode.transparent(true);
	lb_hmode.caption("Mode");

	lb_hbw.fgcolor(nana::colors::white);
	lb_hbw.transparent(true);
	lb_hbw.caption("BW (Hz)");

	lb_freq.fgcolor(nana::colors::white);
	lb_freq.transparent(true);
	lb_freq.caption("Downlink:");

	lb_map.fgcolor(nana::colors::white);
	lb_map.transparent(true);
	lb_map.caption("Map size:");

	if (!std::filesystem::exists(m_parent.GetTLEListName()) || !parse_file(m_parent.GetTLEListName(), sat_list_) || !sat_list_.is_object()) {
		nana::msgbox mb(*this, "Settings");
		mb << m_parent.GetTLEListName() << " not found or incorrect, Please check the " << m_parent.GetTLEFilesFolder() << " folder.";
		mb.show();
	}
	else {
		const auto& flist = sat_list_.get_object();

		std::string selected_file = "";

		int selected_file_index = 0;
		int i = 0;
		for (auto& [key, value] : flist) {
			if (value["enabled"].bool_val()) {
				if (selected_file_index == 0)
					selected_file = key;

				cb_file.push_back(key);

				if (m_parent.GetTLEFile() == key) {
					selected_file_index = i;
					selected_file = key;
				}

				i++;
			}
		}
		cb_file.option(selected_file_index);
		lb_comment.caption(sat_list_[selected_file]["comment"].str_val());

		bool file_found = std::filesystem::exists(m_parent.GetTLEFilesFolder() + selected_file);
		if (!file_found) {
			file_found = download_tle_file(nana::to_wstring(sat_list_[selected_file]["url"].str_val()), nana::to_wstring(m_parent.GetTLEFilesFolder() + selected_file));
			if (!file_found) {
				nana::msgbox mb(*this, "Settings");
				mb << "Cannot download " << selected_file << " from " << sat_list_[selected_file]["url"].str_val() << ", Please check the " << m_parent.GetTLEListName() << " file.";
				mb.show();
			}
		}

		int selected_sat_index = 0;
		if (file_found) {
			tle_map_list sat_list = load_tle_file(m_parent.GetTLEFilesFolder() + selected_file);
			for (auto& it : sat_list) {
				cb_sat.push_back(it.first);
			}
			selected_sat_index = sat_list.contains(m_parent.GetSatName()) ? (int)std::distance(sat_list.begin(), sat_list.find(m_parent.GetSatName())) : 0;
		}
		cb_sat.option(selected_sat_index);
	}

	cb_file.events().selected([&](const nana::arg_combox& ar_cbx) {
		update_tle_file(ar_cbx.widget.caption());
							  });

	cb_sat.events().selected([&](const nana::arg_combox& ar_cbx) {
		update_sat(ar_cbx.widget.caption());
		tb_freq.from(m_parent.GetDownlinkFreq());
		cb_mod.caption(m_parent.GetModulation());
		tb_bw.from(m_parent.GetBandwidth());
							 });

	tb_obs_lon.from(m_parent.GetLongitude());
	tb_obs_lon.events().text_changed([&]() { m_parent.SetLongitude(tb_obs_lon.to_double()); });

	tb_obs_lat.from(m_parent.GetLatitude());
	tb_obs_lat.events().text_changed([&]() { m_parent.SetLatitude(tb_obs_lat.to_double()); });

	tb_obs_elev.from(m_parent.GetElevation());
	tb_obs_elev.events().text_changed([&]() { m_parent.SetElevation(tb_obs_elev.to_double()); });

	tb_freq.from(m_parent.GetDownlinkFreq());
	tb_freq.events().text_changed([&]() { m_parent.SetDownlinkFreq(tb_freq.to_double()); });

	cb_mod.push_back("AM");
	cb_mod.push_back("FM");
	cb_mod.push_back("DSB");
	cb_mod.push_back("LSB");
	cb_mod.push_back("USB");
	cb_mod.push_back("DIGITAL");
	cb_mod.push_back("IQ");
	cb_mod.caption(m_parent.GetModulation());
	cb_mod.events().selected([&](const nana::arg_combox& ar_cbx) {
		m_parent.SetModulation(ar_cbx.widget.caption());
							 });
	tb_bw.from(m_parent.GetBandwidth());
	tb_bw.events().text_changed([&]() { m_parent.SetBandwidth(tb_bw.to_double()); });

	cb_map.push_back("Small");
	cb_map.push_back("Medium");
	cb_map.push_back("Large");

	cb_map.option(e_map_type_to_int(m_parent.GetMapSize()));
	cb_map.events().selected([&](const nana::arg_combox& ar_cbx) {
		update_map_size(ar_cbx.widget.option());
							 });

	btn_update.caption("Update TLE files");
	btn_update.events().click([&]() { update_all_tle_files(); });

	btn_predict.caption("Predictions...");
	btn_predict.events().click([&]() { PredictButton_Click(); });
}

void SDRunoPlugin_SatTrackSettingsDialog::update_tle_file(const std::string& newFile) {
	if (newFile != m_parent.GetTLEFile()) {

		bool file_found = std::filesystem::exists(m_parent.GetTLEFilesFolder() + newFile);
		if (!file_found) {
			file_found = download_tle_file(nana::to_wstring(sat_list_[newFile]["url"].str_val()), nana::to_wstring(m_parent.GetTLEFilesFolder() + newFile));

			if (!file_found) {
				nana::msgbox mb(*this, "Settings");
				mb << "Cannot download " << newFile << " from " << sat_list_[newFile]["url"].str_val() << ", Please check the " << m_parent.GetTLEListName() << " file.";
				mb.show();
			}
		}

		cb_sat.clear();

		if (file_found) {
			lb_comment.caption(sat_list_[newFile]["comment"].str_val());
			m_parent.SetTLEFile(newFile);

			tle_map_list sat_list = load_tle_file(m_parent.GetTLEFilesFolder() + newFile);
			for (auto& it : sat_list) {
				cb_sat.push_back(it.first);
			}
			int selected_sat_index = sat_list.contains(m_parent.GetSatName()) ? (int)std::distance(sat_list.begin(), sat_list.find(m_parent.GetSatName())) : 0;
			cb_sat.option(selected_sat_index);
		}
		else {
			lb_comment.caption("*** Not found ***");
		}
	}
}

void SDRunoPlugin_SatTrackSettingsDialog::update_all_tle_files() {

	const auto& flist = sat_list_.get_object();
	for (auto& [key, value] : flist) {
		if (std::filesystem::exists(m_parent.GetTLEFilesFolder() + key)) {
			std::filesystem::rename(m_parent.GetTLEFilesFolder() + key, m_parent.GetTLEFilesFolder() + key + ".bak");

			if (!download_tle_file(nana::to_wstring(sat_list_[key]["url"].str_val()), nana::to_wstring(m_parent.GetTLEFilesFolder() + key))) {
				std::filesystem::rename(m_parent.GetTLEFilesFolder() + key + ".bak", m_parent.GetTLEFilesFolder() + key);

				nana::msgbox mb(*this, "Updating");
				mb << "Cannot download " << key << " from " << sat_list_[key]["url"].str_val() << ", Please check the " << m_parent.GetTLEListName() << " file.";
				mb.show();
			}
			else
				std::filesystem::remove(m_parent.GetTLEFilesFolder() + key + ".bak");
		}
	}
}

void SDRunoPlugin_SatTrackSettingsDialog::update_sat(const std::string& newSat) {
	m_parent.SetSatName(newSat);
}

void SDRunoPlugin_SatTrackSettingsDialog::update_map_size(size_t sz) {

	m_parent.SetMapSize(static_cast<e_map_type>(sz));
}


void SDRunoPlugin_SatTrackSettingsDialog::PredictButton_Click() {
	PredictDialog predictDialog{ m_parent };
	this->enabled(false);
	predictDialog.events().unload([&] { PredictDialog_Closed(); });
	predictDialog.Run();
}

void SDRunoPlugin_SatTrackSettingsDialog::PredictDialog_Closed() {
	this->enabled(true);
	this->focus();
}