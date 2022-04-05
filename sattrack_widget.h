#pragma once

#include <nana/gui.hpp>
#include <nana/gui.hpp>
#include <nana/gui/dragger.hpp>
#include <nana/gui/drawing.hpp>
#include <nana/paint/image.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <nana/gui/timer.hpp>

#include "sat_calc.h"

class sattrack_widget;

enum class e_map_type {
	small_size, medium_size, large_size
};

constexpr auto e_map_type_to_int(e_map_type e) noexcept {
	return static_cast<std::underlying_type_t<e_map_type>>(e);
}

namespace drawerbase {

	namespace sattrack_widget {

		class sattrack_impl;

		class drawer : public nana::drawer_trigger {
			friend class sattrack_widget;
		public:
			drawer();
			~drawer();


			void attached(widget_reference, graph_reference) override;

			sattrack_impl* impl() const {
				return impl_;
			}

		private:
			void refresh(graph_reference)	override;
		private:
			sattrack_impl* const impl_;

		};

	}
}

class sattrack_widget : public nana::widget_object<nana::category::widget_tag, drawerbase::sattrack_widget::drawer> {
public:
	sattrack_widget(nana::window wd, const std::string& maps_path = {}, nana::point pos = {}, e_map_type mt = e_map_type::small_size);
	~sattrack_widget();

	static nana::size calc_window_size(e_map_type mt);

	void set_map(e_map_type mt, const std::string& maps_path);

	void set_satellite(const std::string& satname, const elsetrec& satrec);

	void set_site(const std::string& sitename, double lat, double lng, double ht);

	void set_downlink_freq(double f);

	double get_doppler_correction_hz();

	void start();
	void stop();

private:
	nana::timer update_;

	void _calc_pos();

	void _on_timer() {
		_calc_pos();
		nana::API::refresh_window(*this);
	}

	double downlinkFreq{ 137.100000 * 1000000.0 };

	elsetrec satrec_{};
	observer_t observer_{};
	eci_pos_t sat_{};
	topocentric_t topo_{};
	geodetic_t geo_{};
};