#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <iunoplugincontroller.h>
#include <iunoplugin.h>
#include <iunostreamobserver.h>
#include <iunoaudioobserver.h>
#include <iunoaudioprocessor.h>
#include <iunostreamobserver.h>
#include <iunoannotator.h>

#include "SDRunoPlugin_SatTrackUI.h"

class SDRunoPlugin_SatTrack : public IUnoPlugin {

public:

	SDRunoPlugin_SatTrack(IUnoPluginController& controller);
	virtual ~SDRunoPlugin_SatTrack();

	virtual const char* GetPluginName() const override { return "SDRuno SatTrack"; }

	// IUnoPlugin
	virtual void HandleEvent(const UnoEvent& ev) override;

private:

	void WorkerFunction();
	void SetMode();

	std::thread* m_worker;
	std::mutex m_lock;
	SDRunoPlugin_SatTrackUI m_form;
};