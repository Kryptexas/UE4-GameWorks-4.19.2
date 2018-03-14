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

UENUM(BlueprintType)
enum class EGoogleARCoreAvailability : uint8
{
	/* An internal error occurred while determining ARCore availability. */
	UnkownError = 0,
	/* ARCore is not installed, and a query has been issued to check if ARCore is is supported. */
	UnkownChecking = 1,
	/*
	 * ARCore is not installed, and the query to check if ARCore is supported
	 * timed out. This may be due to the device being offline.
	 */
	UnkownTimedOut = 2,
	/* ARCore is not supported on this device.*/
	UnsupportedDeviceNotCapable = 100,
	/* The device and Android version are supported, but the ARCore APK is not installed.*/
	SupportedNotInstalled = 201,
	/*
	 * The device and Android version are supported, and a version of the
	 * ARCore APK is installed, but that ARCore APK version is too old.
	 */
	SupportedApkTooOld = 202,
	/* ARCore is supported, installed, and available to use. */
	SupportedInstalled = 203
};

UENUM(BlueprintType)
enum class EGoogleARCoreInstallStatus : uint8
{
	/* The requested resource is already installed.*/
	Installed = 0,
	/* Installation of the resource was requested. The current activity will be paused. */
	Requrested = 1,
};

UENUM(BlueprintType)
enum class EGoogleARCoreInstallRequestResult : uint8
{
	/* The ARCore APK is installed*/
	Installed,
	/* ARCore APK install request failed because the device is not compatible. */
	DeviceNotCompatible,
	/* ARCore APK install request failed because the current version of android is too old to support ARCore. */
	UserDeclinedInstallation,
	/* ARCore APK install request failed because unknown error happens while checking or requesting installation. */
	FatalError
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
	NotTracking = 1,
	/** Tracking is lost will not recover. */
	StoppedTracking = 2
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

/**
 * @ingroup GoogleARCoreBase
 * Describes which channel ARLineTrace will be performed on.
 */
UENUM(BlueprintType, Category = "GoogleARCore|TraceChannel", meta = (Bitflags))
enum class EGoogleARCoreLineTraceChannel : uint8
{
	None = 0,
	/** Trace against feature point cloud. */
	FeaturePoint = 1,
	/** Trace against the infinite plane. */
	InfinitePlane = 2,
	/** Trace against the plane using its extent. */
	PlaneUsingExtent = 4,
	/** Trace against the plane using its boundary polygon. */
	PlaneUsingBoundaryPolygon = 8,
	/**
	 * Trace against feature point and attempt to estimate the normal of the surface centered around the trace hit point.
	 * Surface normal estimation is most likely to succeed on textured surfaces and with camera motion.
	 */
	FeaturePointWithSurfaceNormal = 16,
};
ENUM_CLASS_FLAGS(EGoogleARCoreLineTraceChannel);

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
	UFUNCTION(BlueprintPure, Category = "GoogleARCore|PointCloud")
	bool IsUpdated();

	/** Returns the number of point inside this point cloud. */
	UFUNCTION(BlueprintPure, Category = "GoogleARCore|PointCloud")
	int GetPointNum();

	/** Returns the point position in Unreal world space and it's confidence value from 0 ~ 1. */
	UFUNCTION(BlueprintPure, Category = "GoogleARCore|PointCloud")
	void GetPoint(int Index, FVector& OutWorldPosition, float& OutConfidence);

	/** Release PointCloud's resources back to ArCore. Data will not be available after releasePointCloud is called. */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|PointCloud")
	void ReleasePointCloud();

private:
	TWeakPtr<FGoogleARCoreSession> Session;
	bool bIsUpdated = false;
#if PLATFORM_ANDROID
	ArPointCloud* PointCloudHandle;
#endif
};

/**
* Helper class used to expose FGoogleARCoreSessionConfig setting in the Editor plugin settings.
*/
UCLASS(config = Engine, defaultconfig)
class GOOGLEARCOREBASE_API UGoogleARCoreEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	/** Check this option if you app require ARCore to run on Android. */
	UPROPERTY(EditAnywhere, config, Category = "ARCore Plugin Settings", meta = (ShowOnlyInnerProperties))
	bool bARCoreRequiredApp = true;
};
