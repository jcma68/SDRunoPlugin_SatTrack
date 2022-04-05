#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <map>
#include <ctime>

#include "SGP4.h"

constexpr auto EARTH_MAJOR_AXIS = 6378137.0;
constexpr auto EARTH_MINOR_AXIS = 6356752.314245;
constexpr auto EARTH_MAJOR_AXIS_KM = EARTH_MAJOR_AXIS / 1000.0;	// in km
constexpr auto EARTH_MINOR_AXIS_KM = EARTH_MINOR_AXIS / 1000.0;
constexpr auto EARTH_AXIS_RATIO = (EARTH_MINOR_AXIS / EARTH_MAJOR_AXIS);
constexpr auto SIDERAL_ROTATION_RATE = (2.0 * M_PI / 86400.0 * 1.002737909350); // rad/s
constexpr auto EARTH_FLAT = (1.0 - EARTH_AXIS_RATIO);
constexpr auto EARTH_RADIUS_KM = EARTH_MAJOR_AXIS / 1000.0;	// WGS 84 Earth radius km
constexpr auto OMEGA_E = 1.00273790934;		// Earth rotations/siderial day
constexpr auto MFACTOR = 7.292115E-5;
constexpr auto AU = 1.49597870691E8;		// Astronomical unit - km (IAU 76)
constexpr auto SR = 6.96000E5;				// Solar radius - km (IAU 76)
constexpr double CVAC = 2.99792458e5; // speed of light [km/s]

template <class T>
inline T sqr(T v) {
	return v * v;
}

template <class T>
inline T cube(T v) {
	return v * v * v;
}

inline double to_rad(double angle) {
	return angle * M_PI / 180.0;
}

inline double to_deg(double angle) {
	return angle * 180.0 / M_PI;
}

double reduce(double value, double rangeMin, double rangeMax);

// julian date, days from 4713 bc
inline double julian_now() {
	constexpr double jan_1970 = 2440587.5;	// January 1, 1970 at midnight (00:00:00) 
	return jan_1970 + time(NULL) / 86400.0; // time() returns an UTC time
}

// Modified julian date since the j2000 epoch (January 1, 2000, at 12:00 TT)
inline double to_j2000(double jd) {
	return jd - 2415020.0;
}

inline double to_j1900(double jd) {
	return jd - 2415019.5;
}

// Greenwich meansidereal time (GMST)
double to_gmst(double jd);

// Local mean sidereal time (GMST plus the observer's longitude)
inline double to_lmst(double jd, double lon) {
	return std::fmod(to_gmst(jd) + lon, 2 * M_PI);
}

struct vector_t {
	double x, y, z;

	vector_t(double nx = 0.0, double ny = 0.0, double nz = 0.0) : x(nx), y(ny), z(nz) {
	}

	vector_t(const double vec[3]) : x(vec[0]), y(vec[1]), z(vec[2]) {
	}

	inline void zero() {
		x = y = z = 0.0;
	}

	inline void set(double nx, double ny, double nz) {
		x = nx;
		y = ny;
		z = nz;
	}

	inline void set(const double vec[3]) {
		x = vec[0];
		y = vec[1];
		z = vec[2];
	}

	inline double dot(const vector_t& v2) const {
		return (x * v2.x + y * v2.y + z * v2.z);
	}

	inline double abs_squared() const {
		return sqr(x) + sqr(y) + sqr(z);
	}

	inline double mag() const {
		return sqrt(sqr(x) + sqr(y) + sqr(z));
	}

	inline vector_t normalized() {
		double d = mag();
		if (d == 0.0)
			return {};

		return vector_t{ x / d, y / d, z / d };
	}

	inline void normalize() {
		double d = mag();
		if (d == 0.0)
			return;

		x /= d;
		y /= d;
		z /= d;
	}

	double angle(const vector_t& v) {

		return (std::acos(this->dot(v) / (this->mag() * v.mag())));
	}

