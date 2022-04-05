#include "sat_calc.h"
#include <time.h>
#include <fstream>

// Convert to greenwich mean sidereal time
double to_gmst(double jd) {
	/* Reference:  The 1992 Astronomical Almanac, page B6. */

	double intpart;
	double UT = std::modf(jd + 0.5, &intpart);

	jd = jd - UT;
	double TU = (jd - 2451545.0) / 36525;
	double GMST = 24110.54841 + TU * (8640184.812866 + TU * (0.093104 - TU * 6.2E-6));
	GMST = std::fmod(GMST + 86400.0 * 1.00273790934 * UT, 86400.0);

	return (2 * M_PI * GMST / 86400.0);
}

double reduce(double value, double rangeMin, double rangeMax) {

	double range = rangeMax - rangeMin;
	double fullRanges;
	std::modf((rangeMax - value) / range, &fullRanges);

	double retval = value + fullRanges * range;
	if (retval > rangeMax)
		retval -= range;

	return retval;
}

void eci_pos_t::update(double jd, const geodetic_t& g) {
	double theta = to_lmst(jd, g.lon);

	double c = 1.0 / std::sqrt(1.0 + EARTH_FLAT * (EARTH_FLAT - 2.0) * sqr(std::sin(g.lat)));
	double sq = sqr(1 - EARTH_FLAT) * c;
	double achcp = (EARTH_RADIUS_KM * c + g.alt) * std::cos(g.lat);

	pos.set(achcp * std::cos(theta), achcp * std::sin(theta), (EARTH_RADIUS_KM * sq + g.alt) * std::sin(g.lat));
	vel.set(-MFACTOR * pos.y, MFACTOR * pos.x, 0.0);
}

void geodetic_t::update(double jd, const eci_pos_t& pv) {
	double theta = std::atan2(pv.pos.y, pv.pos.x);

	lon = reduce(theta - to_gmst(jd), -M_PI, M_PI);

	double r = std::sqrt(sqr(pv.pos.x) + sqr(pv.pos.y));
	double e2 = EARTH_FLAT * (2 - EARTH_FLAT);
	lat = std::atan2(pv.pos.z, r);

	double phi, c;
	for (int i = 0; i < 10; i++) {
		phi = lat;
		c = 1.0 / std::sqrt(1.0 - e2 * sqr(std::sin(phi)));
		lat = std::atan2(pv.pos.z + EARTH_RADIUS_KM * c * e2 * std::sin(phi), r);
		if (std::fabs(lat - phi) < 1E-10)
			break;
	}

	alt = r / std::cos(lat) - EARTH_RADIUS_KM * c; // kilometers

	if (lat > M_PI / 2.0)
		lat -= 2 * M_PI;
}

topocentric_t observer_t::get_lookup_angle(double jd, const eci_pos_t& obj) {
	eci.update(jd, geo);

	vector_t range = obj.pos - eci.pos;
	vector_t rgvel = obj.vel - eci.vel;

	double theta = to_lmst(jd, geo.lon);

	double sin_lat = std::sin(geo.lat);
	double cos_lat = std::cos(geo.lat);

	double sin_theta = std::sin(theta);
	double cos_theta = std::cos(theta);

	double top_s = sin_lat * cos_theta * range.x + sin_lat * sin_theta * range.y - cos_lat * range.z;
	double top_e = -sin_theta * range.x + cos_theta * range.y;
	double top_z = cos_lat * cos_theta * range.x + cos_lat * sin_theta * range.y + sin_lat * range.z;
	double azim = std::atan(-top_e / top_s);

	if (top_s > 0.0)
		azim = azim + M_PI;

	if (azim < 0.0)
		azim = azim + 2.0 * M_PI;

	double el = std::asin(top_z / range.mag());

	return topocentric_t(azim, el, range.mag(), range.dot(rgvel) / range.mag());
}

static bool tle_checksum(const std::string& buff) {
	int cksum = 0;

	if (buff.size() < 69)
		return false;

	if ((buff[0] != '1' && buff[0] != '2') || buff[1] != ' ')
		return false;

	for (int i = 0; i < 68; i++) {
		char c = buff[i];
		if (c == ' ' || c == '.' || c == '+' || std::isalpha(c))
			continue;

		if (std::isdigit(c))
			cksum += c - '0';
		else if (c == '-')
			cksum++;
	}

	return ((cksum % 10) == (buff[68] - '0'));
}

#if 0
double getElement(char* gstr, int gstart, int gstop) {
	double retval;
	int    k, glength;
	char   gestr[80];

	glength = gstop - gstart + 1;

	for (k = 0; k <= glength; k++)
		gestr[k] = gstr[gstart + k - 1];

	gestr[glength] = '\0';

	retval = atof(gestr);
	return(retval);
}

