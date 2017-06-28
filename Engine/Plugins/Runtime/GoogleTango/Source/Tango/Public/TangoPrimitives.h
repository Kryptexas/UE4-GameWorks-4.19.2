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

#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "TangoPrimitives.generated.h"

 /**
  * Representation of a Tango timestamp.
  *
  * The only valid use of a Tango timestamp is to query the position of
  * Tango device components at that particular time.
  * Using a timestamp with a value of 0 (or an empty pin in Blueprints)
  * indicates that most absolutely-up-to-date pose from the Tango service
  * should be returned.
  */

USTRUCT(BlueprintType)
struct TANGO_API FTangoTimestamp
{
	GENERATED_BODY()
public:
	FTangoTimestamp(double Timestamp = 0) : TimestampValue(Timestamp) {};

	/** The timestamp value that the FTangoTimestamp is a wrapper for. */
	double TimestampValue;
};

/**
 * A pose from the Tango service, along with timing information for querying
 * other, temporally synchronous poses.
 */
USTRUCT(BlueprintType)
struct TANGO_API FTangoPose
{
	GENERATED_BODY()

	/** Position and rotation of the pose. */
	UPROPERTY(BlueprintReadOnly, Category = Tango)
	FTransform Pose;

	/** Holds a timestamp for synchronizing Tango poses. */
	UPROPERTY(BlueprintReadOnly, Category = Tango)
	FTangoTimestamp Timestamp;
};

UENUM(BlueprintType)
enum class ETangoMotionTrackingMode : uint8
{
	/** Use default motion tracking */
	DEFAULT,
	/**
	* Use persistent cloud area tiles. Your
	* location must be mapped for this to work. Note: requires Location permission.
	*/
	VPS,
	/** Use transient device local drift correction */
	DRIFT_CORRECTION,
	/** Perform device local area learning */
	LOCAL_AREA_LEARNING
};

/**
 * All the settings for the Tango Service that cannot be changed while the service
 * is running.
 */
USTRUCT(BlueprintType)
struct TANGO_API FTangoConfiguration
{
	GENERATED_BODY()
public:
	/** Whether to automatically start Tango tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoServiceConfig")
	bool bAutoConnect = true;

	/**
	* Whether to automatically request required runtime permissions for this
	* configuration
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoServiceConfig")
	bool bAutoRequestRuntimePermissions = true;

	/** Enable Depth Camera - Note: requires camera permission */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoCameraConfig")
	bool bEnableDepthCamera = true;

    /** Initial frame rate - Note: zero will keep the camera disabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoCameraConfig", meta = (EditCondition = "bEnableDepthCamera"))
	int32 InitialDepthCameraFrameRate = 5;

	/** Enable Color Camera - Note: requires camera permission */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoCameraConfig")
	bool bEnablePassthroughCamera = true;

	/** Whether to synchronize the game frame rate with the AR camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoCameraConfig", meta = (EditCondition = "bEnablePassthroughCamera"))
	bool bSyncGameFrameRateWithPassthroughCamera = false;

	/** Link the camera component with Tango pose. When enabled, Tango HMD will be used to update camera pose and rendering pass through camera image when enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoCameraConfig")
	bool bLinkCameraToTangoDevice = true;

	/** Enable the pass through camera rendering controlled by the Tango HMD. If the is enabled the camera field of view that is connected with tango
	    will always match the physical camera on tango device.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoCameraConfig", meta = (EditCondition = "bLinkCameraToTangoDevice"))
	bool bEnablePassthroughCameraRendering = true;

	/** Use late update to update camera pose on render thread just before rendering the scene.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoCameraConfig", meta = (EditCondition = "bLinkCameraToTangoDevice"))
	bool bSyncOnLateUpdate = true;

	/** Motion tracking mode - Note: non-defaults incur additional energy cost */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoMotionConfig")
	ETangoMotionTrackingMode MotionTrackingMode = ETangoMotionTrackingMode::DEFAULT;

