/* Copyright 2016 Google Inc. All rights reserved.
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

#include "gvr_arm_model_math.h"

namespace gvr_arm_model {
  class Controller {
  public:

    struct UpdateData {
      bool connected;
      Vector3 acceleration;
      Quaternion orientation;
      Vector3 gyro;
      Vector3 headDirection;
      Vector3 headPosition;
      float deltaTimeSeconds;
    };

    enum GazeBehavior {
      Never,
      DuringMotion,
      Always
    };

    enum Handedness {
      Right,
      Left,
      Unknown
    };

    Controller();

    const Vector3& GetControllerPosition() const;
    const Quaternion& GetControllerRotation() const;

    const Vector3& GetPointerPositionOffset() const;

    float GetAddedElbowHeight() const;
    void SetAddedElbowHeight(float elbow_height);

    float GetAddedElbowDepth() const;
    void SetAddedElbowDepth(float elbow_depth);

    float GetPointerTiltAngle();
    void SetPointerTiltAngle(float tilt_angle);

    GazeBehavior GetGazeBehavior() const;
    void SetGazeBehavior(GazeBehavior gaze_behavior);

    Handedness GetHandedness() const;
    void SetHandedness(Handedness new_handedness);

    bool GetUseAccelerometer() const;
    void SetUseAccelerometer(bool new_use_accelerometer);

    float GetFadeDistanceFromFace() const;
    void SetFadeDistanceFromFace(float distance_from_face);

    float GetTooltipMinDistanceFromFace() const;
    void SetTooltipMinDistanceFromFace(float distance_from_face);

    float GetControllerAlphaValue() const;

    float GetTooltipAlphaValue() const;

    void Update(const UpdateData& update_data);

  private:

    void UpdateHandedness();
    void UpdateTorsoDirection(const UpdateData& update_data);
    void UpdateFromController(const UpdateData& update_data);
    void UpdateVelocity(const UpdateData& update_data);
    void TransformElbow(const UpdateData& update_data);
    void ApplyArmModel(const UpdateData& update_data);
    void UpdateTransparency(const UpdateData& update_data);

    void ResetState();

    float added_elbow_height;
    float added_elbow_depth;
    float pointer_tilt_angle;
    GazeBehavior follow_gaze;
    Handedness handedness;
    bool use_accelerometer;
    float fade_distance_from_face;
    float tooltip_min_distance_from_face;

    Vector3 wrist_position;
    Quaternion wrist_rotation;

    Vector3 elbow_position;
    Quaternion elbow_rotation;

    Vector3 shoulder_position;
    Quaternion shoulder_rotation;

    Vector3 elbow_offset;
    Vector3 torso_direction;
    Vector3 filtered_velocity;
    Vector3 filtered_accel;
    Vector3 zero_accel;
    Vector3 handed_multiplier;
    float controller_alpha_value;
    float tooltip_alpha_value;

    bool first_update;

    // Strength of the acceleration filter (unitless).
    static constexpr float GRAVITY_CALIB_STRENGTH = 0.999f;

    // Strength of the velocity suppression (unitless).
    static constexpr float VELOCITY_FILTER_SUPPRESS = 0.99f;

    // The minimum allowable accelerometer reading before zeroing (m/s^2).
    static constexpr float MIN_ACCEL = 1.0f;

    // The expected force of gravity (m/s^2).
    static constexpr float GRAVITY_FORCE = 9.807f;

    // Amount of normalized alpha transparency to change per second.
    static constexpr float DELTA_ALPHA = 4.0f;

    // Unrotated Position offset from wrist to pointer.
	static const Vector3 POINTER_OFFSET;

    // Initial relative location of the shoulder (meters).
    static const Vector3 DEFAULT_SHOULDER_RIGHT;//{ 0.19f, -0.19f, 0.03f };

    // The range of movement from the elbow position due to accelerometer (meters).
	static const Vector3 ELBOW_MIN_RANGE;// { -0.05f, -0.1f, -0.2f };
    static const Vector3 ELBOW_MAX_RANGE;//{ 0.05f, 0.1f, 0.0f };
    
    // Forward Vector in GVR Space.
    static const Vector3 FORWARD;//{ 0.0f, 0.0f, -1.0f };

    // Up Vector in GVR Space.
    static const Vector3 UP;//{ 0.0f, 1.0f, 0.0f };
  };
}