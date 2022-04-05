#include "sattrack_widget.h"

namespace drawerbase {

	namespace sattrack_widget {

		static nana::color mainAxisColor(69, 158, 231);
		static nana::color gridColor(58, 85, 209);
		static nana::color satActiveColor(255, 238, 34);
		static nana::color satHiddenColor(237, 126, 126);
		static nana::color siteColor(252, 3, 3);
		static nana::color visCircleColor(0, 255, 0);

		constexpr unsigned int margin_top = 20;
		constexpr unsigned int margin_bottom = 20;
		constexpr unsigned int margin_left = 10;
		constexpr unsigned int margin_right = 10;

		constexpr double grid_step_x = 30.0;
		constexpr double grid_step_y = 30.0;

		constexpr int n_grid_x = 5;
		constexpr int n_grid_y = 2;

		constexpr unsigned int half_ground_spot = 3;
		constexpr unsigned int half_sat_spot = 2;

		constexpr unsigned int n_segs_visual_circle = 180;

		constexpr unsigned int n_segs_per_rev = 120;
		constexpr unsigned int n_groud_track_revs = 1;
		constexpr unsigned int n_segs_ground_track = n_segs_per_rev * n_groud_track_revs + 1;

		class sattrack_impl {

			e_map_type map_type{ e_map_type::small_size };

			nana::size world_size{};

			double grid_scale_x;
			double grid_scale_y;

			nana::paint::image img_;

			std::string sat_name{ };
			elsetrec gt_satrec_{};

			std::string site_name;
			observer_t observer_{};

			int orbit_num_{};
			topocentric_t topo_{};
			geodetic_t geo_{};
			double current_jd_{};

			double downlinkFreq{ 137.100000 * 1000000.0 };

			double orbit_time_{ -1.0 };

			struct segment_t {
				short x1, y1;
				short x2, y2;
			};

			segment_t visib_circle_[n_segs_visual_circle];
			segment_t ground_track_[n_segs_ground_track];

		public:
			nana::widget* wdg_ptr{ nullptr };

			sattrack_impl() {}

			void init(e_map_type mt, const std::string& maps_path) {
				orbit_time_ = -1.0;

				map_type = mt;

				switch (map_type) {
				case e_map_type::small_size:
					world_size = nana::size{ 500, 250 };
					break;
				case e_map_type::medium_size:
					world_size = nana::size{ 600, 300 };
					break;
				case e_map_type::large_size:
					world_size = nana::size{ 700, 350 };
					break;
				}

				grid_scale_x = (double)world_size.width / 360.0;
				grid_scale_y = (double)world_size.height / 180.0;

				if (maps_path.empty())
					img_.close();
				else {
					std::string path = (maps_path.back() != '\\') ? maps_path + "\\" : maps_path;
					switch (map_type) {
					case e_map_type::small_size:
						if (!img_.open(path + "world_map_500x250.bmp")) {
						nana::msgbox mb("sattrack_widget");
							mb << path + "world_map_500x250.bmp" << " not found, Please check the " << path << " folder.";
						mb.show();
					}
					break;
				case e_map_type::medium_size:
						if (!img_.open(path + "world_map_600x300.bmp")) {
						nana::msgbox mb("sattrack_widget");
							mb << path + "world_map_600x300.bmp" << " not found, Please check the " << path << " folder.";
						mb.show();
					}
					break;
				case e_map_type::large_size:
						if (!img_.open(path + "world_map_700x350.bmp")) {
						nana::msgbox mb("sattrack_widget");
							mb << path + "world_map_700x350.bmp" << " not found, Please check the " << path << " folder.";
						mb.show();
					}
					break;
				}
				}
			}

			void set_site(const std::string& sitename, const observer_t& obs) {
				site_name = sitename;
				observer_ = obs;
			}

			void set_satellite(const std::string& satname, const elsetrec& satrec) {
				orbit_time_ = -1.0;
				sat_name = satname;
				gt_satrec_ = satrec;
			}