	/**
	* Unique ID of the local area description to use. The behavior depends on
	* the MotionTrackingMode as follows:
	*  DEFAULT - if the ID is non-empty the area description will be used but
	*    no new features will be learned.
	*  LOCAL_AREA_LEARNING - New features will be learned and appended to the
	*    area description. If the ID is empty a new area description will be
	*    created.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TangoMotionConfig")
	FString LocalAreaDescriptionID;

	inline bool operator==(const FTangoConfiguration& rhs) const
	{
		return bAutoConnect == rhs.bAutoConnect &&
			bEnableDepthCamera == rhs.bEnableDepthCamera &&
			InitialDepthCameraFrameRate == rhs.InitialDepthCameraFrameRate &&
			bEnablePassthroughCamera == rhs.bEnablePassthroughCamera &&
			bLinkCameraToTangoDevice == rhs.bLinkCameraToTangoDevice &&
			bEnablePassthroughCameraRendering == rhs.bEnablePassthroughCameraRendering &&
			bSyncOnLateUpdate == rhs.bSyncOnLateUpdate &&
			MotionTrackingMode == rhs.MotionTrackingMode &&
			bSyncGameFrameRateWithPassthroughCamera == rhs.bSyncGameFrameRateWithPassthroughCamera &&
			LocalAreaDescriptionID == rhs.LocalAreaDescriptionID &&
			bAutoRequestRuntimePermissions == rhs.bAutoRequestRuntimePermissions;
	}
};

// We use the default object to load the TangoCameraMaterial.
// This will make sure the material got cooked in to the game.
UCLASS()
class TANGO_API UTangoCameraOverlayMaterialLoader : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UMaterialInterface* DefaultCameraOverlayMaterial;

	UTangoCameraOverlayMaterialLoader()
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultOverlayMaterialRef(TEXT("/GoogleTango/Tango/TangoCameraMaterial.TangoCameraMaterial"));
		DefaultCameraOverlayMaterial = DefaultOverlayMaterialRef.Object;
	}
};

/**
* Helper class used to expose Tango settings in the Editor.
*/
UCLASS(config = Engine, defaultconfig)
class TANGO_API UTangoEditorSettings  : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, config, Category = TangoServiceConfiguration, meta = (ShowOnlyInnerProperties))
	FTangoConfiguration TangoConfiguration;
};

/**
 * Coordinate reference frames natively supported by Tango.
 *
 * Values correspond to Tango's TangoCoordinateFrameType enumeration.
 */
enum class ETangoReferenceFrame : uint8
{
	/**
	 * Coordinate system for the entire Earth.
	 * See WGS84:
	 * <a href="http://en.wikipedia.org/wiki/World_Geodetic_System">World
	 * Geodetic System</a>
	 */
	GLOBAL_WGS84 = 0 UMETA(Hidden),
	/** Origin within a saved area description. */
	AREA_DESCRIPTION = 1,
	/** Origin when the device started tracking. */
	START_OF_SERVICE = 2,
	/** Immediately previous device pose (deprecated / not well supported). */
	PREVIOUS_DEVICE_POSE UMETA(Hidden) = 3,
	/** Device coordinate frame. */
	DEVICE = 4,
	/** Inertial Measurement Unit. */
	IMU UMETA(Hidden) = 5,
	/** Display */
	DISPLAY UMETA(Hidden) = 6,
	/** Color camera */
	CAMERA_COLOR = 7,
	/** Depth camera */
	CAMERA_DEPTH UMETA(Hidden) = 8,
	/** Fisheye camera */
	CAMERA_FISHEYE UMETA(Hidden) = 9,
	/** Tango unique id */
	UUID UMETA(Hidden) = 10,
	/** Invalid. */
	INVALID UMETA(Hidden) = 11,
	/** Maximum Allowed. */
	MAX UMETA(Hidden) = 12
};

UENUM(BlueprintType)
enum class ETangoPoseRefFrame : uint8
{
	// The values here need to be kept in line with ETangoReferenceFrame, we cannot assign the enum directly
	// due to some limitation on the UENUM.
	DEVICE = 4,
	CAMERA_COLOR = 7
};

UENUM(BlueprintType)
enum class ETangoLocalAreaLearningEventType : uint8
{
	SAVE_PROGRESS,
	EXPORT_RESULT,
	IMPORT_RESULT
};

USTRUCT(BlueprintType)
struct FTangoLocalAreaLearningEvent
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Local Area Learning")
	ETangoLocalAreaLearningEventType EventType;
	/**
	* Depends on EventType as follows
	* case SAVE_PROGRESS:
	*    0..100 indicating the percentage of progress
	* case EXPORT_RESULT:
	* case IMPORT_RESULT:
	*    1 or 0 indicating success or failure
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango|Local Area Learning")
	int32 EventValue;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FTangoLocalAreaLearningEventDelegate, FTangoLocalAreaLearningEvent);
DECLARE_MULTICAST_DELEGATE_OneParam(FTangoConnectionEventDelegate, bool);

