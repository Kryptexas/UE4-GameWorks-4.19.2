#ifndef THIRD_PARTY_REDWOOD_ARCORE_AR_CORE_C_API_ARCORE_C_API_H_
#define THIRD_PARTY_REDWOOD_ARCORE_AR_CORE_C_API_ARCORE_C_API_H_

#include <stddef.h>
#include <stdint.h>

// Object ownership
// ================
// ARCore has two categories of objects: "value types" and "reference types".
//
// - Value types are owned by developer. They are created and destroyed using
//   the 'create'/'destroy' methods, and are populated by ARCore using methods
//   with 'get' in the method name.
//
// - Reference types are owned by ARCore. A reference is acquired by one of the
//   'acquire' methods, and for each 'acquire', the developer must call a
//   'release'. Note that even if last reference is released, ARCore may
//   continue to hold a reference to the object at ARCore's discretion.
//
// Reference types are further split into:
//
// - Long-lived objects. These objects are created by ARCore and are available
//   across frames. Acquire may fail if ARCore is in an incorrect state, e.g.
//   not tracking. Acquire from list always succeeds, as the object already
//   exists.
//
// - Large data. These objects are usually acquired per-frame and are a limited
//   resource. This may fail if the resource is exhausted, deadline exceeded, or
//   resource is not yet available.
//
// Note: Lists are value types (owned by developer), but can hold references.
// This means that the references held by a list are not released until either
// the list is destroyed, or is re-populated by another api call.
// For example, ArAnchorList, which is a value type, will hold references to
// Anchors, which are Long-lived objects.

// Opaque ARCore struct declarations.

// Session and widely-used objects such as pose.
typedef struct ArConfig_ ArConfig;    // Value type.
typedef struct ArSession_ ArSession;  // Value type.
typedef struct ArPose_ ArPose;        // Value type.

// Frame and frame objects.
typedef struct ArCamera_ ArCamera;                // Long-lived.
typedef struct ArFrame_ ArFrame;                  // Value type.
typedef struct ArLightEstimate_ ArLightEstimate;  // Value type.
typedef struct ArPointCloud_ ArPointCloud;        // Large data.
typedef struct ArImageMetadata_ ArImageMetadata;  // Large data.

// Trackables.
typedef struct ArTrackable_ ArTrackable;          // Long-lived.
typedef struct ArTrackableList_ ArTrackableList;  // Value type.
typedef struct ArPlane_ ArPlane;                  // Long-lived, Trackable.
typedef struct ArPoint_ ArPoint;                  // Long-lived, Trackable.

// Anchors.
typedef struct ArAnchor_ ArAnchor;          // Long-lived.
typedef struct ArAnchorList_ ArAnchorList;  // Value type.

// Hit result functionality.
typedef struct ArHitResult_ ArHitResult;          // Value type.
typedef struct ArHitResultList_ ArHitResultList;  // Value type.

// Forward declearing the ACameraMetadata struct from Android ndk, which is used
// in ArImageMetadata_getNdkCameraMetadata
typedef struct ACameraMetadata ACameraMetadata;

/// @defgroup ARCore_CPP_API The ARCore C++ helper functions
/// @{

// Expose allowable type conversions as c++ helper functions. This avoids having
// the reinterpret_casts done by the developer. Convenience methods, but also
// documentation of what is castable.
// Note: these methods do not increment reference count on the casted objects.
// Note: runtime type checks are not done on downcasts. Call the appropriate
// getType() method beforehand to figure out the correct cast. Note: pure C
// users are to do the c-style casts themselves.

#ifdef __cplusplus
inline ArTrackable* ArAsTrackable(ArPlane* plane) {
  return reinterpret_cast<ArTrackable*>(plane);
}
inline ArTrackable* ArAsTrackable(ArPoint* point) {
  return reinterpret_cast<ArTrackable*>(point);
}
inline ArPlane* ArAsPlane(ArTrackable* trackable) {
  return reinterpret_cast<ArPlane*>(trackable);
}
inline ArPoint* ArAsPoint(ArTrackable* trackable) {
  return reinterpret_cast<ArPoint*>(trackable);
}
#endif
/// @}

