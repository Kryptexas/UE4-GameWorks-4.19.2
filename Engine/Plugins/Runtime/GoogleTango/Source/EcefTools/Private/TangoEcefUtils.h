/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

namespace ecef_tools {

/**
 * @brief Struct to hold global location information. Units of measure
 * (degrees vs. radians) are unspecified.
 *
 * This is designed to be semantically similar to the Android Location object:
 * http://developer.android.com/reference/android/location/Location.html
 */
struct Location {
  Location() {}
  Location(const double lat, const double lng, const double alt,
           const double in_heading) :
    latitude(lat), longitude(lng), altitude(alt), heading(in_heading) {}

  double latitude;   // Geodetic latitude using WGS84 model.
  double longitude;  // Geodetic longitude using WGS84 model.
  double altitude;   // Altitude above WGS84 reference spheroid.
  double heading;    // Clockwise angle from north.
};

/**
 * @brief Compute the position of a point in the ADF frame.
 * @param point_location Location of a point, in radians (heading is ignored).
 * @param adf_location Location of the ADF frame, in radians.
 * @param adf_p_point The XYZ position of the point in the ADF frame.
 */
void LocationToPositionInAdf(const Location& point_location,
                             const Location& adf_location,
                             double* adf_p_point);

/**
 * @brief Compute the position of a point in the ADF frame.
 * @param point_location Location of a point, in radians (heading is ignored).
 * @param ecef_p_adf Position of the ADF origin in ECEF coordinates.
 * @param ecef_q_adf ADF orientation in the ECEF frame.
 * @param adf_p_point The XYZ position of the point in the ADF frame.
 */
void LocationToPositionInAdf(const Location& point_location,
                             const double* ecef_p_adf,
                             const double* ecef_q_adf,
                             double* adf_p_point);

/**
 * @brief Convert a point in the ADF frame to a global location.
 * @param adf_p_point The XYZ position of the point in the ADF frame.
 * @param adf_location Location of the ADF frame, in radians.
 * @param point_location Location of the point, in radians (heading for a point
 * is undefined, but it is set to 0).
 */
void PositionInAdfToLocation(const double* adf_p_point,
                             const Location& adf_location,
                             Location* point_location);

/**
 * @brief Convert a point in the ADF frame to a global location.
 * @param adf_p_point The XYZ position of the point in the ADF frame.
 * @param ecef_p_adf Position of the ADF origin in ECEF coordinates.
 * @param ecef_q_adf ADF orientation in the ECEF frame.
 * @param point_location Location of the point, in radians (heading for a point
 * is undefined, but it is set to 0).
 */
void PositionInAdfToLocation(const double* adf_p_point,
                             const double* ecef_p_adf,
                             const double* ecef_q_adf,
                             Location* point_location);

/**
 * @brief Convert device orientation in adf frame (or start-of-service) to a
 * heading angle on range [0, 2*PI). A negative result indicates failure due to
 * device pointing up or down.
 *
 * This is computed using the "forward" vector which points out the back of the
 * device, which is then projected onto the ground (X-Y) plane of the
 * local-level frame; the heading is the clockwise angle between the
 * ADF/start-of-service Y-axis and this projection.
 * @param adf_q_device Device orientation in ADF/start-of-service frame.
 * @return Device heading in radians [0, 2*PI), or negative result on failure.
 */
double DeviceOrientationInAdfToHeadingInAdf(const double* adf_q_device);

/**
 * @brief Convert device pose to lat/lng, altitude, heading (angles in radians).
 * @param[in]  ecef_p_device   Vector from ECEF origin to device origin,
 *                             expressed in ECEF frame.
 * @param[in]  ecef_q_device   Quaternion representing the matrix ecef_R_device.
 * @param[out] device_location Output location with angles in radians.
 */
void DevicePoseInEcefToLocation(const double* ecef_p_device,
                                const double* ecef_q_device,
                                Location* device_location);

/**
 * @brief Convert ADF pose to lat/lng, altitude, heading (angles in radians).
 * @param[in]  ecef_p_adf   Vector from ECEF origin to adf origin, expressed
 *                          in ECEF frame.
 * @param[in]  ecef_q_adf   Quaternion representing the matrix ecef_R_adf.
 * @param[out] adf_location Output location with angles in radians.
 * @return True on success; false if the quaternion is invalid (not normalized).
 */
bool AdfPoseInEcefToLocation(const double* ecef_p_adf,
                             const double* ecef_q_adf,
                             Location* adf_location);

/**
 * @brief Convert lat/lng, altitude, heading (angles in radians) to adf pose.
 * @param[in]  adf_location Contains location with angles in radians.
 * @param[out] ecef_p_adf Output vector from ECEF origin to adf origin,
 *                        expressed in ECEF frame.
 * @param[out] ecef_q_adf Output quaternion representing the matrix ecef_R_adf.
 */
void LocationToAdfPoseInEcef(const Location& adf_location,
                             double* ecef_p_adf,
                             double* ecef_q_adf);

/**
 * @brief Convert lat/lng (radians) and altitude (meters) to vector from ECEF
 *        origin to that point.
 * @param[in]  lat          Geodedic latitude, in radians.
 * @param[in]  lng          Longitude, in radians.
 * @param[in]  alt          Height above WGS84 spheroid, in meters.
 * @param[out] ecef_p_point Position (meters) from ECEF origin, expressed in
 *                          ECEF frame.
 */
void LocationToPositionInEcef(const long double lat, const long double lng,
                              const long double alt, double* ecef_p_point);

/**
 * @brief Convert position from ECEF origin (in meters) to lat/lng (radians)
 *        and altitude (meters).
 * @param[in]  ecef_p_point Position (meters) from ECEF origin, expressed in
 *                          ECEF frame.
 * @param[out] lat          Geodedic latitude, in radians.
 * @param[out] lng          Longitude, in radians.
 * @param[out] alt          Height above WGS84 spheroid, in meters.
 */
void PositionInEcefToLocation(const double* ecef_p_point, double* lat,
                              double* lng, double* alt);

/**
 * @brief Make the rotation matrix (row-major double[9]) relating ECEF and the
 *        local "East-North-Up" frame, given lat/lng in radians.
 */
void GetRotation_ECEF_R_ENU(const double lat, const double lng, double* m);

/**
 * @brief Make the rotation matrix (row-major double[9]) relating the local
 *        "East-North-Up" frame and the local-level ADF frame, given a heading
 *        in radians. The heading is the clockwise angle between "north" and
 *        the ADF +Y axis.
 */
void GetRotation_ENU_R_ADF(const double heading, double* m);

/**
 * @brief Make the rotation matrix (row-major double[9]) relating ECEF and the
 *        local-level ADF frame, given lat/lng/heading in radians.
 */
void GetRotation_ECEF_R_ADF(const double lat, const double lng,
                            const double heading, double* m);

/**
 * @brief Convert rotation matrix (row-major double[9]) to quaternion [x y z w].
 */
void RotationMatrixToQuaternion(const double* a_R_b, double* q_xyzw);

/**
 * @brief Convert quaternion [x y z w] to rotation matrix (row-major double[9]).
 */
void QuaternionToRotationMatrix(const double* q_xyzw, double* a_R_b);

/**
 * @brief Transpose a 3x3 matrix (row-major double[9]).
 */
void GetMatrix3x3Transpose(const double* in, double* out);

/**
 * @brief Matrix multiplication a * b = c. Each matrix is row-major double[9].
 */
void MultiplyMatrix3x3(const double* a, const double* b, double* c);

/**
 * @brief Re-express a vector: a_vec = a_R_b * b_vec.
 * a_R_b is row-major double[9], b_vec and a_vec are double[3].
 */
void ExpressVector3(const double* a_R_b, const double* b_vec, double* a_vec);

}  // namespace ecef_tools