int readTle(int satNameFlag, char* satNameTle, long* pSatNum, int* pElementSet,
			double* pEpochDay, double* pInclination, double* pRaan, double* pEccentricity,
			double* pArgPerigee, double* pMeanAnomaly, double* pMeanMotion,
			double* pDecayRate, double* pDecayRateDot, double* pbStarCoeff,
			long* pOrbitNum, int* pEphemerisType) {
	double epochYear, exponent, atofSatNum, checkLow, checkHigh;
	long   strSatNum;
	long   tleSatNum = 0L;
	long   saveSatNum = 0L;
	int    error, satNameLen, checkSum, checkValue, takeSatNum;
	int    i, j, m, n;
	int    tleIndex = 0;
	char   line0[80], line1[80], line2[80], str[80], strng[10], saveStr[80];
	char   tleStr[80];

	satNameLen = strlen(satNameTle);
	atofSatNum = atof(satNameTle);

	checkLow = pow(10.0, (double)(satNameLen - 1)) - ONEPPM;
	checkHigh = pow(10.0, (double)(satNameLen)) - 1.0 + ONEPPM;

	takeSatNum = (atofSatNum > checkLow && atofSatNum < checkHigh) ? TRUE : FALSE;

	if (takeSatNum)
		satNameFlag = FALSE;

	upperCase(satNameTle);

	error = FALSE;
	m = 0;

	for (i = 0; i < numTle; i++)               /* check for ambiguous entries */
	{
		if (m == 1) {
			strcpy(saveStr, tleStr);
			saveSatNum = tleSatNum;
		}

		strcpy(str, tle[i].tleLine0);
		upperCase(str);
		strSatNum = tle[i].tleSatNum;

		n = (satNameFlag) ? satNameLen : strlen(str);

		if ((!takeSatNum && n == satNameLen &&
			 !strncmp(satNameTle, str, (unsigned int)satNameLen)) ||
			(takeSatNum && (long)(atofSatNum + ONEPPM) == strSatNum)) {
			strcpy(tleStr, tle[i].tleLine0);
			tleSatNum = tle[i].tleSatNum;

			if (m == 0)
				tleIndex = i;

			if (m == 1)
				printf("\n%5ld:  %s\n", saveSatNum, saveStr);

			if (m >= 1)
				printf("%5ld:  %s\n", tleSatNum, tleStr);

			m++;
		}
	}

	if (m == 0)                                        /* satellite not found */
		return(3);

	if (m > 1)                                         /* multiple entries    */
		return(4);

	strcpy(line0, tle[tleIndex].tleLine0);              /* continue if m = 1   */
	truncBlanks(line0);                                /* (only one entry)    */

	strcpy(satNameTle, line0);
	strcpy(line1, tle[tleIndex].tleLine1);                      /* read line 1 */
	strcpy(line2, tle[tleIndex].tleLine2);                      /* read line 2 */



	exponent = getElement(line1, 51, 52);
	*pDecayRateDot = getElement(line1, 45, 50) * pow(10.0, -5.0 + exponent);

	exponent = getElement(line1, 60, 61);
	*pbStarCoeff = getElement(line1, 54, 59) * pow(10.0, -5.0 + exponent);

	return(0);
}

#endif

