// Copyright 2017 Google Inc.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LatentActionManager.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GoogleARCoreTypes.h"
#include "GoogleARCoreSessionConfig.h"
#include "GoogleARCoreFunctionLibrary.generated.h"

/** A function library that provides static/Blueprint functions associated with GoogleARCore session.*/
UCLASS()
class GOOGLEARCOREBASE_API UGoogleARCoreSessionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//-----------------Lifecycle---------------------

	/**
	 * A Latent Action to check the availability of ARCore on this device.
	 * This may initiate a query with a remote service to determine if the device is supported by ARCore. The Latent Action will complete when the check is finished.
	 *
	 * @param OutAvailability	The availability result as a EGoogleARCoreAvailability.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|Availability", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", Keywords = "googlear arcore availability"))
	static void CheckARCoreAvailability(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, EGoogleARCoreAvailability& OutAvailability);

	/**
	 * A Latent Action to initiates installation of ARCore if required.
	 * This function may cause your application be paused if installing ARCore is required.
	 *
	 * @param OutInstallResult	The install request result as a EGoogleARCoreInstallRequestResult.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|Availability", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", Keywords = "googlear arcore availability"))
	static void InstallARCoreService(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, EGoogleARCoreInstallRequestResult& OutInstallResult);
	
	/**
	 * A polling function to check the ARCore availability in C++.
	 * This may initiate a query with a remote service to determine if the device is supported by ARCore, so this function will EGoogleARCoreAvailability::UnkownChecking.
	 *
	 * @return	The availability result as a EGoogleARCoreAvailability.
	 */
	static EGoogleARCoreAvailability CheckARCoreAvailableStatus();
	
	/**
	 * Initiates installation of ARCore if required.
	 * This function will return immediately and may pause your application if installing ARCore is required.
	 *
	 * @return EGoogleARCoreInstallStatus::Requrested if it started a install request.
	 */
	static EGoogleARCoreInstallStatus RequestInstallARCoreAPK();
	
	/**
	 * A polling function to check the ARCore install request result in C++.
	 * After you call RequestInstallARCoreAPK() and it returns EGoogleARCoreInstallStatus::Requrested. You can call this function to check the install requst result.
	 *
	 * @return The install request result as a EGoogleARCoreInstallRequestResult.
	 */
	static EGoogleARCoreInstallRequestResult GetARCoreAPKInstallResult();

	/**
	 * Starts a new ARCore tracking session GoogleARCore specific configuration.
	 * If the session already started and the config isn't the same, it will stop the previous session and start a new session with the new config.
	 * Note that this is a latent action, you can query the session start result by querying GetARCoreSessionStatus() after the latent action finished.
	 *
	 * @param Configuration				The ARCoreSession configuration to start the session.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|Session", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", Keywords = "googlear arcore session start config"))
	static void StartARCoreSession(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, UGoogleARCoreSessionConfig* Configuration);

	//-----------------PassthroughCamera---------------------
	/**
	 * Returns the state of the passthrough camera rendering in GoogleARCore ARSystem.
	 *
	 * @return	True if the passthrough camera rendering is enabled.
	 */
	UFUNCTION(BlueprintPure, Category = "GoogleARCore|PassthroughCamera", meta = (Keywords = "googlear arcore passthrough camera"))
	static bool IsPassthroughCameraRenderingEnabled();

	/**
	 * Enables/Disables the passthrough camera rendering in GoogleARCore ARSystem.
	 * Note that when passthrough camera rendering is enabled, the camera FOV will be forced
	 * to match FOV of the physical camera on the device.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|PassthroughCamera", meta = (Keywords = "googlear arcore passthrough camera"))
	static void SetPassthroughCameraRenderingEnabled(bool bEnable);

	/**
	 * Gets the texture coordinate information about the passthrough camera texture.
	 *
	 * @param InUV		The original UVs of on the quad. Should be an array with 8 floats.
	 * @param OutUV		The orientated UVs that can be used to sample the passthrough camera texture and make sure it is displayed correctly.
	 */
	UFUNCTION(BlueprintPure, Category = "GoogleARCore|PassthroughCamera", meta = (Keywords = "googlear arcore passthrough camera uv"))
	static void GetPassthroughCameraImageUV(const TArray<float>& InUV, TArray<float>& OutUV);

	//-------------------Trackables-------------------------
	/**
	 * Gets a list of all valid UARPlaneGeometry objects that ARCore is currently tracking.
	 * Planes that have entered the EARTrackingState::StoppedTracking state or for which 
	 * UARPlaneGeometry::GetSubsumedBy returns non-null will not be included.
	 *
	 * @param OutAnchorList		An array that contains all the valid planes detected by ARCore.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|TrackablePlane", meta = (Keywords = "googlear arcore all plane"))
	static void GetAllPlanes(TArray<UARPlaneGeometry*>& OutPlaneList);
	
	/**
	 * Gets a list of all valid UARTrackedPoint objects that ARCore is currently tracking.
	 * TrackablePoint that have entered the EARTrackingState::StoppedTracking state will not be included.
	 *
	 * @param OutAnchorList		An array that contains all the valid planes detected by ARCore.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|TrackablePoint", meta = (Keywords = "googlear arcore pose transform"))
	static void GetAllTrackablePoints(TArray<UARTrackedPoint*>& OutTrackablePointList);

	/** Template function to get all trackables from a given type. */
	template< class T > static void GetAllTrackable(TArray<T*>& OutTrackableList);
};

