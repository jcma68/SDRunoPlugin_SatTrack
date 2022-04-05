#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/timer.hpp>
#include <iunoplugin.h>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <iunoplugincontroller.h>
#include "SDRunoPlugin_SatTrackForm.h"

// Forward reference
class SDRunoPlugin_SatTrack;

class SDRunoPlugin_SatTrackUI {
public:

	SDRunoPlugin_SatTrackUI(SDRunoPlugin_SatTrack& parent, IUnoPluginController& controller);
	~SDRunoPlugin_SatTrackUI();

	void HandleEvent(const UnoEvent& evt);
	void FormClosed();

	void ShowUi();

private:

	SDRunoPlugin_SatTrack& m_parent;
	std::thread m_thread;
	std::shared_ptr<SDRunoPlugin_SatTrackForm> m_form;

	bool m_started;

	std::mutex m_lock;

	IUnoPluginController& m_controller;
};