			void set_downlink_freq(double f) {
				downlinkFreq = f;
			}

			void update_state(double jd, int orbit, const topocentric_t& topo, const geodetic_t& geo) {
				current_jd_ = jd;
				orbit_num_ = orbit;
				topo_ = topo;
				geo_ = geo;
			}

			void render(nana::paint::graphics& graph) {
				if (img_.empty())
					return;

				nana::internal_scope_guard lock;

				img_.paste(graph, nana::point{ margin_left, margin_top });

				_draw_frame(graph);
				_draw_grid(graph);
				_draw_site(graph);
				_draw_sat(graph);
			}

		private:
			double _get_doppler_correction_hz() {
				return (topo_.elevation > 0.0) ? downlinkFreq * (1.0 - topo_.range_rate / CVAC) : downlinkFreq;
			}

			void _draw_grid(nana::paint::graphics& graph) {
				int mapCenterX = (int)(margin_left + world_size.width / 2 - 1);
				int mapCenterY = (int)(margin_top + world_size.height / 2 - 1);

				graph.line({ mapCenterX, margin_top }, { mapCenterX, (int)(margin_top + world_size.height - 1) }, mainAxisColor);
				graph.line({ margin_left, mapCenterY }, { (int)(margin_left + world_size.width - 1), mapCenterY }, mainAxisColor);

				int gridStepX = (int)(grid_step_x * grid_scale_x);
				int gridStepY = (int)(grid_step_y * grid_scale_y);

				for (int i = 0; i < n_grid_x; i++) {
					graph.line({ mapCenterX + (int)((double)(i + 1) * gridStepX), margin_top }, { mapCenterX + (int)((double)(i + 1) * gridStepX), (int)(margin_top + world_size.height - 1) }, gridColor);
					graph.line({ mapCenterX - (int)((double)(i + 1) * gridStepX), margin_top }, { mapCenterX - (int)((double)(i + 1) * gridStepX), (int)(margin_top + world_size.height - 1) }, gridColor);
				}

				for (int i = 0; i < n_grid_y; i++) {
					graph.line({ margin_left, mapCenterY + (int)((double)(i + 1) * gridStepY) }, { (int)(margin_left + world_size.width - 1), mapCenterY + (int)((double)(i + 1) * gridStepY) }, gridColor);
					graph.line({ margin_left, mapCenterY - (int)((double)(i + 1) * gridStepY) }, { (int)(margin_left + world_size.width - 1), mapCenterY - (int)((double)(i + 1) * gridStepY) }, gridColor);
				}
			}

