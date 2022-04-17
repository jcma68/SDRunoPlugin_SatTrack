#include <sstream>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include "SDRunoPlugin_SatTrackForm.h"
#include "SDRunoPlugin_SatTrackSettingsDialog.h"
#include "SDRunoPlugin_SatTrackUI.h"
#include "resource.h"
#include <io.h>
#include <shlobj.h>

#define VERSION "V1.0"

// Form constructor with handles to parent and uno controller - launches form Setup
SDRunoPlugin_SatTrackForm::SDRunoPlugin_SatTrackForm(SDRunoPlugin_SatTrackUI& parent, IUnoPluginController& controller)
	: nana::form(nana::API::make_center(default_formWidth, default_formHeight), nana::appearance(false, true, false, false, true, false, false))
	, m_parent(parent)
	, m_controller(controller)	{

	LoadSettings();
	Setup();
}

// Form deconstructor
SDRunoPlugin_SatTrackForm::~SDRunoPlugin_SatTrackForm()
{
	SavePos();
}

// Start Form and start Nana UI processing
void SDRunoPlugin_SatTrackForm::Run() {
	show();
	nana::exec();
}

#ifdef _WIN32

static bool load_image_from_res(nana::paint::image& img, unsigned short id) {
	HDC hdc = GetDC(NULL);
	if (hdc == NULL)
		return false;

	HMODULE hModule = GetModuleHandle(L"SDRunoPlugin_SatTrack");
	if (hModule == NULL) 
		return false;

	HBITMAP hBitmap = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_COPYFROMRESOURCE);
	if (hBitmap == NULL) 
		return false;

	BITMAPINFO bitmapInfo = { 0 };
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);

	if (GetDIBits(hdc, hBitmap, 0, 0, NULL, &bitmapInfo, DIB_RGB_COLORS) == NULL) 
		return false;

	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	const unsigned int rawDataOffset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);

	BITMAPFILEHEADER bitmapFileHeader = { 0 };
	bitmapFileHeader.bfOffBits = rawDataOffset;
	bitmapFileHeader.bfSize = bitmapInfo.bmiHeader.biSizeImage;
	bitmapFileHeader.bfType = 0x4D42;

	std::unique_ptr<BYTE[]> lpPixels = std::make_unique<BYTE[]>(bitmapInfo.bmiHeader.biSizeImage + rawDataOffset);

	*(BITMAPFILEHEADER*)lpPixels.get() = bitmapFileHeader;
	*(BITMAPINFO*)(lpPixels.get() + sizeof(BITMAPFILEHEADER)) = bitmapInfo;

	if (GetDIBits(hdc, hBitmap, 0, bitmapInfo.bmiHeader.biHeight, (LPVOID)(lpPixels.get() + rawDataOffset), &bitmapInfo, DIB_RGB_COLORS) == NULL) 
		return false;
	
	img.open(lpPixels.get(), bitmapInfo.bmiHeader.biSizeImage);

	ReleaseDC(NULL, hdc);

	return true;
}

#endif

