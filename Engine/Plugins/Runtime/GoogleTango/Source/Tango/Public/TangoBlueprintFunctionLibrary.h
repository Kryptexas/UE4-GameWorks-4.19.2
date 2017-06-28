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

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TangoPrimitives.h"

#include "TangoBlueprintFunctionLibrary.generated.h"

UCLASS()
class TANGO_API UTangoBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Get whether Tango is currently running.
	 *
	 * Note that it is technically possible for Tango to stop at any time (for instance, if the Tango Core service
	 * is updated on the device), and thus does not guarantee that Tango will still be bound and running during
	 * subsequent calls to anything.
	 * @return True if Tango is currently running.
	 */
	UFUNCTION(BlueprintPure, Category = "Tango|Lifecycle")
	static bool IsTangoRunning();

	/**
	 * Get a copy of the current FTangoConfiguration that Tango is running with.
	 *
	 * @return True if it get the current Tango configuration successfully. False if the Tango is not currently running.
	 */
	UFUNCTION(BlueprintPure, Category = "Tango|Lifecycle")
	static bool GetCurrentTangoConfig(FTangoConfiguration& OutCurrentTangoConfig);

	/**
	 * Get the latest available Tango device pose. Updated at the beginning of the frame.
	 *
	 * The details of exactly which Tango reference frames this represents may vary depending on your setup.
	 * @param OutPose Pose data.
	 * @return True if the fetched pose is valid. An invalid pose should not be used.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|Motion")
	static bool GetLatestPose(ETangoPoseRefFrame RefFrame, FTangoPose& OutTangoPose);

	/**
	* Get the Tango device pose at Timestamp.
	*
	* The details of exactly which Tango reference frames this represents may vary depending on your setup.
	* @param OutPose Pose data.
	* @return True if the fetched pose is valid. An invalid pose should not be used.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Motion")
	static bool GetPoseAtTime(ETangoPoseRefFrame RefFrame, const FTangoTimestamp& Timestamp, FTangoPose& OutTangoPose);

	/**
	* Start tango. Note: only valid if AutoConnect is false in your settings
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Motion")
	static void StartTangoTracking();


	/**
	* Start tango with the provided configuration.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Motion")
	static void ConfigureAndStartTangoTracking(const FTangoConfiguration& Configuration);

	/**
	* Stop tango
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Motion")
	static void StopTangoTracking();

	/**
	 * Enable/Disable the color camera pass through rendering in Tango HMD. Note that this won't change the camera FOV.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|HMD")
	static void SetPassthroughCameraRenderingEnabled(bool bEnable);

	/**
	 * Get if the AR camera rendering is enabled in Tango HMD.
	 */
	UFUNCTION(BlueprintPure, Category = "Tango|HMD")
	static bool IsPassthroughCameraRenderingEnabled();

	/**
	* Returns the current depth camera FPS
	*/
	UFUNCTION(BlueprintPure, Category = "Tango|Depth Camera")
	static void GetDepthCameraFrameRate(int32& FrameRate);

	/**
	* Sets the depth camera FPS. Assigning a frame rate of zero will
	* disable the camera and prevent energy use.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Depth Camera")
	static bool SetDepthCameraFrameRate(int32 NewFrameRate);

	/**
	* Returns a list of the required runtime permissions for the current
	* configuration suitable for use with the AndroidPermission plugin.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Configuration")
	static bool GetRequiredRuntimePermissions(TArray<FString>& RuntimePermissions)
	{
		FTangoConfiguration Config;
		if (GetCurrentTangoConfig(Config))
		{
			GetRequiredRuntimePermissionsForConfiguration(Config, RuntimePermissions);
			return true;
		}
		return false;
	}

	/**
	* Returns a list of the required runtime permissions for the given
	* configuration suitable for use with the AndroidPermission plugin.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Configuration")
	static void GetRequiredRuntimePermissionsForConfiguration(
		const FTangoConfiguration& Configuration,
		TArray<FString>& RuntimePermissions);

	/**
	* Returns whether the TangoCore service is present on the device
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Configuration")
	static bool IsTangoCorePresent();

	/**
	* Returns whether the version of the TangoCore service on the device is
	* up to date
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|Configuration")
	static bool IsTangoCoreUpToDate();

	// TangoAreaLearning dependencies
	static FTangoConnectionEventDelegate& GetTangoConnectionEventDelegate();
	static FTangoLocalAreaLearningEventDelegate& GetTangoLocalAreaLearningEventDelegate();
	static void RequestExportLocalAreaDescription(const FString& UUID, const FString& Filename);
	static void RequestImportLocalAreaDescription(const FString& Filename);

};
