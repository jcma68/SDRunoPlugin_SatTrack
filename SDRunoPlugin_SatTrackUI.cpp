#include <sstream>
#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/timer.hpp>
#include <unoevent.h>

#include "SDRunoPlugin_SatTrack.h"
#include "SDRunoPlugin_SatTrackUI.h"
#include "SDRunoPlugin_SatTrackForm.h"

// Ui constructor - load the Ui control into a thread
SDRunoPlugin_SatTrackUI::SDRunoPlugin_SatTrackUI(SDRunoPlugin_SatTrack& parent, IUnoPluginController& controller) :
	m_parent(parent),
	m_form(nullptr),
	m_controller(controller) {
	m_thread = std::thread(&SDRunoPlugin_SatTrackUI::ShowUi, this);
}

// Ui destructor (the nana::API::exit_all();) is required if using Nana UI library
SDRunoPlugin_SatTrackUI::~SDRunoPlugin_SatTrackUI() {
	nana::API::exit_all();
	m_thread.join();
}

// Show and execute the form
void SDRunoPlugin_SatTrackUI::ShowUi() {
	m_lock.lock();
	m_form = std::make_shared<SDRunoPlugin_SatTrackForm>(*this, m_controller);
	m_lock.unlock();

	m_form->Run();
}


// Handle events from SDRuno
// TODO: code what to do when receiving relevant events
void SDRunoPlugin_SatTrackUI::HandleEvent(const UnoEvent& ev) {
	switch (ev.GetType()) {
	case UnoEvent::StreamingStarted:
		break;

	case UnoEvent::StreamingStopped:
		break;

	case UnoEvent::SavingWorkspace:
		break;

	case UnoEvent::ClosingDown:
		FormClosed();
		break;

	default:
		break;
	}
}

// Required to make sure the plugin is correctly unloaded when closed
void SDRunoPlugin_SatTrackUI::FormClosed() {
	m_controller.RequestUnload(&m_parent);
}
