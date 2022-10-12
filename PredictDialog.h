#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/checkbox.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <nana/threads/pool.hpp>

#include "sat_tools.h"
#include "sat_calc.h"

class SDRunoPlugin_SatTrackForm;

class PredictDialog : public nana::form {
public:
	PredictDialog(SDRunoPlugin_SatTrackForm& parent);

	void Run();

private:
	void Setup();

	struct predict_results {
		double jd_pass_start;
		double jd_pass_max;
		double jd_pass_end;

		double azm_start;
		double azm_max;
		double azm_end;

		double elev_start;
		double elev_max;
		double elev_end;
		std::string name;

		bool operator==(const predict_results& rhs) const {
			return (jd_pass_start == rhs.jd_pass_start);
		}
		bool operator<(const predict_results& rhs) const {
			return jd_pass_start < rhs.jd_pass_start;
		}
	};

	void Predict();
	void Predictions(const std::string& sat_name);

	std::string file_;
	json_utils::json_value& selections_;
	std::map<std::string, line_pair> sat_list_;
	observer_t observer_;
	double filter_{};

	//nana::threads::pool thrpool_;
	std::atomic_bool canceled_ = false;

	std::multiset<predict_results> results_;

	nana::listbox lb_predicts{ *this, {10,10,415,380} };
	nana::listbox lb_select{ *this, {430,10,190,380} };
	nana::label lb_filter{ *this, nana::rectangle(10, 400, 110, 20) };
	nana::textbox tb_filter{ *this, nana::rectangle(125, 400, 60, 20) };

	nana::button btn_predict{ *this, nana::rectangle(190, 400, 100, 20) };
	nana::progress  prog_{ *this, nana::rectangle(295, 400, 325, 20) };

	SDRunoPlugin_SatTrackForm& m_parent;
};