// Create the initial plugin form
void SDRunoPlugin_SatTrackForm::Setup() {

	unsigned int formWidth = default_formWidth;
	unsigned int formHeight = default_formHeight;

	nana::size window_map_size = sattrack_widget::calc_window_size(GetMapSize());

	formWidth = window_map_size.width + (2 * sideBorderWidth);
	formHeight = window_map_size.height + topBarHeight + bottomBarHeight;

	// This first section is all related to the background and border
	// it shouldn't need to be changed

	bg_border.size({ formWidth, formHeight });
	bg_inner.size({ formWidth - (2 * sideBorderWidth), formHeight - topBarHeight - bottomBarHeight });
	bg_inner.move({ sideBorderWidth, topBarHeight });

	form_drag_label.size({ formWidth, formHeight });

	versionLbl.move({ (int)(formWidth - 40), (int)(formHeight - 30) });

	nana::paint::image img_border;
	nana::paint::image img_inner;

	load_image_from_res(img_border, IDB_BG_BORDER);
	load_image_from_res(img_inner, IDB_BACKGROUND);
	load_image_from_res(img_close_normal, IDB_CLOSE);
	load_image_from_res(img_close_down, IDB_CLOSE_DOWN);
	load_image_from_res(img_min_normal, IDB_MIN);
	load_image_from_res(img_min_down, IDB_MIN_DOWN);
	load_image_from_res(img_header, IDB_HEADER);
	load_image_from_res(img_sett_normal, IDB_SETT);
	load_image_from_res(img_sett_down, IDB_SETT_DOWN);

	bg_border.load(img_border, nana::rectangle(0, 0, 590, 340));
	bg_border.stretchable(0, 0, 0, 0);
	bg_border.transparent(true);
	bg_inner.load(img_inner, nana::rectangle(0, 0, 582, 299));
	bg_inner.stretchable(sideBorderWidth, 0, sideBorderWidth, bottomBarHeight);
	bg_inner.transparent(false);

	// TODO: Form code starts here

	// Load X and Y location for the form from the ini file (if exists)
	move(formX_, formY_);
	events().move([&](const nana::arg_move& mov) {
		formX_ = mov.x;
		formY_ = mov.y;
	});


	// This code sets the plugin size, title and what to do when the X is pressed
	size(nana::size(formWidth, formHeight));
	caption("SDRuno SatTrack Plugin");
	events().destroy([&] { m_parent.FormClosed(); });

	//Initialize header bar
	header_bar.size(nana::size(122, 20));
	header_bar.load(img_header, nana::rectangle(0, 0, 122, 20));
	header_bar.stretchable(0, 0, 0, 0);
	header_bar.move(nana::point((formWidth / 2) - 61, 5));
	header_bar.transparent(true);

	//Initial header text 
	title_bar_label.size(nana::size(65, 12));
	title_bar_label.move(nana::point((formWidth / 2) - 5, 9));
	title_bar_label.format(true);
	title_bar_label.caption("< bold size = 6 color = 0x000000 font = \"Verdana\">SatTrack</>");
	title_bar_label.text_align(nana::align::center, nana::align_v::center);
	title_bar_label.fgcolor(nana::color_rgb(0x000000));
	title_bar_label.transparent(true);

	//Iniitialize drag_label
	form_drag_label.move(nana::point(0, 0));
	form_drag_label.transparent(true);

	//Initialize dragger and set target to form, and trigger to drag_label 
	form_dragger.target(*this);
	form_dragger.trigger(form_drag_label);

	//Initialise the "Minimize button"
	min_button.load(img_min_normal, nana::rectangle(0, 0, 20, 15));
	min_button.bgcolor(nana::color_rgb(0x000000));
	min_button.move(nana::point(formWidth - 51, 9));
	min_button.transparent(true);
	min_button.events().mouse_down([&] { min_button.load(img_min_down, nana::rectangle(0, 0, 20, 15)); });
	min_button.events().mouse_up([&] { min_button.load(img_min_normal, nana::rectangle(0, 0, 20, 15)); nana::API::zoom_window(this->handle(), false); });
	min_button.events().mouse_leave([&] { min_button.load(img_min_normal, nana::rectangle(0, 0, 20, 15)); });

	//Initialise the "Close button"
	close_button.load(img_close_normal, nana::rectangle(0, 0, 20, 15));
	close_button.bgcolor(nana::color_rgb(0x000000));
	close_button.move(nana::point(formWidth - 26, 9));
	close_button.transparent(true);
	close_button.events().mouse_down([&] { close_button.load(img_close_down, nana::rectangle(0, 0, 20, 15)); });
	close_button.events().mouse_up([&] { close_button.load(img_close_normal, nana::rectangle(0, 0, 20, 15)); close(); });
	close_button.events().mouse_leave([&] { close_button.load(img_close_normal, nana::rectangle(0, 0, 20, 15)); });

	//Uncomment the following block of code to Initialise the "Setting button"
	sett_button.load(img_sett_normal, nana::rectangle(0, 0, 40, 15));
	sett_button.bgcolor(nana::color_rgb(0x000000));
	sett_button.move(nana::point(10, 9));
	sett_button.events().mouse_down([&] { sett_button.load(img_sett_down, nana::rectangle(0, 0, 40, 15)); });
	sett_button.events().mouse_up([&] { sett_button.load(img_sett_normal, nana::rectangle(0, 0, 40, 15)); SettingsButton_Click(); });
	sett_button.events().mouse_leave([&] { sett_button.load(img_sett_normal, nana::rectangle(0, 0, 40, 15)); });
	sett_button.tooltip("Show settings window");
	sett_button.transparent(true);

	versionLbl.fgcolor(nana::colors::white);
	versionLbl.caption(VERSION);
	versionLbl.transparent(true);

	// TODO: Extra Form code goes here

	sattrack_ctrl.bgcolor(nana::colors::black);
	sattrack_ctrl.set_map(GetMapSize(), maps_dirs_);
	sattrack_ctrl.set_downlink_freq(GetDownlinkFreq() * 1000000.0);
	sattrack_ctrl.set_site(GetLocationName(), GetLatitude(), GetLongitude(), GetElevation());

	SatChanged();

	dopplerTimer_.interval(std::chrono::milliseconds(1000));
	dopplerTimer_.elapse([&] {
		DopplerTick();
	});
	dopplerTimer_.start();
}