//  XXXXXXXXXXX                                                              
//  1 AAAAAU 00  0  0 BBBBB.BBBBBBBB +.CCCCCCCC +DDDDD-D +EEEEE-E F  GGGZ    
//  2 AAAAA HHH.HHHH III.IIII JJJJJJJ KKK.KKKK LLL.LLLL MM.MMMMMMMMNNNNNZ    
//                                                                           
//  X = Satellite name                                                       
//  A = Catalog number                                                       
//  B = Epoch time                                                           
//  C = One half of first derivative of mean motion (decay rate)             
//  D = One sixth of second derivative of mean motion                        
//  E = BSTAR drag coefficient                                               
//  F = Ephemeris type                                                       
//  G = Number of the element set                                            
//  H = Inclination                                                          
//  I = RAAN                                                                 
//  J = Eccentricity                                                         
//  K = Argument of perigee                                                  
//  L = Mean anomaly                                                         
//  M = Mean motion                                                          
//  N = Orbit number                                                         
//  Z = Check sum (modulo 10)                                                
//                                                                           
//                                                                           
//  Line 0 is an eleven-character name. Lines 1 and 2 are the standard       
//  two-line orbital element set format identical to that used by NASA and   
//  NORAD. The format description is as follows:                             
//                                                                           
//  Line 0:                                                                  
//  Column     Description                                                   
//    1-11     Satellite name                                                
//                                                                           
//  Line 1:                                                                  
//    1- 1     Line number of element set                                    
//    3- 7     Satellite number                                              
//    8- 8     Classification (U = unclassified)                             
//   10-11     International designator (last two digits of launch year)     
//   12-14     International designator (launch number of the year)          
//   15-17     International designator (piece of launch)                    
//   19-20     Epoch year (last two digits of year)                          
//   21-32     Epoch (Julian day and fractional portion of the day)          
//   34-43     One half of first time derivative of mean motion (decay rate) 
//             or ballistic coefficient (depending on ephemeris type)        
//   45-52     One sixth of second time derivative of mean motion            
//             (decimal point assumed; blank if n/a)                         
//   54-61     BSTAR drag coefficient if SGP4/SGP8 general perturbation      
//             theory used; otherwise, radiation pressure coefficient        
//             (decimal point assumed)                                       
//   63-63     Ephemeris type                                                
//   66-68     Element set number                                            
//   69-69     Check sum (modulo 10)                                         
//             (letters, blanks, periods, plus sign = 0; minus sign = 1)     
//                                                                           
//  Line 2:                                                                  
//    1- 1     Line number of element set                                    
//    3- 7     Satellite number                                              
//    9-16     Inclination [deg]                                             
//   18-25     Right Ascension of the ascending node [deg]                   
//   27-33     Eccentricity (decimal point assumed)                          
//   35-42     Argument of perigee [deg]                                     
//   44-51     Mean anomaly [deg]                                            
//   53-63     Mean motion [rev/d]                                           
//   64-68     Orbit (revolution number) at epoch [rev]                      
//   69-69     Check sum (modulo 10)                                         
//                                                                           
//  All other columns are blank or fixed.                                    

inline void get_element_str(char* dst, const std::string src, int start, int end) {
	auto clen = src.copy(dst, end - start + 1, start - 1);
	dst[clen] = '\0';
}

inline int get_element_int(const std::string src, int start, int end) {
	return std::stoi(src.substr(start - 1, end - start + 1));
}

inline double get_element_double(const std::string src, int start, int end) {
	return std::stod(src.substr(start - 1, end - start + 1));
}

inline double get_element_double_exp(const std::string src, int start) {
	std::string n = src.substr(start - 1,1) + "0." + src.substr(start - 1 + 1, 5) + "e" + src.substr(start - 1 + 6, 2);

	return std::stod(n);
}

inline double get_element_impldec(const std::string src, int start, int end) {
	std::string n = "0." + src.substr(start - 1, end - start + 1);

	return std::stod(n);
}

void parse_tle_lines(const line_pair& tle_data, char opsmode, gravconsttype whichconst, elsetrec& satrec) {

	satrec.error = 0;

	std::string longstr1 = tle_data.l1;
	std::string longstr2 = tle_data.l2;

	int cardnumb = longstr1[0] - '0';
	get_element_str(satrec.satnum, longstr1, 3, 7);
	satrec.classification = longstr1[8 - 1];
	get_element_str(satrec.intldesg, longstr1, 10, 17);
	satrec.epochyr = get_element_int(longstr1, 19, 20);
	satrec.epochdays = get_element_double(longstr1, 21, 32);
	satrec.ndot = get_element_double(longstr1, 34, 43);
	satrec.nddot = get_element_double_exp(longstr1, 45); 
	satrec.bstar = get_element_double_exp(longstr1, 54); 
	satrec.ephtype = longstr1[63-1] - '0';
	satrec.elnum = get_element_int(longstr1, 65, 68);
	char chec_sum = longstr1[69 - 1];


	cardnumb = longstr2[0] - '0';
	get_element_str(satrec.satnum, longstr2, 3, 7);
	satrec.inclo = get_element_double(longstr2, 9, 16);
	satrec.nodeo = get_element_double(longstr2, 18, 25);
	satrec.ecco = get_element_impldec(longstr2, 27, 33);
	satrec.argpo = get_element_double(longstr2, 35, 42);
	satrec.mo = get_element_double(longstr2, 44, 51);
	satrec.no_kozai = get_element_double(longstr2, 53, 63);	
	satrec.revnum = (long)get_element_int(longstr2, 64, 68);
	chec_sum = longstr2[69 - 1];


	const double xpdotp = 1440.0 / (2.0 * M_PI);

	// satnum : Satellite catalog number
	// ecco   : Eccentricity
	// revnum : Revolution number at epoch
	
	// ---- find no, ndot, nddot ----
	satrec.no_kozai = satrec.no_kozai / xpdotp;				// Epoch mean motion (rad/min)


	// ---- convert to sgp4 units ----
	satrec.ndot = satrec.ndot / (xpdotp * 1440.0);			// First derivative of mean motion (rad/min^2)
	satrec.nddot = satrec.nddot / (xpdotp * 1440.0 * 1440);	// Second derivative of mean motion (rad/min^3)

	// ---- find standard orbital elements ----
	satrec.inclo = to_rad(satrec.inclo);					// Inclination (rad)
	satrec.nodeo = to_rad(satrec.nodeo);					// Right ascension of the ascending node (rad)
	satrec.argpo = to_rad(satrec.argpo);					// Argument of perigee (rad)
	satrec.mo = to_rad(satrec.mo);							// Mean anomaly (rad)

	int year = 0;
	if (satrec.epochyr < 57)
		year = satrec.epochyr + 2000;
	else
		year = satrec.epochyr + 1900;

	double sec;
	int mon, day, hr, minute;
	SGP4Funcs::days2mdhms_SGP4(year, satrec.epochdays, mon, day, hr, minute, sec);
	SGP4Funcs::jday_SGP4(year, mon, day, hr, minute, sec, satrec.jdsatepoch, satrec.jdsatepochF);

	// ---------------- initialize the orbit at sgp4epoch -------------------
	SGP4Funcs::sgp4init(whichconst, opsmode, satrec.satnum, (satrec.jdsatepoch + satrec.jdsatepochF) - 2433281.5, satrec.bstar,
						satrec.ndot, satrec.nddot, satrec.ecco, satrec.argpo, satrec.inclo, satrec.mo, satrec.no_kozai,
						satrec.nodeo, satrec);
}