			void _draw_frame(nana::paint::graphics& graph) {
				graph.rectangle(nana::rectangle{ 0, 0, margin_left + world_size.width + margin_right, margin_top }, true, nana::colors::black);
				graph.rectangle(nana::rectangle{ 0, (int)(margin_top + world_size.height), margin_left + world_size.width + margin_right, margin_bottom }, true, nana::colors::black);
				graph.rectangle(nana::rectangle{ 0, margin_top, margin_left, world_size.height }, true, nana::colors::black);
				graph.rectangle(nana::rectangle{ (int)(margin_left + world_size.width), (int)margin_top, margin_left, world_size.height }, true, nana::colors::black);

				graph.rectangle(nana::rectangle{ margin_left + 1, margin_top - 15, 9, 9 }, true, topo_.elevation > 0 ? satActiveColor : satHiddenColor);

				int text_pos = (margin_top - graph.text_extent_size(sat_name).height) / 2;
				graph.string(nana::point{ margin_left + 14, text_pos }, sat_name, nana::colors::white);

				std::string s;

				if (map_type != e_map_type::small_size) {
					s = std::format("Orbit: {}", orbit_num_);
					graph.string(nana::point{ (int)(margin_left + 0.18 * world_size.width), text_pos }, s, nana::colors::white);
				}

				switch (map_type) {
				case e_map_type::small_size:
					s = std::format("Azi: {:3.0f}°  Ele: {:3.0f}", to_deg(topo_.azimuth), to_deg(topo_.elevation));
					graph.string(nana::point{ (int)(margin_left + 0.24 * world_size.width), text_pos }, s, nana::colors::white);
					s = std::format("Lat: {:2.0f}° {}  Lng: {:3.0f}° {}", std::abs(to_deg(geo_.lat)), (geo_.lat >= 0.0) ? "N" : "S", std::abs(to_deg(geo_.lon)), (geo_.lon >= 0.0) ? "E" : "W");
					graph.string(nana::point{ (int)(margin_left + 0.43 * world_size.width), text_pos }, s, nana::colors::white);
					break;
				case e_map_type::medium_size:
					s = std::format("Azi: {:3.0f}°  Ele: {:3.0f}", to_deg(topo_.azimuth), to_deg(topo_.elevation));
					graph.string(nana::point{ (int)(margin_left + 0.33 * world_size.width), text_pos }, s, nana::colors::white);
					s = std::format("Lat: {:2.0f}° {}  Lng: {:3.0f}° {}", std::abs(to_deg(geo_.lat)), (geo_.lat >= 0.0) ? "N" : "S", std::abs(to_deg(geo_.lon)), (geo_.lon >= 0.0) ? "E" : "W");
					graph.string(nana::point{ (int)(margin_left + 0.54 * world_size.width), text_pos }, s, nana::colors::white);
					break;
				case e_map_type::large_size:
					s = std::format("Azi: {:5.1f}°  Ele: {:5.1f}", to_deg(topo_.azimuth), to_deg(topo_.elevation));
					graph.string(nana::point{ (int)(margin_left + 0.33 * world_size.width), text_pos }, s, nana::colors::white);
					s = std::format("Lat: {:4.1f}° {}  Lng: {:5.1f}° {}", std::abs(to_deg(geo_.lat)), (geo_.lat >= 0.0) ? "N" : "S", std::abs(to_deg(geo_.lon)), (geo_.lon >= 0.0) ? "E" : "W");
					graph.string(nana::point{ (int)(margin_left + 0.53 * world_size.width), text_pos }, s, nana::colors::white);
					break;
				default:
					break;
				}

				s = std::format("{:%d-%m-%Y %H:%M:%OS}", std::chrono::system_clock::now());
				graph.string(nana::point{ (int)(margin_left + (map_type == e_map_type::small_size ? 0.73 : 0.78) * world_size.width), text_pos }, s, nana::colors::white);

				text_pos = world_size.height + margin_top + (margin_bottom - graph.text_extent_size(sat_name).height) / 2;

				s = std::format("Downlink: {:11.6f} MHz", _get_doppler_correction_hz() / 1000000.0);
				graph.string(nana::point{ margin_left, text_pos }, s, topo_.elevation > 0 ? satActiveColor : satHiddenColor);
			}

			void _draw_site(nana::paint::graphics& graph) {
				if (site_name.empty())
					return;

				nana::point site = _map_location(to_deg(observer_.geo.lat), to_deg(observer_.geo.lon));

				graph.rectangle(nana::rectangle{ site.x - (int)half_ground_spot, site.y - (int)half_ground_spot,(int)(half_ground_spot * 2 + 1), (int)(half_ground_spot * 2 + 1) }, true, siteColor);

				graph.string(nana::point{ site.x + (int)half_ground_spot + 10, site.y - (int)graph.text_extent_size(sat_name).height / 2 }, site_name, nana::colors::white);
			}