void SDRunoPlugin_SatTrackForm::SatChanged() {
	sattrack_ctrl.stop();

	if (!std::filesystem::exists(tle_files_dirs_ + GetTLEFile())) {
		nana::msgbox mb(*this, "SDRunoPlugin_SatTrackForm");
		mb << tle_files_dirs_ + GetTLEFile() << " not found, Please check the " << tle_files_dirs_ << " folder.";
		mb.show();
	}
	else {
		tle_map_list sat_list = load_tle_file(tle_files_dirs_ + GetTLEFile());
		if (sat_list.contains(GetSatName())) {
			line_pair tle_data = sat_list[GetSatName()];
			elsetrec satrec{};
			parse_tle_lines(tle_data, 'a', wgs72, satrec);
			if (!satrec.error) {
				sattrack_ctrl.set_satellite(GetSatName(), satrec);
			}
		}
	}

	sattrack_ctrl.start();
}

void SDRunoPlugin_SatTrackForm::SettingsButton_Click()
{
	//Create a new settings dialog object
	SDRunoPlugin_SatTrackSettingsDialog settingsDialog{ *this, m_controller };

	//disable this form so settings dialog retains top level focus
	this->enabled(false);

	//Attach a handler to the settings dialog close event
	settingsDialog.events().unload([&] { SettingsDialog_Closed(); });

	//Show the setttings dialog
	settingsDialog.Run();
}

void SDRunoPlugin_SatTrackForm::SettingsDialog_Closed()
{
	//DO NOT REMOVE THE FLLOWING CODE it is required for the proper operation of the settings dialog form

	this->enabled(true);
	this->focus();

	//TODO: Extra code goes here to be preformed when settings dialog form closes

	SavePos();
	config_.save_to(config_file_);
}

void SDRunoPlugin_SatTrackForm::SetDatasFolder(const std::string& name) {
	data_dir_ = (name.back() != '\\') ? name + "\\" : name;
	maps_dirs_ = data_dir_ + "maps\\";
	tle_files_dirs_ = data_dir_ + "tle\\";
	tle_list_ = tle_files_dirs_ + TLE_LIST;
	config_file_ = tle_files_dirs_ + CONFIG_FILE;

	m_controller.SetConfigurationKey("SatTrack.DatasFolder", data_dir_);

	SatChanged();
}

void SDRunoPlugin_SatTrackForm::SetTLEFile(const std::string& name) {
	config_["current"]["tle_file"] = name;
}

void SDRunoPlugin_SatTrackForm::SetSatName(const std::string& name) {

	if (config_["current"]["name"].str_val() != name) {
		config_["current"]["name"] = name;

		SatChanged();
	}
}

void SDRunoPlugin_SatTrackForm::SetLocationName(const std::string& name) {
	auto& loc = config_["location"];

	loc["name"] = name;

	sattrack_ctrl.set_site(name, loc["latitude"].num_val(), loc["longitude"].num_val(), loc["elevation"].num_val());
}

void SDRunoPlugin_SatTrackForm::SetLatitude(double value) {
	auto& loc = config_["location"];

	loc["latitude"] = value;

	sattrack_ctrl.set_site(loc["name"].str_val(), value, loc["longitude"].num_val(), loc["elevation"].num_val());
}

void SDRunoPlugin_SatTrackForm::SetLongitude(double value) {
	auto& loc = config_["location"];

	loc["longitude"] = value;

	sattrack_ctrl.set_site(loc["name"].str_val(), loc["latitude"].num_val(), value, loc["elevation"].num_val());
}