// TODO: Ignore inactive satellites, remvove [-], [D]
// Operational Status	Descriptions
// [+]					Operational
// [-]					Nonoperational
// [P]					Partially Operational
// 						Partially fulfilling primary mission or secondary mission(s)
// [B]					Backup/Standby
// 						Previously operational satellite put into reserve status
// [S]					Spare
// 						New satellite awaiting full activation
// [X]					Extended Mission
// [D]					Decayed
// ?					Unknown
// *Active is any satellite with an operational status of +, P, B, S, or X.
// 
std::map<std::string, line_pair> load_tle_file(const std::string& filename) {
	std::ifstream in(filename.c_str());
	std::map<std::string, line_pair>  list;

	if (!in) {
		std::cerr << "Cannot open the File : " << filename << std::endl;
		return list;
	}

	std::string str;
	line_pair lp{};
	while (true) {
		if (!std::getline(in, str))
			break;

		std::string name = str;
		name.erase(std::find_if(name.rbegin(), name.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), name.end());

		if (name.size() == 0 || str[0] == '1' || str[0] == '2')
			continue;

		if (!std::getline(in, str))
			break;

		if (str[0] != '1')
			continue;

		lp.l1 = str;

		if (!std::getline(in, str))
			break;

		if (str[0] != '2')
			continue;

		lp.l2 = str;

		if (!tle_checksum(lp.l1.data()) || !tle_checksum(lp.l2.data())) 
			continue;

		list[name] = lp;
	}

	in.close();

	return list;
}

eci_pos_t get_sat_pos(double t_since, elsetrec& satrec) {
	if (satrec.init != 'n') {
		satrec.error = 7;	// custom : not initialzed

		return eci_pos_t{};
	}

	double sat_vel[3], sat_pos[3];
	bool res = SGP4Funcs::sgp4(satrec, t_since, sat_pos, sat_vel);
	if (!res)
		return eci_pos_t{};

	return eci_pos_t{ sat_pos, sat_vel };
}

int get_orbit_num(double jd, const elsetrec& satrec) {

	double dT = (jd - (satrec.jdsatepoch + satrec.jdsatepochF));	// (days)
	double dT2 = dT * dT;	// (day^2)

	double epochMeanMotion = satrec.no_kozai * 1440 / (2.0 * M_PI);			// (rev/day)
	double decayRate = satrec.ndot * 1440 * 1440 / (2.0 * M_PI);			// (rev/day^2)
	double decayRateDot = satrec.nddot * 1440 * 1440 * 1440 / (2.0 * M_PI);	// (rev/day^3)
	double curMotion = epochMeanMotion + 2.0 * decayRate * dT + 6.0 * decayRateDot * dT2;	// current number of rev/day
	double refOrbit = (double)satrec.revnum + satrec.mo / (2.0*M_PI);		 // (rev)

	return (int)(refOrbit + curMotion * dT);
}

