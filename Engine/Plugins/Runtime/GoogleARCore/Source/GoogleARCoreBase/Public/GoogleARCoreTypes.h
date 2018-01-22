// Copyright 2017 Google Inc.
#pragma once

#include "CoreMinimal.h"

#include "ARTypes.h"
#include "ARTrackable.h"
#include "ARTraceResult.h"
#include "ARSystem.h"
#include "ARPin.h"

#include "GoogleARCoreTypes.generated.h"

/// @defgroup GoogleARCoreBase Google ARCore Base
/// The base module for Google ARCore plugin

#if PLATFORM_ANDROID
// Include <Camera/NdkCameraMetadata.h> to use this type.
typedef struct ACameraMetadata ACameraMetadata;

// Forward decalare type defined in arcore_c_api.h
typedef struct ArTrackable_ ArTrackable;
typedef struct ArPlane_ ArPlane;
typedef struct ArPoint_ ArPoint;
typedef struct ArPointCloud_ ArPointCloud;
typedef struct ArAnchor_ ArAnchor;
#endif

/**
 * @ingroup GoogleARCoreBase
 * Describes the ARCore support status.
 */
UENUM(BlueprintType)
enum class EGoogleARCoreSupportStatus : uint8
{
	/** ARCore is supported. */
	Supported,
	/** The device is not compatible with ARCore. */
	DeviceNotCompatiable,
	/** ARCore APK is not installed. Try to install the ARCore APK to fix the problem.*/
	ARCoreAPKNotInstalled,
	/** The Android version on the device doesn't support ARCore. Try to update the Android version on the phone to fix the problem. */
	AndroidVersionNotSupported,
	/** ARCore APK version is too old. Try to update ARCore APK to fix the problem. */
	ARCoreAPKTooOld,
	/** ARCore SDK version is too old. Try to use an Unreal Engine with newer version of ARCore plugin to fix the problem.*/
	ARCoreSDKTooOld,
	/** Unknown reason. */
	Unknown
};

/**
 * @ingroup GoogleARCoreBase
 * Describes the status of most ARCore functions.
 */
UENUM(BlueprintType)
enum class EGoogleARCoreFunctionStatus : uint8
{
	/** Function returned successfully. */
	Success,
	/** Function failed due to Fatal error. */
	Fatal,
	/** Function failed due to the session isn't running. */
	SessionPaused,
	/** Function failed due to ARCore session isn't in tracking state. */
	NotTracking,
	/** Function failed due to the requested resource is exhausted. */
	ResourceExhausted,
	/** Function failed due to the requested resource isn't available yet. */
	NotAvailable,
	/** Function failed due to the function augment has invalid type. */
	InvalidType,
	/** Function failed with unknown reason. */
	Unknown
};

/**
 * @ingroup GoogleARCoreBase
 * Describes the tracking state of the current ARCore session.
 */
UENUM(BlueprintType)
enum class EGoogleARCoreTrackingState : uint8
{
	/** Tracking is valid. */
	Tracking = 0,
	/** Tracking is temporary lost but could recover in the future. */
	Paused = 1,
	/** Tracking is lost will not recover. */
	Stopped = 2
};

/**
 * @ingroup GoogleARCoreBase
 * Describes what type of light estimation will be performed in ARCore session.
 */
UENUM(BlueprintType)
enum class EGoogleARCoreLightEstimationMode : uint8
{
	/** Light estimation disabled. */
	None = 0,
	/** Enable light estimation for ambient intensity. */
	AmbientIntensity = 1,
};

/**
 * @ingroup GoogleARCoreBase
 * Describes what type of plane detection will be performed in ARCore session.
 */
UENUM(BlueprintType)
enum class EGoogleARCorePlaneDetectionMode : uint8
{
	/** Disable plane detection. */
	None = 0,
	/** Track for horizontal plane. */
	HorizontalPlane = 1,
};

/**
 * @ingroup GoogleARCoreBase
 * Describes how the frame will be updated in ARCore session.
 */
UENUM(BlueprintType)
enum class EGoogleARCoreFrameUpdateMode : uint8
{
	/** Unreal tick will be synced with the camera image update rate. */
	SyncTickWithCameraImage = 0,
	/** Unreal tick will not related to the camera image update rate. */
	SyncTickWithoutCameraImage = 1,
};