			void _draw_sat(nana::paint::graphics& graph) {
				if (sat_name.empty())
					return;

				nana::point sat_loc = _map_location(to_deg(geo_.lat), to_deg(geo_.lon));

				graph.rectangle(nana::rectangle{ (int)(sat_loc.x - half_sat_spot), (int)(sat_loc.y - half_sat_spot), half_sat_spot * 2 + 1, half_sat_spot * 2 + 1 }, true, topo_.elevation > 0 ? satActiveColor : satHiddenColor);

				int x = ((int)(margin_left + world_size.width) - sat_loc.x > 100) ? 10 : -10 - graph.text_extent_size(sat_name).width;
				int y = (sat_loc.y - margin_top < 25) ? 20 : -8;

				graph.string(nana::point{ sat_loc.x + x, sat_loc.y + y }, sat_name, topo_.elevation > 0 ? satActiveColor : satHiddenColor);

				_calc_visib_circle(to_deg(geo_.lat), to_deg(geo_.lon), geo_.alt);
				for (int i = 0; i < n_segs_visual_circle; i++) {
					graph.line({ visib_circle_[i].x1, visib_circle_[i].y1 }, { visib_circle_[i].x2, visib_circle_[i].y2 }, visCircleColor);
				}

				_calc_ground_track();
				for (int i = 0; i < n_segs_ground_track; i++) {
					graph.line({ ground_track_[i].x1, ground_track_[i].y1 }, { ground_track_[i].x2, ground_track_[i].y2 }, topo_.elevation > 0 ? satActiveColor : satHiddenColor);
				}
			}

			nana::point _map_location(double lat, double lon) {
				return nana::point(margin_left + (int)((180.0 + lon) * grid_scale_x - 0.5), margin_top + (int)((90.0 - lat) * grid_scale_y - 0.5));
			}

			void _calc_visib_circle(double circLtd, double circLng, double circHgt) {

				double the = M_PI / 2.0 - to_rad(circLtd);
				double psi = M_PI / 2.0 + to_rad(circLng);

				double cosThe = std::cos(the);
				double sinThe = std::sin(the);
				double cosPsi = std::cos(psi);
				double sinPsi = std::sin(psi);

				rot_matrix_t rot(
					vector_t{ sinPsi * cosThe, -cosPsi, sinPsi * sinThe },
					vector_t{ cosPsi * cosThe,  sinPsi, cosPsi * sinThe },
					vector_t{ -sinThe,           0.0,    cosThe }
				);

				double arg = EARTH_RADIUS_KM / (EARTH_RADIUS_KM + circHgt);
				if (arg > 1.0)
					arg = 1.0;
				if (arg < -1.0)
					arg = -1.0;

				double gamma = std::acos(arg);
				double beta = M_PI / 2.0 - gamma;
				double cosBeta = std::cos(beta);
				double sinBeta = std::sin(beta);

				double circStep = 360.0 / ((double)n_segs_visual_circle);
				for (int k = 0; k < n_segs_visual_circle; k++) {
					double lambda = to_rad(circStep * k);

					vector_t vu{ std::cos(lambda) * cosBeta, std::sin(lambda) * cosBeta, sinBeta };
					vector_t vq = vu * rot;
					vq.normalize();

					arg = vq.z;
					if (arg > 1.0)
						arg = 1.0;
					if (arg < -1.0)
						arg = -1.0;

					double lat = to_deg(std::asin(arg));

					vq.z = 0.0;
					vq.normalize();

					double lng = to_deg(std::atan2(vq.y, vq.x));
					lng = reduce(lng, -180.0, 180.0);

					int posX = (int)((180.0 - lng) * grid_scale_x + margin_left - 0.5);
					int posY = (int)((90.0 - lat) * grid_scale_y + margin_top - 0.5);

					visib_circle_[k].x1 = (short)posX;
					visib_circle_[k].y1 = (short)posY;

					if (k > 0) {
						visib_circle_[k - 1].x2 = visib_circle_[k].x1;
						visib_circle_[k - 1].y2 = visib_circle_[k].y1;
					}
				}

				visib_circle_[n_segs_visual_circle - 1].x2 = visib_circle_[0].x1;
				visib_circle_[n_segs_visual_circle - 1].y2 = visib_circle_[0].y1;

				_clean_segments(&visib_circle_[0], n_segs_visual_circle, true);
			}

