#include <sstream>
#include <unoevent.h>
#include <iunoplugincontroller.h>
#include <vector>
#include <sstream>
#include <chrono>
#include <nana/system/platform.hpp>
#include "SDRunoPlugin_SatTrack.h"
#include "SDRunoPlugin_SatTrackUI.h"

SDRunoPlugin_SatTrack::SDRunoPlugin_SatTrack(IUnoPluginController& controller) :
	IUnoPlugin(controller),
	m_form(*this, controller),
	m_worker(nullptr) {

	SetMode();
}

SDRunoPlugin_SatTrack::~SDRunoPlugin_SatTrack() {
}

void SDRunoPlugin_SatTrack::HandleEvent(const UnoEvent& ev) {
	m_form.HandleEvent(ev);
}

void SDRunoPlugin_SatTrack::WorkerFunction() {
	// Worker Function Code Goes Here
}

void SDRunoPlugin_SatTrack::SetMode() {

	std::lock_guard<std::mutex> l(m_lock);

	if (m_controller.GetDemodulatorType(0) != IUnoPluginController::DemodulatorMFM) {
		m_controller.SetDemodulatorType(0, IUnoPluginController::DemodulatorNone);
		nana::system::sleep(100);
		m_controller.SetDemodulatorType(0, IUnoPluginController::DemodulatorMFM);
	}
}