/// @defgroup ARCore_ENUM The ARCore enum types
/// @{

// If compiling for c++11, use the 'enum underlying type' feature to enforce
// size for ABI compatibility. In pre-c++11, use int32_t for fixed size.
#if __cplusplus >= 201100
#define AR_DEFINE_ENUM(_type) enum _type : int32_t
#else
#define AR_DEFINE_ENUM(_type) \
  typedef int32_t _type;      \
  enum
#endif

// Note: for Type enums the values are chosen arbitrarily, but specifically
// non-zero and with some non-trivial value. This allows some sanity checking
// and debugging if ARCore is given a raw pointer, but one should not rely on
// this of course to do validation of raw pointers.
AR_DEFINE_ENUM(ArTrackableType){
    // Class types for heterogeneous query/update lists.

    // Represents the base Trackable type. Can be passed to
    // ArSession_getAllTrackables/ArFrame_getUpdatedTrackables as the
    // filter_type to get all/updated Trackables of all subtypes.
    AR_TRACKABLE_BASE_TRACKABLE = 0x41520100,

    // Represents the Plane subtype of Trackable.
    AR_TRACKABLE_PLANE = 0x41520101,

    // Represents the Point subtype of Trackable.
    AR_TRACKABLE_POINT = 0x41520102,

    // Represents an invalid Trackable type.
    AR_TRACKABLE_NOT_VALID = 0,
};

AR_DEFINE_ENUM(ArStatus){
    AR_SUCCESS = 0,
    // Invalid argument handling: null pointers and invalid enums for void
    // functions are handled by logging and returning best-effort value.
    // Non-void functions additionally return AR_ERROR_INVALID_ARGUMENT.
    AR_ERROR_INVALID_ARGUMENT = -1,
    AR_ERROR_FATAL = -2,
    AR_ERROR_SESSION_PAUSED = -3,
    AR_ERROR_SESSION_NOT_PAUSED = -4,
    AR_ERROR_NOT_TRACKING = -5,
    AR_ERROR_TEXTURE_NOT_SET = -6,
    AR_ERROR_MISSING_GL_CONTEXT = -7,
    AR_ERROR_UNSUPPORTED_CONFIGURATION = -8,
    AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED = -9,

    // Acquire failed because the object being acquired is already released.
    // This happens e.g. if the developer holds an old frame for too long, and
    // then tries to acquire a point cloud from it.
    AR_ERROR_DEADLINE_EXCEEDED = -10,

    // Acquire failed because there are too many objects already acquired. For
    // example, the developer may acquire up to N point clouds.
    // N is determined by available resources, and is usually small, e.g. 8.
    // If the developer tries to acquire N+1 point clouds without releasing the
    // previously acquired ones, they will get this error.
    AR_ERROR_RESOURCE_EXHAUSTED = -11,

    // Acquire failed because the data isn't available yet for the current
    // frame. For example, acquire the image metadata may fail with this error
    // because the camera hasn't fully started.
    AR_ERROR_NOT_YET_AVAILABLE = -12,

    AR_UNAVAILABLE_ARCORE_NOT_INSTALLED = -100,
    AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE = -101,
    AR_UNAVAILABLE_ANDROID_VERSION_NOT_SUPPORTED = -102,
    // The ARCore APK currently installed on device is too old and needs to be
    // updated. For example, SDK v2.0.0 when APK is v1.0.0.
    AR_UNAVAILABLE_APK_TOO_OLD = -103,
    // The ARCore APK currently installed no longer supports the sdk that the
    // app was built with. For example, SDK v1.0.0 when APK includes support for
    // v2.0.0+.
    AR_UNAVAILABLE_SDK_TOO_OLD = -104,
};

AR_DEFINE_ENUM(ArTrackingState){
    AR_TRACKING_STATE_TRACKING = 0,
    AR_TRACKING_STATE_PAUSED = 1,
    AR_TRACKING_STATE_STOPPED = 2,
};

