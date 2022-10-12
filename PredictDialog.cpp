#include "PredictDialog.h"
#include "SDRunoPlugin_SatTrackForm.h"

PredictDialog::PredictDialog(SDRunoPlugin_SatTrackForm& parent) :
	nana::form(nana::API::make_center(630, 440), nana::appearance(true, false, true, false, false, false, false)),
	m_parent(parent),
	file_(parent.GetTLEFile()),
	selections_(parent.GetSelections()) {
	Setup();
}

void PredictDialog::Run() {
	show();
	nana::exec();
}

void PredictDialog::Setup() {
	file_ = m_parent.GetTLEFile();
	selections_ = m_parent.GetSelections();

	move(m_parent.GetPredictX(), m_parent.GetPredictY());
	events().move([&](const nana::arg_move& mov) {
		m_parent.SetPredictX(mov.x);
		m_parent.SetPredictY(mov.y);
				  });

	caption("SDRuno SatTrack Plugin - Predictions");

	// Set the forms back color to black to match SDRuno's settings dialogs
	this->bgcolor(nana::colors::black);

	double obs_lat = to_rad(m_parent.GetLatitude());
	double obs_lon = to_rad(m_parent.GetLongitude());
	double obs_ht_in_meters = m_parent.GetElevation();

	observer_.reset(obs_lat, obs_lon, obs_ht_in_meters / 1000.0);

	lb_predicts.enabled(false);
	lb_predicts.sortable(false);
	lb_predicts.append_header("Time", 120);
	lb_predicts.append_header("Satellite", 145);
	lb_predicts.append_header("Azimut", 60);
	lb_predicts.append_header("Elevation", 60);
	lb_predicts.column_at(0).text_align(nana::align::left);
	lb_predicts.column_at(1).text_align(nana::align::left);
	lb_predicts.column_at(2).text_align(nana::align::right);
	lb_predicts.column_at(3).text_align(nana::align::right);

	lb_select.checkable(true);
	lb_select.sortable(false);
	lb_select.append_header("Selection", 165);

	lb_filter.fgcolor(nana::colors::white);
	lb_filter.transparent(true);
	lb_filter.caption("Satellite elevation >");

	sat_list_ = load_tle_file(m_parent.GetTLEFilesFolder() + file_);

	filter_ = selections_["sat_elevation"].num_val();

	tb_filter.multi_lines(false);
	tb_filter.from(filter_);
	tb_filter.events().text_changed([&]() {
		filter_ = tb_filter.to_double();
		selections_["sat_elevation"] = filter_;
									});

	for (auto& l : sat_list_) {
		auto item = lb_select.at(0).append(l.first);
		if (!selections_.contains_key(l.first)) {
			selections_.add_pair(l.first, json_utils::json_value{ false });
		}
		else
			item->check(selections_[l.first].bool_val());
	}
	lb_select.events().checked([&](const nana::arg_listbox& ar_lbx) {
		if (selections_.contains_key(ar_lbx.item.text(0))) {
			selections_[ar_lbx.item.text(0)] = ar_lbx.item.checked();
		}
	});

	btn_predict.caption("Predict");

	prog_.bgcolor(nana::colors::black);
	prog_.hide();

	//btn_predict.events().click(nana::threads::pool_push(thrpool_, *this, &PredictDialog::Predict));
	btn_predict.events().click([&] {
		Predict();
	});

	events().unload([&] {

		if (!canceled_.load())
		{
			canceled_ = true;

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
					});
}

void PredictDialog::Predictions(const std::string& sat_name) {

	if (!sat_list_.contains(sat_name))
		return;

	line_pair tle_data = sat_list_[sat_name];

	elsetrec satrec; /* Pointer to two-line elements set for satellite */
	parse_tle_lines(tle_data, 'a', wgs72, satrec);
	if (satrec.error)
		return;

	bool geostationary = (int)((2.0 * M_PI) / satrec.no_kozai) == 1436;	// 1 sideral day : 23h 56mn 4.0905 s = 1436.068175 mn
	if (geostationary)
		return; // geostationary satellites are ignored

	double start = julian_now();
	double end = start + 1.0;	// 1 day predictions

	double tle_date = satrec.jdsatepoch + satrec.jdsatepochF;
	double rev_per_day = satrec.no_kozai * 1440 / (2.0 * M_PI);	// (rev/day)
	double step = 1.0 / rev_per_day / 20.0; // coarse step 20 points per period

	bool first = true;
	double previous_elev = 0.0;

	predict_results result{};
	result.name = sat_name;

	double jd = (tle_date > start) ? tle_date : start;
	while (jd < end) {

		auto [azimut, elevation] = calc_azm_elev(jd, observer_, satrec);
		if (satrec.error != 0)
			break;

		if (first) {
			previous_elev = elevation;
			jd += step;
			first = false;

			continue;
		}

		if (elevation * previous_elev < 0.0) {	// zero crossing
			if (previous_elev < elevation) {
				result.jd_pass_start = regula_falsi(jd - step, jd, observer_, satrec);
				if (result.jd_pass_start > 0) {
					std::tie(result.azm_start, result.elev_start) = calc_azm_elev(result.jd_pass_start, observer_, satrec);
				}
			}
			else {
				if (result.jd_pass_start > 0) {

					result.jd_pass_end = regula_falsi(jd - step, jd, observer_, satrec);
					if (result.jd_pass_end > 0) {
						std::tie(result.azm_end, result.elev_end) = calc_azm_elev(result.jd_pass_end, observer_, satrec);

						result.jd_pass_max = (result.jd_pass_start + result.jd_pass_end) / 2.0;
						std::tie(result.azm_max, result.elev_max) = calc_azm_elev(result.jd_pass_max, observer_, satrec);
						if (satrec.error == 0) {
							results_.insert(result);
						}
					}
				}

				result.jd_pass_start = 0;
				result.jd_pass_end = 0;
			}
		}

		previous_elev = elevation;

		jd += step;
	}

}

void PredictDialog::Predict() {
	results_.clear();
	lb_predicts.clear();

	auto sats = lb_select.checked();
	if (sats.empty())
		return;

	canceled_ = false;

	tb_filter.enabled(false);
	btn_predict.enabled(false);
	prog_.show();

	prog_.amount((int)sats.size());
	prog_.value(0);

	for (const auto& sat : sats) {
		if (canceled_.load()) {
			prog_.hide();
			btn_predict.enabled(true);
			tb_filter.enabled(true);

			return;
		}

		std::string s = lb_select.at(sat).text(0);
		prog_.inc();
		prog_.caption(s);
		Predictions(s);
	}

	prog_.amount((int)results_.size());
	prog_.value(0);

	for (auto& r : results_) {
		if (canceled_.load()) {
	prog_.hide();
	btn_predict.enabled(true);
	tb_filter.enabled(true);

			return;
		}

		prog_.inc();
		prog_.caption(r.name);
		if (to_deg(r.elev_max) >= filter_) {
			lb_predicts.at(0).append({ julian_to_string(r.jd_pass_start, false), r.name, std::format("{:6.1f}",to_deg(r.azm_start)), std::format("{:6.1f}",to_deg(r.elev_start)) });
			lb_predicts.at(0).append({ julian_to_string(r.jd_pass_max, false), r.name, std::format("{:6.1f}",to_deg(r.azm_max)), std::format("{:6.1f}",to_deg(r.elev_max)) });
			lb_predicts.at(0).append({ julian_to_string(r.jd_pass_end, false), r.name, std::format("{:6.1f}",to_deg(r.azm_end)), std::format("{:6.1f}",to_deg(r.elev_end)) });
			auto item = lb_predicts.at(0).append("");
			item->bgcolor(nana::colors::dark_gray);
		}
	}

	prog_.hide();
	btn_predict.enabled(true);
	tb_filter.enabled(true);

	canceled_ = true;
}


