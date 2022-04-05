#include <sstream>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "SDRunoPlugin_SatTrackSettingsDialog.h"
#include "SDRunoPlugin_SatTrackForm.h"
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

int SDRunoPlugin_SatTrackSettingsDialog::LoadX() {
	std::string tmp;
	m_controller.GetConfigurationKey("SatTrack.X", tmp);
	if (tmp.empty()) {
		return -1;
	}
	return stoi(tmp);
}

// Load Y from the ini file (if exists)
// TODO: Change Template to plugin name
int SDRunoPlugin_SatTrackSettingsDialog::LoadY() {
	std::string tmp;
	m_controller.GetConfigurationKey("SatTrack.Y", tmp);
	if (tmp.empty()) {
		return -1;
	}
	return stoi(tmp);
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

	lb_freq.fgcolor(nana::colors::white);
	lb_freq.transparent(true);
	lb_freq.caption("Dnlink (MHz):");

	lb_map.fgcolor(nana::colors::white);
	lb_map.transparent(true);
	lb_map.caption("Map size:");

	if (!std::filesystem::exists(m_parent.GetTLEListName())) {
		nana::msgbox mb(*this, "Settings");
		mb << m_parent.GetTLEListName() << " not found, Please check the " << m_parent.GetTLEFilesFolder() << " folder.";
		mb.show();
	}
	else {
		flist = load_tle_file_list(m_parent.GetTLEListName());
		int selected_file_index = 0;
		for (auto i = 0; i < (int)flist.size(); i++) {
			tle_list_line_t l = flist[i];
			cb_file.push_back(l.filename);
			if (m_parent.GetTLEFile() == l.filename)
				selected_file_index = i;
		}
		cb_file.option(selected_file_index);
		lb_comment.caption(flist[selected_file_index].comment);

		bool file_found = std::filesystem::exists(m_parent.GetTLEFilesFolder() + flist[selected_file_index].filename);
		if (!file_found) {
			file_found = download_tle_file(nana::to_wstring(flist[selected_file_index].url), nana::to_wstring(m_parent.GetTLEFilesFolder() + flist[selected_file_index].filename));
			if (!file_found) {
				nana::msgbox mb(*this, "Settings");
				mb << "Cannot download " << flist[selected_file_index].filename << " from " << flist[selected_file_index].url << ", Please check the " << m_parent.GetTLEListName() << " file.";
				mb.show();
			}
		}

		int selected_sat_index = 0;

		if (file_found) {
			tle_map_list sat_list = load_tle_file(m_parent.GetTLEFilesFolder() + flist[selected_file_index].filename);
			for (auto& it : sat_list) {
				if (it.first.find("[-]") != std::string::npos || it.first.find("[D]") != std::string::npos)
					continue;

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
							 });

	tb_obs_lon.from(m_parent.GetLongitude());
	tb_obs_lon.events().text_changed([&]() { m_parent.SetLongitude(tb_obs_lon.to_double()); });

	tb_obs_lat.from(m_parent.GetLatitude());
	tb_obs_lat.events().text_changed([&]() { m_parent.SetLatitude(tb_obs_lat.to_double()); });

	tb_obs_elev.from(m_parent.GetElevation());
	tb_obs_elev.events().text_changed([&]() { m_parent.SetElevation(tb_obs_elev.to_double()); });

	tb_freq.from(m_parent.GetDownlinkFreq());
	tb_freq.events().text_changed([&]() { m_parent.SetDownlinkFreq(tb_freq.to_double()); });

	cb_map.push_back("Small");
	cb_map.push_back("Medium");
	cb_map.push_back("Large");

	cb_map.option(e_map_type_to_int(m_parent.GetMapSize()));
	cb_map.events().selected([&](const nana::arg_combox& ar_cbx) {
		update_map_size(ar_cbx.widget.option());
							 });

	btn_update.caption("Update TLE files");
	btn_update.events().click([&]() { update_all_tle_files(); });
}

void SDRunoPlugin_SatTrackSettingsDialog::update_tle_file(const std::string& newFile) {
	if (newFile != m_parent.GetTLEFile()) {
		size_t selected = cb_file.option();
		tle_list_line_t l = flist[selected];

		bool file_found = std::filesystem::exists(m_parent.GetTLEFilesFolder() + l.filename);
		if (!file_found) {
			file_found = download_tle_file(nana::to_wstring(l.url), nana::to_wstring(m_parent.GetTLEFilesFolder() + l.filename));
			if (!file_found) {
				nana::msgbox mb(*this, "Settings");
				mb << "Cannot download " << l.filename << " from " << l.url << ", Please check the " << m_parent.GetTLEListName() << " file.";
				mb.show();
			}
		}

		cb_sat.clear();

		if (file_found) {
			lb_comment.caption(l.comment);
			m_parent.SetTLEFile(l.filename);

			tle_map_list sat_list = load_tle_file(m_parent.GetTLEFilesFolder() + l.filename);
			for (auto& it : sat_list) {
				if (it.first.find("[-]") != std::string::npos || it.first.find("[D]") != std::string::npos)
					continue;

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

	if (!std::filesystem::exists(m_parent.GetTLEListName())) {
		nana::msgbox mb(*this, "Updating");
		mb << m_parent.GetTLEListName() << " not found, Please check the " << m_parent.GetTLEFilesFolder() << " folder.";
		mb.show();
	}
	else {
		std::vector<tle_list_line_t> list = load_tle_file_list(m_parent.GetTLEListName());
		for (auto& f : list) {
			if (std::filesystem::exists(m_parent.GetTLEFilesFolder() + f.filename)) {
				std::filesystem::rename(m_parent.GetTLEFilesFolder() + f.filename, m_parent.GetTLEFilesFolder() + f.filename + ".bak");

				if (!download_tle_file(nana::to_wstring(f.url), nana::to_wstring(m_parent.GetTLEFilesFolder() + f.filename))) {
					std::filesystem::rename(m_parent.GetTLEFilesFolder() + f.filename + ".bak", m_parent.GetTLEFilesFolder() + f.filename);

					nana::msgbox mb(*this, "Updating");
					mb << "Cannot download " << f.filename << " from " << f.url << ", Please check the " << m_parent.GetTLEListName() << " file.";
					mb.show();
				}
				else
					std::filesystem::remove(m_parent.GetTLEFilesFolder() + f.filename + ".bak");
			}
		}
	}
}

void SDRunoPlugin_SatTrackSettingsDialog::update_sat(const std::string& newSat) {
	m_parent.SetSatName(newSat);
}

void SDRunoPlugin_SatTrackSettingsDialog::update_map_size(size_t sz) {

	m_parent.SetMapSize(static_cast<e_map_type>(sz));
}