	inline vector_t& operator+=(const vector_t& v) {
		x += v.x;
		y += v.y;
		z += v.z;

		return *this;
	}
};

inline vector_t operator+(const vector_t& L, const vector_t& R) {
	return vector_t{ L.x + R.x, L.y + R.y, L.z + R.z };
}

inline vector_t operator-(const vector_t& L, const vector_t& R) {
	return vector_t{ L.x - R.x, L.y - R.y, L.z - R.z };
}

inline vector_t operator*(const vector_t& L, const double& k) {
	return vector_t{ L.x * k, L.y * k, L.z * k };
}

inline vector_t operator/(const vector_t& L, const double& k) {
	return vector_t{ L.x / k, L.y / k, L.z / k };
}

inline vector_t cross(const vector_t& u, const vector_t& v) {
	return vector_t{
		u.y * v.z - u.z * v.y,
		u.z * v.x - u.x * v.z,
		u.x * v.y - u.y * v.x
	};
}

struct rot_matrix_t {

	rot_matrix_t() :u(1.0, 0.0, 0.0), v(0.0, 1.0, 0.0), w(0.0, 0.0, 1.0) {}
	rot_matrix_t(const vector_t& vu, const vector_t& vv, const vector_t& vw) :u(vu), v(vv), w(vw) {}

	// u perpendicular to vectors vv and vw
	void set_vw(const vector_t& vv, const vector_t& vw) {
		u = cross(vv, vw);
		v = vv;
		w = vw;
	}

	vector_t u;
	vector_t v;
	vector_t w;
};

inline vector_t operator*(const vector_t& v, const rot_matrix_t& m) {
	return vector_t{
		m.u.dot(v),
		m.v.dot(v),
		m.w.dot(v)
	};
}

struct geodetic_t;

// Earth-centered inertial position
struct eci_pos_t {
	eci_pos_t() = default;

	eci_pos_t(const vector_t& p, const vector_t& v)
		: pos(p)
		, vel(v) {

	}

	eci_pos_t(double jd, const geodetic_t& g) {
		update(jd, g);
	}

	void set(const double p[3], const double v[3]) {
		pos.set(p);
		vel.set(v);
	}

	void update(double jd, const geodetic_t& g);

	vector_t pos;
	vector_t vel;
};

struct geodetic_t {
	geodetic_t() = default;

	geodetic_t(double la, double lo, double al)
		: lat(la)
		, lon(lo)
		, alt(al) {
	}

	geodetic_t(double jd, const eci_pos_t& pv) {
		update(jd, pv);
	}

	void update(double jd, const eci_pos_t& pv);

	double lat, lon, alt;
};

struct topocentric_t {
public:
	topocentric_t() = default;

	topocentric_t(double az, double el, double rnge, double rnge_rate)
		: azimuth(az)
		, elevation(el)
		, range(rnge)
		, range_rate(rnge_rate) {
	}

	double azimuth;
	double elevation;
	double range;
	double range_rate;
};

struct observer_t {
	observer_t() = default;

	observer_t(double la, double lo, double al)
		: geo(la, lo, al)
		, eci(julian_now(), geo) {
	}

	void reset(double la, double lo, double al) {
		update(julian_now(), geodetic_t{ la,lo,al });
	}

	void update(double jd, const geodetic_t& g) {
		geo = g;
		eci.update(jd, g);
	}

	topocentric_t get_lookup_angle(double jd, const eci_pos_t& obj);

	geodetic_t geo;
	eci_pos_t eci;
};

struct line_pair {
	std::string l1;
	std::string l2;
};

using tle_map_list = std::map<std::string, line_pair>;

tle_map_list load_tle_file(const std::string& filename);
void parse_tle_lines(const line_pair& tle_data, char opsmode, gravconsttype whichconst, elsetrec& satrec);
eci_pos_t get_sat_pos(double t_since, elsetrec& satrec);

int get_orbit_num(double jd, const elsetrec& satrec);