			void _clean_segments(segment_t* segs, int count, bool visibCircleFlag) {
				int k, k0, n, dx, dy, dX, dY;

				k0 = (visibCircleFlag) ? 0 : -1;

				for (k = 0; k < count + k0; k++) {
					n = (visibCircleFlag && k == count - 1) ? 0 : k + 1;

					if (segs[k].x1 - segs[k].x2 > (short) (world_size.width / 2)) {
						dx = (int)(world_size.width + margin_left) - (int)segs[k].x1;
						dX = (int)segs[k].x2 - (int)margin_left + dx;

						dY = (int)(segs[k].y2 - segs[k].y1);

						if (dX != 0)
							dy = (int)((double)dY / (double)dX * (double)dx);
						else
							dy = 0;

						segs[n].x1 = segs[k].x2 - (short)(dX - dx);
						segs[k].x2 = segs[k].x1 + (short)dx;

						segs[n].y1 = segs[k].y2 - (short)(dY - dy);
						segs[k].y2 = segs[k].y1 + (short)dy;
					}

					if (segs[k].x2 - segs[k].x1 > (short)(world_size.width / 2)) {
						dx = (int)segs[k].x1 - (int)margin_left;
						dX = (int)(world_size.width + margin_left) - (int)segs[k].x2 + dx;

						dY = (int)(segs[k].y2 - segs[k].y1);

						if (dX != 0)
							dy = (int)((double)dY / (double)dX * (double)dx);
						else
							dy = 0;

						segs[n].x1 = segs[k].x2 + (short)(dX - dx);
						segs[k].x2 = segs[k].x1 - (short)dx;

						segs[n].y1 = segs[k].y2 - (short)(dY - dy);
						segs[k].y2 = segs[k].y1 + (short)dy;
					}
				}

				for (k = 0; k < count; k++) {
					segs[k].x1 = std::min(std::max(segs[k].x1, (short)margin_left), (short)(world_size.width + margin_left - 1));
					segs[k].x2 = std::min(std::max(segs[k].x2, (short)margin_left), (short)(world_size.width + margin_left - 1));

					segs[k].y1 = std::min(std::max(segs[k].y1, (short)margin_top), (short)(world_size.height + margin_top - 1));
					segs[k].y2 = std::min(std::max(segs[k].y2, (short)margin_top), (short)(world_size.height + margin_top - 1));

				}
			}

			void _calc_ground_track() {

				if (gt_satrec_.error)
					return;

				double jd = current_jd_;
				double epochMeanMotion = gt_satrec_.no_kozai * 1440 / (2.0 * M_PI);			// (rev/day)

				if (jd - orbit_time_ > 1.0 / epochMeanMotion || orbit_time_ < 0.0) {

					orbit_time_ = jd;

					double tle_date = gt_satrec_.jdsatepoch + gt_satrec_.jdsatepochF;
					double tmpTime = (tle_date > jd) ? tle_date : jd - (1.0 / epochMeanMotion) / 20.0;

					for (int k = 0; k < n_segs_ground_track; k++) {

						eci_pos_t sat = get_sat_pos((tmpTime - tle_date) * 1440, gt_satrec_);
						if (gt_satrec_.error != 0)
							return;

						topocentric_t topo = observer_.get_lookup_angle(tmpTime, sat);
						geodetic_t geo(tmpTime, sat);

						ground_track_[k].x1 = (short)((180.0 + to_deg(geo.lon)) * grid_scale_x + margin_left - 0.5);
						ground_track_[k].y1 = (short)((90.0 - to_deg(geo.lat)) * grid_scale_y + margin_top - 0.5);
						if (k > 0) {
							ground_track_[k - 1].x2 = ground_track_[k].x1;
							ground_track_[k - 1].y2 = ground_track_[k].y1;
						}

						tmpTime += (1.0 / epochMeanMotion) / (double)n_segs_per_rev;
					}

					ground_track_[n_segs_ground_track - 1].x2 = ground_track_[n_segs_ground_track - 1].x1;
					ground_track_[n_segs_ground_track - 1].y2 = ground_track_[n_segs_ground_track - 1].y1;

					_clean_segments(&ground_track_[0], n_segs_ground_track, false);
				}
			}
		};