/** A function library that provides static/Blueprint functions associated with most recent GoogleARCore tracking frame.*/
UCLASS()
class GOOGLEARCOREBASE_API UGoogleARCoreFrameFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns the current ARCore session status.
	 *
	 * @return	A EARSessionStatus enum that describes the session status.
	 */
	UFUNCTION(BlueprintPure, Category = "GoogleARCore|MotionTracking", meta = (Keywords = "googlear arcore session"))
	static EGoogleARCoreTrackingState GetTrackingState();

	/**
	 * Gets the latest tracking pose in Unreal world space of the ARCore device.
	 *
	 * Note that ARCore motion tracking has already integrated with HMD and the motion controller interface.
	 * Use this function only if you need to implement your own tracking component.
	 *
	 * @param OutPose		The latest device pose.
	 * @return				True if the pose is updated successfully for this frame.
	 */
	UFUNCTION(BlueprintPure, Category = "GoogleARCore|MotionTracking", meta = (Keywords = "googlear arcore pose transform"))
	static void GetPose(FTransform& OutPose);

	/**
	 * Traces a ray from the user's device in the direction of the given location in the camera
	 * view. Intersections with detected scene geometry are returned, sorted by distance from the
	 * device; the nearest intersection is returned first.
	 *
	 * @param WorldContextObject	The world context.
	 * @param ScreenPosition		The position on the screen to cast the ray from.
	 * @param ARObjectType			A set of EGoogleARCoreLineTraceChannel indicate which type of line trace it should perform.
	 * @param OutHitResults			The list of hit results sorted by distance.
	 * @return						True if there is a hit detected.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|LineTrace", meta = (WorldContext = "WorldContextObject", Keywords = "googlear arcore raycast hit"))
	static bool ARCoreLineTrace(UObject* WorldContextObject, const FVector2D& ScreenPosition, TSet<EGoogleARCoreLineTraceChannel> TraceChannels, TArray<FARTraceResult>& OutHitResults);

	/**
	 * Gets a list of UARPin objects that were changed in this frame.
	 *
	 * @param OutAnchorList		An array that contains the updated UARPin.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|ARAnchor", meta = (Keywords = "googlear arcore Anchor"))
	static void GetUpdatedARPins(TArray<UARPin*>& OutAnchorList);

	/**
	 * Gets a list of UARPlaneGeometry objects that were changed in this frame.
	 *
	 * @param OutARCorePlaneList	An array that contains the updated UARPlaneGeometry.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|TrackablePlane", meta = (Keywords = "googlear arcore pose transform"))
	static void GetUpdatedPlanes(TArray<UARPlaneGeometry*>& OutPlaneList);

	/**
	 * Gets a list of UARTrackedPoint objects that were changed in this frame.
	 *
	 * @param OutARCorePlaneList	An array that contains the updated UARTrackedPoint.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|TrackablePoint", meta = (Keywords = "googlear arcore pose transform"))
	static void GetUpdatedTrackablePoints(TArray<UARTrackedPoint*>& OutTrackablePointList);

	/** Template function to get the updated trackables in this frame a given trackable type. */
	template< class T > static void GetUpdatedTrackable(TArray<T*>& OutTrackableList);

	/**
	 * Gets the latest light estimation.
	 *
	 * @param OutLightEstimate		The struct that describes the latest light estimation.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|LightEstimation", meta = (Keywords = "googlear arcore light ambient"))
	static void GetLightEstimation(FGoogleARCoreLightEstimate& OutLightEstimate);

	/**
	 * Gets the latest point cloud that will be only available for this frame.
	 * If you want to keep the point cloud data, you can either copy it to your own struct 
	 * or call AcquireLatestPointCloud() to avoid the copy.
	 * 
	 * @param OutLatestPointCloud		A pointer point to the latest point cloud.
	 * @return  An EGoogleARCoreFunctionStatus. Possible value: Success, SessionPaused, ResourceExhausted.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|PointCloud", meta = (Keywords = "googlear arcore pointcloud"))
	static EGoogleARCoreFunctionStatus GetPointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud);

	/**
	 * Acquires latest point cloud. This will make the point cloud remain valid unless you call UGoogleARCrePointCloud::ReleasePointCloud().
	 * Be aware that this function could fail if the maximal number of point cloud has been acquired.
	 *
	 * @param OutLatestPointCloud		A pointer point to the latest point cloud.
	 * @return  An EGoogleARCoreFunctionStatus. Possible value: Success, SessionPaused, ResourceExhausted.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleARCore|PointCloud", meta = (Keywords = "googlear arcore pointcloud"))
	static EGoogleARCoreFunctionStatus AcquirePointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud);

#if PLATFORM_ANDROID
	/**
	 * Gets the camera metadata for the latest camera image.
	 * Note that ACameraMetadata is a Ndk type. Include the Ndk header <camera/NdkCameraMetadata.h> to use query value from ACameraMetadata.
	 *
	 * @param OutCameraMetadata		A pointer to a ACameraMetadata struct which is only valid in one frame.
	 * @return An EGoogleARCoreFunctionStatus. Possible value: Success, SessionPaused, NotAvailable.
	 */
	static EGoogleARCoreFunctionStatus GetCameraMetadata(const ACameraMetadata*& OutCameraMetadata);
#endif
};
