// Copyright 2017 Google Inc.

// Based on equations here:
//   https://microem.ru/files/2012/08/GPS.G1-X-00006.pdf

#include "TangoEcefUtils.h"
#include "CoreMinimal.h"

#define CHECK_NOTNULL(x) check (x)
#define CHECK_LE(x, y) check (x < y)
#define CHECK_GE(x, y) check (x > y)

namespace ecef_tools
{

static const double a = 6378137.0;           // meters
static const double b = 6356752.314245;      // meters
static const double e2_a = (a*a - b*b)/(a*a);  // no units, also 2*f - f^2
static const double e2_b = (a*a - b*b)/(b*b);  // no units

// Minimum numerical vector length (arbitrary units) for angle via atan2.
static constexpr double kEpsilonSquared = 1e-9;

/**
 * Use X-Y values of a vector to compute heading angle on the range [0, 2*PI).
 * The heading is measured *clockwise* from the +Y axis. A negative result
 * indicates failure (e.g. due to zero-length vector).
 * @return Clockwise angle between +Y axis vector(x, y).
 */
static double ComputeHeadingFromVector(const double x, const double y) {
  if (x * x + y * y > kEpsilonSquared) {
    // Heading is measured clockwise from Y, so we do atan(X/Y) here.
    double heading = atan2(x, y);
    if (heading < 0) {
      heading += 2 * PI;  // Ensure range [0, 2 * PI)
    }
    return heading;
  } else {
    // Use negative value to denote failure (due to device pointed up or down).
    return -1.0;
  }
}

static bool QuaternionIsUnitLength(const double* q) {
  CHECK_NOTNULL(q);
  double norm_squared = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
  constexpr double kEpsilon = 1e-4;
  return (norm_squared < 1.0 + kEpsilon && norm_squared > 1.0 - kEpsilon);
}

void LocationToPositionInAdf(const Location& point_location, const Location& adf_location, double* adf_p_point)
{
	CHECK_NOTNULL(adf_p_point);
	double ecef_p_adf[3], ecef_q_adf[4];
	LocationToAdfPoseInEcef(adf_location, ecef_p_adf, ecef_q_adf);
	LocationToPositionInAdf(point_location, ecef_p_adf, ecef_q_adf, adf_p_point);
}

void LocationToPositionInAdf(const Location& point_location, const double* ecef_p_adf, const double* ecef_q_adf, double* adf_p_point)
{
	CHECK_NOTNULL(ecef_p_adf);
	CHECK_NOTNULL(ecef_q_adf);
	CHECK_NOTNULL(adf_p_point);
	double ecef_p_point[3];
	LocationToPositionInEcef(point_location.latitude, point_location.longitude, point_location.altitude, ecef_p_point);

	double adf_p_point_in_ecef[3];
	adf_p_point_in_ecef[0] = ecef_p_point[0] - ecef_p_adf[0];
	adf_p_point_in_ecef[1] = ecef_p_point[1] - ecef_p_adf[1];
	adf_p_point_in_ecef[2] = ecef_p_point[2] - ecef_p_adf[2];

	double ecef_R_adf[9], adf_R_ecef[9];
	QuaternionToRotationMatrix(ecef_q_adf, ecef_R_adf);
	GetMatrix3x3Transpose(ecef_R_adf, adf_R_ecef);
	ExpressVector3(adf_R_ecef, adf_p_point_in_ecef, adf_p_point);
}

void PositionInAdfToLocation(const double* adf_p_point,const Location& adf_location,Location* point_location)
{
	CHECK_NOTNULL(adf_p_point);
	double ecef_p_adf[3], ecef_q_adf[4];
	LocationToAdfPoseInEcef(adf_location, ecef_p_adf, ecef_q_adf);
	PositionInAdfToLocation(adf_p_point, ecef_p_adf, ecef_q_adf,point_location);
}

void PositionInAdfToLocation(const double* adf_p_point,const double* ecef_p_adf,const double* ecef_q_adf,Location* point_location)
{
	CHECK_NOTNULL(adf_p_point);
	CHECK_NOTNULL(ecef_p_adf);
	CHECK_NOTNULL(ecef_q_adf);
	CHECK_NOTNULL(point_location);

	double ecef_R_adf[9];
	QuaternionToRotationMatrix(ecef_q_adf, ecef_R_adf);
	double adf_p_point_in_ecef[3];
	ExpressVector3(ecef_R_adf, adf_p_point, adf_p_point_in_ecef);
	double ecef_p_point[3];
	ecef_p_point[0] = ecef_p_adf[0] + adf_p_point_in_ecef[0];
	ecef_p_point[1] = ecef_p_adf[1] + adf_p_point_in_ecef[1];
	ecef_p_point[2] = ecef_p_adf[2] + adf_p_point_in_ecef[2];

	PositionInEcefToLocation(ecef_p_point, &point_location->latitude, &point_location->longitude, &point_location->altitude);
	point_location->heading = 0;
}

void LocationToAdfPoseInEcef(const Location& adf_location,double* ecef_p_adf,double* ecef_q_adf)
{
	// Position.
	LocationToPositionInEcef(adf_location.latitude, adf_location.longitude,adf_location.altitude, ecef_p_adf);
	// Orientation.
	double ecef_R_map[9];
	GetRotation_ECEF_R_ADF(adf_location.latitude, adf_location.longitude,adf_location.heading, ecef_R_map);
	RotationMatrixToQuaternion(ecef_R_map, ecef_q_adf);
}

double DeviceOrientationInAdfToHeadingInAdf(const double* adf_q_device)
{
	CHECK_NOTNULL(adf_q_device);
	double adf_R_device[9];
	QuaternionToRotationMatrix(adf_q_device, adf_R_device);
	double forward[3] = {-adf_R_device[2], -adf_R_device[5], -adf_R_device[8]};
	return ComputeHeadingFromVector(forward[0], forward[1]);
}

void DevicePoseInEcefToLocation(const double* ecef_p_device,const double* ecef_q_device,Location* device_location)
{
	CHECK_NOTNULL(ecef_p_device);
	CHECK_NOTNULL(ecef_q_device);
	CHECK_NOTNULL(device_location);

	// Position to Lat, lng, Alt.
	double lat, lng, alt;
	PositionInEcefToLocation(ecef_p_device, &lat, &lng, &alt);

	// Device Orientation to heading
	double ecef_R_device[9], ecef_R_enu[9], enu_R_ecef[9], enu_R_device[9];
	QuaternionToRotationMatrix(ecef_q_device, ecef_R_device);
	GetRotation_ECEF_R_ENU(lat, lng, ecef_R_enu);
	GetMatrix3x3Transpose(ecef_R_enu, enu_R_ecef);
	MultiplyMatrix3x3(enu_R_ecef, ecef_R_device, enu_R_device);
	double forward[3] = {-enu_R_device[2], -enu_R_device[5], -enu_R_device[8]};
	double heading = ComputeHeadingFromVector(forward[0], forward[1]);

	device_location->latitude  = lat;
	device_location->longitude = lng;
	device_location->heading   = heading;
	device_location->altitude  = alt;
}

bool AdfPoseInEcefToLocation(const double* ecef_p_adf,const double* ecef_q_adf,Location* adf_location)
{
	CHECK_NOTNULL(ecef_p_adf);
	CHECK_NOTNULL(ecef_q_adf);
	CHECK_NOTNULL(adf_location);

	if (!QuaternionIsUnitLength(ecef_q_adf))
	{
		return false;
	}

	// Position to Lat, lng, Alt.
	double lat, lng, alt;
	PositionInEcefToLocation(ecef_p_adf, &lat, &lng, &alt);

	// Device Orientation to heading
	double ecef_R_adf[9], ecef_R_enu[9], enu_R_ecef[9], enu_R_adf[9];
	QuaternionToRotationMatrix(ecef_q_adf, ecef_R_adf);
	GetRotation_ECEF_R_ENU(lat, lng, ecef_R_enu);
	GetMatrix3x3Transpose(ecef_R_enu, enu_R_ecef);
	MultiplyMatrix3x3(enu_R_ecef, ecef_R_adf, enu_R_adf);

	double north_dot_adfY = FMath::Max(-1.0, FMath::Min(1.0, enu_R_adf[4]));
	double north_dot_adfX = enu_R_adf[3];
	double heading = acos(north_dot_adfY);
	if (north_dot_adfX > 0)
	{
		heading = -heading + 2 * PI;
	}
	if (FMath::IsNaN(heading))
	{
		heading = 0.0;
	}

	adf_location->latitude  = lat;
	adf_location->longitude = lng;
	adf_location->heading   = heading;
	adf_location->altitude  = alt;
	return true;
}

// Convert (lat, lng) in radians, altitude in meters to ECEF position in meters.
// Adapted from: https://microem.ru/files/2012/08/GPS.G1-X-00006.pdf
void LocationToPositionInEcef(const long double lat, const long double lng, const long double alt, double* ecef_p_point)
{
	CHECK_NOTNULL(ecef_p_point);
	const double sin_lat = FMath::Sin(lat);
	const double N = a / FMath::Sqrt(1 - e2_a * sin_lat * sin_lat);
	ecef_p_point[0] = (N + alt)            * FMath::Cos(lat) * FMath::Cos(lng);
	ecef_p_point[1] = (N + alt)            * FMath::Cos(lat) * FMath::Sin(lng);
	ecef_p_point[2] = ((1 - e2_a)*N + alt) * sin_lat;
}

// Convert ECEF position in meters to (lat, lng) in radians, altitude in meters.
// Adapted from: https://microem.ru/files/2012/08/GPS.G1-X-00006.pdf
void PositionInEcefToLocation(const double* ecef_p_point_d, double* lat_d,double* lng_d, double* alt_d)
{
	CHECK_NOTNULL(ecef_p_point_d);
	CHECK_NOTNULL(lat_d);
	CHECK_NOTNULL(lng_d);
	CHECK_NOTNULL(alt_d);
	long double ecef_p_point[3] = { ecef_p_point_d[0], ecef_p_point_d[1], ecef_p_point_d[2] };
	long double p = FMath::Sqrt(ecef_p_point[0] * ecef_p_point[0] + ecef_p_point[1] * ecef_p_point[1]);
	long double theta = FMath::Atan2((ecef_p_point[2] * a), (p * b));
	long double st = FMath::Sin(theta);
	long double ct = FMath::Cos(theta);

	long double lat = FMath::Atan((ecef_p_point[2] + e2_b * b * st * st * st)/(p - e2_a * a * ct * ct * ct));
	long double lng = FMath::Atan2(ecef_p_point[1], ecef_p_point[0]);
	long double sin_lat = FMath::Sin(lat);
	long double N = a / FMath::Sqrt(1 - e2_a * sin_lat * sin_lat);
	long double alt = p / FMath::Cos(lat) - N;
	*lat_d = lat;
	*lng_d = lng;
	*alt_d = alt;
}

// Create matrix relating ECEF frame and ENU (east-north-up).
void GetRotation_ECEF_R_ENU(const double lat, const double lng, double* m)
{
	m[0] = -sin(lng);
	m[1] = -sin(lat)*cos(lng);
	m[2] = cos(lat)*cos(lng);
	m[3] = cos(lng);
	m[4] = -sin(lat)*sin(lng);
	m[5] = sin(lng)*cos(lat);
	m[6] = 0;
	m[7] = cos(lat);
	m[8] = sin(lat);
}

// Create matrix relating ENU (east-north-up) and the map frame (rotated by
// heading).
void GetRotation_ENU_R_ADF(const double heading, double* m)
{
	m[0] = cos(heading);
	m[1] = sin(heading);
	m[2] = 0;
	m[3] = -sin(heading);
	m[4] = cos(heading);
	m[5] = 0;
	m[6] = 0;
	m[7] = 0;
	m[8] = 1;
}

void GetRotation_ECEF_R_ADF(const double lat, const double lng, const double heading, double* m)
{
	m[0] = sin(heading)*sin(lat)*cos(lng) - sin(lng)*cos(heading);
	m[1] = -sin(heading)*sin(lng) - sin(lat)*cos(heading)*cos(lng);
	m[2] = cos(lat)*cos(lng);
	m[3] = cos(heading)*cos(lng) + sin(heading)*sin(lat)*sin(lng);
	m[4] = sin(heading)*cos(lng) - sin(lat)*sin(lng)*cos(heading);
	m[5] = sin(lng)*cos(lat);
	m[6] = -sin(heading)*cos(lat);
	m[7] = cos(heading)*cos(lat);
	m[8] = sin(lat);
}

void GetMatrix3x3Transpose(const double* in, double* out) {
  out[0] = in[0]; out[1] = in[3]; out[2] = in[6];
  out[3] = in[1]; out[4] = in[4]; out[5] = in[7];
  out[6] = in[2]; out[7] = in[5]; out[8] = in[8];
}

void MultiplyMatrix3x3(const double* m, const double* n, double* result)
{
	double
		m11 = m[0], m12 = m[1], m13 = m[2],
		m21 = m[3], m22 = m[4], m23 = m[5],
		m31 = m[6], m32 = m[7], m33 = m[8];
	double
		n11 = n[0], n12 = n[1], n13 = n[2],
		n21 = n[3], n22 = n[4], n23 = n[5],
		n31 = n[6], n32 = n[7], n33 = n[8];

	result[0] = m11*n11 + m12*n21 + m13*n31;
	result[1] = m11*n12 + m12*n22 + m13*n32;
	result[2] = m11*n13 + m12*n23 + m13*n33;

	result[3] = m21*n11 + m22*n21 + m23*n31;
	result[4] = m21*n12 + m22*n22 + m23*n32;
	result[5] = m21*n13 + m22*n23 + m23*n33;

	result[6] = m31*n11 + m32*n21 + m33*n31;
	result[7] = m31*n12 + m32*n22 + m33*n32;
	result[8] = m31*n13 + m32*n23 + m33*n33;
}

void ExpressVector3(const double* a_R_b, const double* b_vec, double* a_vec)
{
	double
		R11 = a_R_b[0], R12 = a_R_b[1], R13 = a_R_b[2],
		R21 = a_R_b[3], R22 = a_R_b[4], R23 = a_R_b[5],
		R31 = a_R_b[6], R32 = a_R_b[7], R33 = a_R_b[8];
	a_vec[0] = R11*b_vec[0] + R12*b_vec[1] + R13*b_vec[2];
	a_vec[1] = R21*b_vec[0] + R22*b_vec[1] + R23*b_vec[2];
	a_vec[2] = R31*b_vec[0] + R32*b_vec[1] + R33*b_vec[2];
}

void RotationMatrixToQuaternion(const double* a_R_b, double* q_xyzw)
{
	// See http://www.euclideanspace.com/maths/geometry/rotations/conversions/
	//     matrixToQuaternion/index.htm
	
	double m11 = a_R_b[0], m12 = a_R_b[1], m13 = a_R_b[2],
			m21 = a_R_b[3], m22 = a_R_b[4], m23 = a_R_b[5],
			m31 = a_R_b[6], m32 = a_R_b[7], m33 = a_R_b[8];

	double trace = m11 + m22 + m33;
	double x, y, z, w;
	if (trace > 0)
	{
		double s = 0.5 / FMath::Sqrt(trace + 1.0);
		w = 0.25 / s;
		x = (m32 - m23) * s;
		y = (m13 - m31) * s;
		z = (m21 - m12) * s;
	}
	else if (m11 > m22 && m11 > m33)
	{
		double s = 2.0 * FMath::Sqrt(1.0 + m11 - m22 - m33);
		w = (m32 - m23) / s;
		x = 0.25 * s;
		y = (m12 + m21) / s;
		z = (m13 + m31) / s;
	}
	else if (m22 > m33)
	{
		double s = 2.0 * FMath::Sqrt(1.0 + m22 - m11 - m33);
		w = (m13 - m31) / s;
		x = (m12 + m21) / s;
		y = 0.25 * s;
		z = (m23 + m32) / s;
	}
	else
	{
		double s = 2.0 * FMath::Sqrt(1.0 + m33 - m11 - m22);
		w = (m21 - m12) / s;
		x = (m13 + m31) / s;
		y = (m23 + m32) / s;
		z = 0.25 * s;
	}
	q_xyzw[0] = x;
	q_xyzw[1] = y;
	q_xyzw[2] = z;
	q_xyzw[3] = w;
}

void QuaternionToRotationMatrix(const double* q_xyzw, double* a_R_b)
{
	double x = q_xyzw[0];
	double y = q_xyzw[1];
	double z = q_xyzw[2];
	double w = q_xyzw[3];
	double xx = x*x;
	double yy = y*y;
	double zz = z*z;
	double ww = w*w;
	double sq_norm = xx + yy + zz + ww;
	constexpr double kSquaredNormThreshold = 1e-3;
	CHECK_LE(sq_norm, 1.0 + kSquaredNormThreshold);
	CHECK_GE(sq_norm, 1.0 - kSquaredNormThreshold);
	a_R_b[0] = 2 * xx + 2 * ww - 1;
	a_R_b[1] = 2 * (x*y - z*w);
	a_R_b[2] = 2 * (x*z + y*w);
	a_R_b[3] = 2 * (x*y + z*w);
	a_R_b[4] = 2 * yy + 2 * ww - 1;
	a_R_b[5] = 2 * (y*z - x*w);
	a_R_b[6] = 2 * (x*z - y*w);
	a_R_b[7] = 2 * (y*z + x*w);
	a_R_b[8] = 2 * ww + 2 * zz - 1;
}

}  // namespace ecef_tools