/**
 * @ingroup GoogleARCoreBase
 * Describes which channel ARLineTrace will be performed on.
 */
UENUM(BlueprintType)
enum class EGoogleARCoreLineTraceChannel : uint8
{
	/** Trace against feature point cloud. */
	FeaturePoint = 0,
	/** Trace against the infinite plane. */
	InfinitePlane = 1,
	/** Trace against the plane using its extent. */
	PlaneUsingExtent = 2,
	/** Trace against the plane using its boundary polygon. */
	PlaneUsingBoundaryPolygon = 3
};

/**
 * A struct describes the ARCore light estimation.
 */
USTRUCT(BlueprintType)
struct FGoogleARCoreLightEstimate
{
	GENERATED_BODY()

	/** Whether this light estimation is valid. */
	UPROPERTY(BlueprintReadOnly, Category = "GoogleARCore|LightEstimate")
	bool bIsValid;

	/** The average pixel intensity of the passthrough camera image. */
	UPROPERTY(BlueprintReadOnly, Category = "GoogleARCore|LightEstimate")
	float PixelIntensity;
};

class FGoogleARCoreSession;
/**
 * A UObject that contains a set of observed 3D points and confidence values.
 */
UCLASS(BlueprintType)
class GOOGLEARCOREBASE_API UGoogleARCorePointCloud : public UObject
{
	friend class FGoogleARCoreFrame;
	friend class FGoogleARCoreSession;

	GENERATED_BODY()
public:
	/** Destructor */
	~UGoogleARCorePointCloud();

	/** Returns the timestamp in nanosecond when this point cloud was observed. */
	int64 GetUpdateTimestamp();

	/** Checks if this point cloud has been updated in this frame. */
	UFUNCTION(BlueprintPure, Category = "UGoogleARCorePointCloud")
	bool IsUpdated();

	/** Returns the number of point inside this point cloud. */
	UFUNCTION(BlueprintPure, Category = "UGoogleARCorePointCloud")
	int GetPointNum();

	/** Returns the point position in Unreal world space given the point index. */
	UFUNCTION(BlueprintPure, Category = "UGoogleARCorePointCloud")
	FVector GetWorldSpacePoint(int Index);

	/** Returns the point confidence value given the point index. */
	UFUNCTION(BlueprintPure, Category = "UGoogleARCorePointCloud")
	float GetPointConfidence(int Index);

	/** Release PointCloud's resources back to ArCore. Data will not be available after releasePointCloud is called. */
	UFUNCTION(BlueprintCallable, Category = "UGoogleARCorePointCloud")
	void ReleasePointCloud();

private:
	TWeakPtr<FGoogleARCoreSession> Session;
	bool bIsUpdated = false;
#if PLATFORM_ANDROID
	ArPointCloud* PointCloudHandle;
#endif
};

/**
* A UObject that describes a fixed location and orientation in the real world.
* To stay at a fixed location in physical space, the numerical description of this position will update
* as ARCore's understanding of the space improves. Use GetLatestPose() to get the latest updated numerical
* location of this anchor.
*/
UCLASS(BlueprintType)
class GOOGLEARCOREBASE_API UGoogleARCoreAnchor : public UObject
{
	GENERATED_BODY()

	friend class FGoogleARCoreSession;
	friend class FGoogleARCoreFrame;
public:

	/**
	* Returns the current state of the pose of this anchor object. If this
	* state is anything but <code>Tracking</code> the
	* pose may be dramatically incorrect.
	*/
	UFUNCTION(BlueprintPure, Category = "GoogleARAnchor")
	EGoogleARCoreTrackingState GetTrackingState()
	{
		return TrackingState;
	};

	/**
	* Returns the pose of the anchor in Unreal world space. This pose
	* should only be considered valid if GetTrackingState() returns
	* <code>Tracking</code>.
	*/
	UFUNCTION(BlueprintPure, Category = "GoogleARAnchor")
	FTransform GetLatestPose()
	{
		return Pose;
	};

private:
#if PLATFORM_ANDROID
	ArAnchor* AnchorHandle;
#endif
	/** The anchor's latest pose in Unreal world space. */
	FTransform Pose;
	/** The anchor's current tracking state. */
	EGoogleARCoreTrackingState TrackingState;
};