void SDRunoPlugin_SatTrackForm::SetElevation(double value) {

	auto& loc = config_["location"];

	loc["elevation"] = value;

	sattrack_ctrl.set_site(loc["name"].str_val(), loc["latitude"].num_val(), loc["longitude"].num_val(), value);
}

void SDRunoPlugin_SatTrackForm::SetDownlinkFreq(double value) {

	const std::string& name = GetSatName();

	auto& sats = config_["satellites"];
	if (!sats.contains_key(name)) {
		create_sat_entry(sats, name);
	}
	sats[name]["downlink"] = value;

	sattrack_ctrl.set_downlink_freq(value * 1000000.0);
}

void SDRunoPlugin_SatTrackForm::SetMapSize(e_map_type sz) {
	config_["current"]["map_size"] = e_map_type_to_int(sz);

	sattrack_ctrl.stop();
	ResizeWindow(sz);
	sattrack_ctrl.start();
}

void SDRunoPlugin_SatTrackForm::SetModulation(const std::string& value) {
	const std::string& name = GetSatName();

	auto& sats = config_["satellites"];
	if (!sats.contains_key(name)) {
		create_sat_entry(sats, name);
	}
	sats[name]["modulation"] = value;
}

void SDRunoPlugin_SatTrackForm::SetBandwidth(double value) {
	const std::string& name = GetSatName();

	auto& sats = config_["satellites"];
	if (!sats.contains_key(name)) {
		create_sat_entry(sats, name);
	}
	sats[name]["bandwidth"] = value;
	}

std::string SDRunoPlugin_SatTrackForm::GetTLEFile() const {
	return config_["current"]["tle_file"].str_val();
	}

std::string SDRunoPlugin_SatTrackForm::GetSatName() const {
	return config_["current"]["name"].str_val();
	}

std::string SDRunoPlugin_SatTrackForm::GetLocationName() const {
	return config_["location"]["name"].str_val();
	}

double SDRunoPlugin_SatTrackForm::GetLatitude() const {
	return config_["location"]["latitude"].num_val();
	}

double SDRunoPlugin_SatTrackForm::GetLongitude() const {
	return config_["location"]["longitude"].num_val();
	}

double SDRunoPlugin_SatTrackForm::GetElevation() const {
	return config_["location"]["elevation"].num_val();
	}

double SDRunoPlugin_SatTrackForm::GetDownlinkFreq() {
	const std::string& name = GetSatName();

	auto& sats = config_["satellites"];
	if (!sats.contains_key(name)) {
		create_sat_entry(sats, name);
	}
	return sats[name]["downlink"].num_val();
	}

std::string SDRunoPlugin_SatTrackForm::GetModulation() {
	const std::string& name = GetSatName();

	auto& sats = config_["satellites"];
	if (!sats.contains_key(name)) {
		create_sat_entry(sats, name);
	}
	return sats[name]["modulation"].str_val();
	}

double SDRunoPlugin_SatTrackForm::GetBandwidth() {
	const std::string& name = GetSatName();

	auto& sats = config_["satellites"];
	if (!sats.contains_key(name)) {
		create_sat_entry(sats, name);
	}
	return sats[name]["bandwidth"].num_val();
	}

e_map_type SDRunoPlugin_SatTrackForm::GetMapSize() const {

	int map_size = (int)config_["current"]["map_size"].num_val();

	return static_cast<e_map_type>(map_size);
	}

json_utils::json_value& SDRunoPlugin_SatTrackForm::GetSelections() {
	return config_["selections"];
	}