		drawer::drawer() :impl_(new sattrack_impl) {
		}

		drawer::~drawer() {
			delete impl_;
		}

		void drawer::attached(widget_reference wdg, graph_reference) {
			impl_->wdg_ptr = &wdg;
			nana::API::ignore_mouse_focus(wdg, true);
		}

		void drawer::refresh(graph_reference graph) {
			graph.rectangle(true, impl_->wdg_ptr->bgcolor());

			impl_->render(graph);
		}
	}
}

sattrack_widget::sattrack_widget(nana::window wd, const std::string& maps_path, nana::point pos, e_map_type mt) {
	this->create(wd, true);

	nana::internal_scope_guard lock;

	get_drawer_trigger().impl()->init(mt, maps_path);

	move(pos);
	size(calc_window_size(mt));

	update_.interval(std::chrono::seconds{ 2 });
	update_.elapse([this]() {
		_on_timer();
				   });
}

sattrack_widget::~sattrack_widget() {
	stop();
}

nana::size sattrack_widget::calc_window_size(e_map_type mt) {
	using namespace drawerbase::sattrack_widget;

	switch (mt) {
	case e_map_type::small_size:
		return nana::size({ 500 + margin_left + margin_right,  250 + margin_top + margin_bottom });
	case e_map_type::medium_size:
		return nana::size({ 600 + margin_left + margin_right,  300 + margin_top + margin_bottom });
	case e_map_type::large_size:
		return nana::size({ 700 + margin_left + margin_right, 350 + margin_top + margin_bottom });
	default:
		return nana::size({ 500 + margin_left + margin_right,  250 + margin_top + margin_bottom });
	}
}

void sattrack_widget::set_map(e_map_type mt, const std::string& maps_path) {
	nana::internal_scope_guard lock;

	get_drawer_trigger().impl()->init(mt, maps_path);

	size(calc_window_size(mt));
}

void sattrack_widget::set_satellite(const std::string& satname, const elsetrec& satrec) {
	satrec_ = satrec;

	nana::internal_scope_guard lock;

	get_drawer_trigger().impl()->set_satellite(satname, satrec_);
}

void sattrack_widget::set_site(const std::string& sitename, double lat, double lng, double ht) {
	observer_.reset(to_rad(lat), to_rad(lng), ht / 1000.0);

	nana::internal_scope_guard lock;

	get_drawer_trigger().impl()->set_site(sitename, observer_);
}

void sattrack_widget::set_downlink_freq(double f) {
	downlinkFreq = f;

	nana::internal_scope_guard lock;

	get_drawer_trigger().impl()->set_downlink_freq(downlinkFreq);
}

void sattrack_widget::start() {
	_calc_pos();

	update_.start();
}

void sattrack_widget::stop() {
	update_.stop();
}

double sattrack_widget::get_doppler_correction_hz() {
	nana::internal_scope_guard lock;

	return (topo_.elevation > 0.0) ? downlinkFreq * (1.0 - topo_.range_rate / CVAC) : downlinkFreq;
}

void sattrack_widget::_calc_pos() {
	if (satrec_.error != 0)
		return;

	double jd = julian_now();
	double t_since = (jd - (satrec_.jdsatepoch + satrec_.jdsatepochF)) * 1440;

	sat_ = get_sat_pos(t_since, satrec_);
	if (satrec_.error != 0)
		return;

	topo_ = observer_.get_lookup_angle(jd, sat_);
	geo_.update(jd, sat_);

	int orbit_num = get_orbit_num(jd, satrec_);

	nana::internal_scope_guard lock;

	get_drawer_trigger().impl()->update_state(jd, orbit_num, topo_, geo_);
}