AR_DEFINE_ENUM(ArLightEstimationMode){
    AR_LIGHT_ESTIMATION_MODE_DISABLED = 0,
    AR_LIGHT_ESTIMATION_MODE_AMBIENT_INTENSITY = 1,
};

AR_DEFINE_ENUM(ArPlaneFindingMode){
    AR_PLANE_FINDING_MODE_DISABLED = 0,
    AR_PLANE_FINDING_MODE_HORIZONTAL = 1,
};

AR_DEFINE_ENUM(ArUpdateMode){
    AR_UPDATE_MODE_BLOCKING = 0,
    AR_UPDATE_MODE_LATEST_CAMERA_IMAGE = 1,
};

AR_DEFINE_ENUM(ArPlaneType){
    AR_PLANE_HORIZONTAL_UPWARD_FACING = 0,
    AR_PLANE_HORIZONTAL_DOWNWARD_FACING = 1,
};

AR_DEFINE_ENUM(ArLightEstimateState){
    AR_LIGHT_ESTIMATE_STATE_NOT_VALID = 0,
    AR_LIGHT_ESTIMATE_STATE_VALID = 1,
};

#undef AR_DEFINE_ENUM

/// @}

/// @defgroup ARCore_C_API The ARCore C API
/// @{

#ifdef __cplusplus
extern "C" {
#endif

// TODO(b/67461776): port documentation from Java.

// Note: destroy methods do not take ArSession* to allow late destruction in
// finalizers of garbage-collected languages such as Java.

// === Session create ===
// This function is only implemented in session_create_client.cc which is
// packaged in SDK for developer to call for remote .so loading, remote library
// will call ArSession_createService to create the actual session, so this
// function should not be Dynamite forwarded.

// Errors:
// AR_UNAVAILABLE_ARCORE_NOT_INSTALLED
// AR_UNAVAILABLE_APK_TOO_OLD
// AR_UNAVAILABLE_SDK_TOO_OLD
// TODO(b/68716750): Add API to query whether arcore can be installed on this
// device.
ArStatus ArSession_create(void* env, void* application_context,
                          ArSession** out_session_pointer);

// === ArConfig methods ===
void ArConfig_create(const ArSession* session, ArConfig** out_config);
void ArConfig_destroy(ArConfig* config);
void ArConfig_getLightEstimationMode(
    const ArSession* session, const ArConfig* config,
    ArLightEstimationMode* light_estimation_mode);
void ArConfig_setLightEstimationMode(
    const ArSession* session, ArConfig* config,
    ArLightEstimationMode light_estimation_mode);
void ArConfig_getPlaneFindingMode(const ArSession* session,
                                  const ArConfig* config,
                                  ArPlaneFindingMode* plane_finding_mode);
void ArConfig_setPlaneFindingMode(const ArSession* session, ArConfig* config,
                                  ArPlaneFindingMode plane_finding_mode);
void ArConfig_getUpdateMode(const ArSession* session, const ArConfig* config,
                            ArUpdateMode* update_mode);
void ArConfig_setUpdateMode(const ArSession* session, ArConfig* config,
                            ArUpdateMode update_mode);

// === ArSession methods ===
void ArSession_destroy(ArSession* session);
// AR_ERROR_UNSUPPORTED_CONFIGURATION
ArStatus ArSession_checkSupported(const ArSession* session,
                                  const ArConfig* config);
// AR_ERROR_FATAL
// AR_ERROR_UNSUPPORTED_CONFIGURATION
// AR_ERROR_SESSION_NOT_PAUSED
ArStatus ArSession_configure(ArSession* session, const ArConfig* config);

// AR_ERROR_FATAL
// AR_ERROR_CAMERA_PERMISSION_NOT_GRANTED
ArStatus ArSession_resume(ArSession* session);

// AR_ERROR_FATAL
ArStatus ArSession_pause(ArSession* session);

void ArSession_setCameraTextureName(ArSession* session, uint32_t texture_id);

// Note: this function doesn't fail. If given invalid values, it logs
// and doesn't apply the changes.
void ArSession_setDisplayGeometry(ArSession* session, int rotation, int width,
                                  int height);
// AR_ERROR_FATAL
// AR_ERROR_SESSION_PAUSED
// AR_ERROR_TEXTURE_NOT_SET
// AR_ERROR_MISSING_GL_CONTEXT
ArStatus ArSession_update(ArSession* session, ArFrame* out_frame);
// AR_ERROR_NOT_TRACKING
// AR_ERROR_SESSION_PAUSED
// AR_ERROR_RESOURCE_EXHAUSTED
ArStatus ArSession_acquireNewAnchor(ArSession* session, const ArPose* pose,
                                    ArAnchor** out_anchor);
void ArSession_getAllAnchors(const ArSession* session,
                             ArAnchorList* out_anchor_list);
void ArSession_getAllTrackables(const ArSession* session,
                                ArTrackableType filter_type,
                                ArTrackableList* out_trackable_list);

// === ArPose methods ===
// pose_raw is the rotation (quaternion) and translation in the same order as
// the first 7 elements of the Android Sensor.TYPE_POSE_6DOF (qx, qy, qz, qw,
// tx, ty, tz).
// https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_POSE_6DOF

// Creates a new ArPose.
// If pose_raw is null, initializes with the identity pose.

void ArPose_create(const ArSession* session, const float* pose_raw,
                   ArPose** out_pose);
void ArPose_destroy(ArPose* pose);
void ArPose_getPoseRaw(const ArSession* session, const ArPose* pose,
                       float* out_pose_raw);
void ArPose_getMatrix(const ArSession* session, const ArPose* pose,
                      float* out_matrix_col_major_4x4);

// === ArCamera methods ===
void ArCamera_getPose(const ArSession* session, const ArCamera* camera,
                      ArPose* out_pose);
void ArCamera_getDisplayOrientedPose(const ArSession* session,
                                     const ArCamera* camera, ArPose* out_pose);
void ArCamera_getViewMatrix(const ArSession* session, const ArCamera* camera,
                            float* out_col_major_4x4);
void ArCamera_getTrackingState(const ArSession* session, const ArCamera* camera,
                               ArTrackingState* out_tracking_state);
void ArCamera_getProjectionMatrix(const ArSession* session,
                                  const ArCamera* camera, float near, float far,
                                  float* dest_col_major_4x4);
void ArCamera_release(ArCamera* camera);

// === ArFrame methods ===
void ArFrame_create(const ArSession* session, ArFrame** out_frame);
void ArFrame_destroy(ArFrame* frame);
void ArFrame_getDisplayGeometryChanged(const ArSession* session,
                                       const ArFrame* frame,
                                       int32_t* out_geometry_changed);
void ArFrame_getTimestamp(const ArSession* session, const ArFrame* frame,
                          int64_t* out_timestamp_ns);

// Note: this function doesn't fail. If given odd number of elements, it logs
// and applies rotation only to the even number of elements. Last element is set
// to zero. For negative number of elements, log and no effect.
void ArFrame_transformDisplayUvCoords(const ArSession* session,
                                      const ArFrame* frame,
                                      int32_t num_elements, const float* uvs_in,
                                      float* uvs_out);
// Note: If not tracking, the hit_result_list will be empty.
// Note: If called on an old frame (not the latest given by ArSession_update),
// the hit_result_list will be empty.
void ArFrame_hitTest(const ArSession* session, const ArFrame* frame,
                     float pixel_x, float pixel_y,
                     ArHitResultList* hit_result_list);
void ArFrame_getLightEstimate(const ArSession* session, const ArFrame* frame,
                              ArLightEstimate* out_light_estimate);

// AR_ERROR_DEADLINE_EXCEEDED
// AR_ERROR_RESOURCE_EXHAUSTED
ArStatus ArFrame_acquirePointCloud(const ArSession* session,
                                   const ArFrame* frame,
                                   ArPointCloud** out_point_cloud);
// This behaves like acquire-from-list and so does not fail.
void ArFrame_acquireCamera(const ArSession* session, const ArFrame* frame,
                           ArCamera** out_camera);
// AR_ERROR_DEADLINE_EXCEEDED
// AR_ERROR_NOT_YET_AVAILABLE
ArStatus ArFrame_acquireImageMetadata(const ArSession* session,
                                      const ArFrame* frame,
                                      ArImageMetadata** out_metadata);
void ArFrame_getUpdatedAnchors(const ArSession* session, const ArFrame* frame,
                               ArAnchorList* out_anchor_list);
void ArFrame_getUpdatedTrackables(const ArSession* session,
                                  const ArFrame* frame,
                                  ArTrackableType filter_type,
                                  ArTrackableList* out_trackable_list);

// === ArPointCloud methods ===
void ArPointCloud_getNumberOfPoints(const ArSession* session,
                                    const ArPointCloud* point_cloud,
                                    int32_t* out_number_of_points);
// The pointer returned by this function is valid only until release. Developer
// must copy data if they wish to maintain it longer. The data is in world
// coordinates corresponding to the frame it was obtained from.
// If the number of points is zero, the value of out_point_cloud_data is
// undefined.
void ArPointCloud_getData(const ArSession* session,
                          const ArPointCloud* point_cloud,
                          const float** out_point_cloud_data);
void ArPointCloud_getTimestamp(const ArSession* session,
                               const ArPointCloud* point_cloud,
                               int64_t* out_timestamp_ns);
void ArPointCloud_release(ArPointCloud* point_cloud);

// === Image Metadata methods ===
// The ACameraMetadata is a struct in Android ndk. Include NdkCameraMetadata.h
// to use this type.
// Note that the ACameraMetadata returned from this function will be invalid if
// its ArImageMetadata struct is released.
void ArImageMetadata_getNdkCameraMetadata(
    const ArSession* session, const ArImageMetadata* image_metadata,
    const ACameraMetadata** out_ndk_metadata);

void ArImageMetadata_release(ArImageMetadata* metadata);

// === ArLightEstimate methods ===
void ArLightEstimate_create(const ArSession* session,
                            ArLightEstimate** out_light_estimate);
void ArLightEstimate_destroy(ArLightEstimate* light_estimate);
void ArLightEstimate_getState(const ArSession* session,
                              const ArLightEstimate* light_estimate,
                              ArLightEstimateState* out_light_estimate_state);
void ArLightEstimate_getPixelIntensity(const ArSession* session,
                                       const ArLightEstimate* light_estimate,
                                       float* out_pixel_intensity);

// === ArAnchorList methods ===
void ArAnchorList_create(const ArSession* session,
                         ArAnchorList** out_anchor_list);
void ArAnchorList_destroy(ArAnchorList* anchor_list);
void ArAnchorList_getSize(const ArSession* session,
                          const ArAnchorList* anchor_list, int32_t* out_size);
void ArAnchorList_acquireItem(const ArSession* session,
                              const ArAnchorList* anchor_list, int32_t index,
                              ArAnchor** out_anchor);

// === ArAnchor methods ===
void ArAnchor_getPose(const ArSession* session, const ArAnchor* anchor,
                      ArPose* out_pose);
void ArAnchor_getTrackingState(const ArSession* session, const ArAnchor* anchor,
                               ArTrackingState* out_tracking_state);

// Tells ARCore to stop tracking this anchor.
void ArAnchor_detach(ArSession* session, ArAnchor* anchor);

// Releases a reference to an anchor. This does not mean that the anchor will
// stop tracking, as it will be obtainable from e.g. ArSession_getAllAnchors().
void ArAnchor_release(ArAnchor* anchor);

// === ArTrackableList methods ===
void ArTrackableList_create(const ArSession* session,
                            ArTrackableList** out_trackable_list);
void ArTrackableList_destroy(ArTrackableList* trackable_list);
void ArTrackableList_getSize(const ArSession* session,
                             const ArTrackableList* trackable_list,
                             int32_t* out_size);
void ArTrackableList_acquireItem(const ArSession* session,
                                 const ArTrackableList* trackable_list,
                                 int32_t index, ArTrackable** out_trackable);

// === ArTrackable methods ===
// Releases a reference to a trackable. This does not mean that the trackable
// will necessarily stop tracking. The same trackable can still be obtained from
// e.g. ArSession_getAllTrackables().
void ArTrackable_release(ArTrackable* trackable);

void ArTrackable_getType(const ArSession* session, const ArTrackable* trackable,
                         ArTrackableType* out_trackable_type);

void ArTrackable_getTrackingState(const ArSession* session,
                                  const ArTrackable* trackable,
                                  ArTrackingState* out_tracking_state);

// AR_ERROR_NOT_TRACKING
// AR_ERROR_SESSION_PAUSED
// AR_ERROR_RESOURCE_EXHAUSTED
ArStatus ArTrackable_acquireNewAnchor(ArSession* session,
                                      ArTrackable* trackable, ArPose* pose,
                                      ArAnchor** out_anchor);

void ArTrackable_getAnchors(const ArSession* session,
                            const ArTrackable* trackable,
                            ArAnchorList* out_anchor_list);

// === ArPlane methods ===
// Note that this function can normally return null in out_subsumed_by, if the
// plane is not subsumed. ArTrackable_release(nullptr) will behave properly
// (ie not crash or error).
void ArPlane_acquireSubsumedBy(const ArSession* session, const ArPlane* plane,
                               ArPlane** out_subsumed_by);

void ArPlane_getType(const ArSession* session, const ArPlane* plane,
                     ArPlaneType* out_plane_type);
void ArPlane_getCenterPose(const ArSession* session, const ArPlane* plane,
                           ArPose* out_pose);
void ArPlane_getExtentX(const ArSession* session, const ArPlane* plane,
                        float* out_extent_x);
void ArPlane_getExtentZ(const ArSession* session, const ArPlane* plane,
                        float* out_extent_z);
void ArPlane_getPolygonSize(const ArSession* session, const ArPlane* plane,
                            int32_t* out_polygon_size);
void ArPlane_getPolygon(const ArSession* session, const ArPlane* plane,
                        float* out_polygon_xz);
void ArPlane_isPoseInExtents(const ArSession* session, const ArPlane* plane,
                             const ArPose* pose, int32_t* out_pose_in_extents);
void ArPlane_isPoseInPolygon(const ArSession* session, const ArPlane* plane,
                             const ArPose* pose, int32_t* out_pose_in_polygon);

// === ArPoint methods ===
void ArPoint_getPose(const ArSession* session, const ArPoint* point,
                     ArPose* out_pose);

// === ArHitResultList methods ===
void ArHitResultList_create(const ArSession* session,
                            ArHitResultList** out_hit_result_list);
void ArHitResultList_destroy(ArHitResultList* hit_result_list);
void ArHitResultList_getSize(const ArSession* session,
                             const ArHitResultList* hit_result_list,
                             int32_t* out_size);
void ArHitResultList_getItem(const ArSession* session,
                             const ArHitResultList* hit_result_list,
                             int32_t index, ArHitResult* out_hit_result);

// === ArHitResult methods ===
void ArHitResult_create(const ArSession* session, ArHitResult** out_hit_result);
void ArHitResult_destroy(ArHitResult* hit_result);
void ArHitResult_getDistance(const ArSession* session,
                             const ArHitResult* hit_result,
                             float* out_distance);
void ArHitResult_getHitPose(const ArSession* session,
                            const ArHitResult* hit_result, ArPose* out_pose);

// This behaves like acquire-from-list and so does not fail.
void ArHitResult_acquireTrackable(const ArSession* session,
                                  const ArHitResult* hit_result,
                                  ArTrackable** out_trackable);

// AR_ERROR_NOT_TRACKING
// AR_ERROR_SESSION_PAUSED
// AR_ERROR_RESOURCE_EXHAUSTED
// AR_ERROR_DEADLINE_EXCEEDED - hit result must be used before the next call to
// update().
ArStatus ArHitResult_acquireNewAnchor(ArSession* session,
                                      ArHitResult* hit_result,
                                      ArAnchor** out_anchor);

#ifdef __cplusplus
}
#endif

/// @}

#endif  // THIRD_PARTY_REDWOOD_ARCORE_AR_CORE_C_API_ARCORE_C_API_H_