void SDRunoPlugin_SatTrackForm::LoadSettings() 	{
	std::string tmp;

	m_controller.GetConfigurationKey("SatTrack.DatasFolder", tmp);
	if (tmp.empty()) {

		CHAR szPath[MAX_PATH];
		SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, 0, szPath);
		std::string strpath(szPath);
		if (strpath.back() != '\\') 
			strpath = strpath + "\\";
		data_dir_ = strpath + "CommunityPlugins\\";
	}
	else {
		data_dir_ = (tmp.back() != '\\') ? tmp + "\\" : tmp;

	}

	m_controller.SetConfigurationKey("SatTrack.DatasFolder", data_dir_);

	maps_dirs_ = data_dir_ + "maps\\";
	tle_files_dirs_ = data_dir_ + "tle\\";
	tle_list_ = tle_files_dirs_ + TLE_LIST;
	config_file_ = tle_files_dirs_ + CONFIG_FILE;

	if (!std::filesystem::exists(config_file_)) {
		create_default_config(config_file_);
	}

	if (!std::filesystem::exists(config_file_) || !parse_file(config_file_, config_)) {
		nana::msgbox mb(*this, "SDRunoPlugin_SatTrackForm");
		mb << config_file_ << " not found or incorrect, Please check the " << tle_files_dirs_ << " folder.";
		mb.show();

		return;
	}

	SatChanged();

	m_controller.GetConfigurationKey("SatTrack.X", tmp);
	if (tmp.empty()) {
		formX_ = 0;
	}
	else {
		formX_ = stoi(tmp);
		if (formX_ < 0)
			formX_ = 0;
	}

	m_controller.GetConfigurationKey("SatTrack.Y", tmp);
	if (tmp.empty()) {
		formY_ = 0;
	}
	else {
		formY_ = stoi(tmp);
		if (formY_ < 0)
			formY_ = 0;
	}

	m_controller.GetConfigurationKey("SatTrack.SettingsX", tmp);
	if (tmp.empty()) {
		settingsX_ = formX_;
	}
	else {
		settingsX_ = stoi(tmp);
		if (settingsX_ < 0)
			settingsX_ = 0;

	}

	m_controller.GetConfigurationKey("SatTrack.SettingsY", tmp);
	if (tmp.empty()) {
		settingsY_ = formY_;
	}
	else {
		settingsY_ = stoi(tmp);
		if (settingsY_ < 0)
			settingsY_ = 0;

	}

	m_controller.GetConfigurationKey("SatTrack.PredictX", tmp);
	if (tmp.empty()) {
		predictX_ = formX_;
	}
	else {
		predictX_ = stoi(tmp);
		if (predictX_ < 0)
			predictX_ = 0;

	}

	m_controller.GetConfigurationKey("SatTrack.PredictY", tmp);
	if (tmp.empty()) {
		predictY_ = formY_;
	}
	else {
		predictY_ = stoi(tmp);
		if (predictY_ < 0)
			predictY_ = 0;

	}
}

void SDRunoPlugin_SatTrackForm::ResizeWindow(e_map_type map_type) {

	unsigned int formWidth = default_formWidth;
	unsigned int formHeight = default_formHeight;

	nana::size window_map_size = sattrack_widget::calc_window_size(map_type);
	formWidth = window_map_size.width + (2 * sideBorderWidth);
	formHeight = window_map_size.height + topBarHeight + bottomBarHeight;

	size(nana::size(formWidth, formHeight));

	bg_border.size({ formWidth, formHeight });
	bg_inner.size({ formWidth - (2 * sideBorderWidth), formHeight - topBarHeight - bottomBarHeight });
	bg_inner.move({ sideBorderWidth, topBarHeight });

	form_drag_label.size({ formWidth, formHeight });

	versionLbl.move({ (int)(formWidth - 40), (int)(formHeight - 30) });

	header_bar.move(nana::point((formWidth / 2) - 61, 5));

	title_bar_label.move(nana::point((formWidth / 2) - 5, 9));

	min_button.move(nana::point(formWidth - 51, 9));

	close_button.move(nana::point(formWidth - 26, 9));

	sattrack_ctrl.set_map(GetMapSize(), maps_dirs_);
}

void SDRunoPlugin_SatTrackForm::SavePos() {
	// do not store negative position (-32000 when windows is minimized)
	if (formX_ >= 0 && formY_ >= 0) {
		m_controller.SetConfigurationKey("SatTrack.X", std::to_string(formX_));
		m_controller.SetConfigurationKey("SatTrack.Y", std::to_string(formY_));
	}

	if (settingsX_ >= 0 && settingsY_ >= 0) {
		m_controller.SetConfigurationKey("SatTrack.SettingsX", std::to_string(settingsX_));
		m_controller.SetConfigurationKey("SatTrack.SettingsY", std::to_string(settingsY_));
	}

	if (predictX_ >= 0 && predictY_ >= 0) {
		m_controller.SetConfigurationKey("SatTrack.PredictX", std::to_string(predictX_));
		m_controller.SetConfigurationKey("SatTrack.PredictY", std::to_string(predictY_));
	}
}

void SDRunoPlugin_SatTrackForm::DopplerTick() {
	double freq = sattrack_ctrl.get_doppler_correction_hz();

	m_controller.SetVfoFrequency(0, freq);
}