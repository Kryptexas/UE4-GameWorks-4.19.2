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

#include "CoreDelegates.h"
#include "Engine/EngineBaseTypes.h"

#include "TangoPrimitives.h"
#include "TangoMotion.h"
#include "TangoARCamera.h"
#include "TangoPointCloud.h"

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#include "Android/AndroidJNI.h"
#endif

DECLARE_MULTICAST_DELEGATE(FOnTangoServiceBound);
DECLARE_MULTICAST_DELEGATE(FOnTangoServiceUnbound);

class FTangoDevice
{
public:
	static FTangoDevice* GetInstance();

	FTangoDevice();

	/**
	 * Whether a proper connection with the Tango Core system service is currently established.
	 * When this is is false much of Tango's functionality will be unavailable.
	 *
	 * Note that it is technically possible for Tango to stop at any time (for instance, if the Tango Core service
	 * is updated on the device), and thus does not guarantee that Tango will still be bound during
	 * subsequent calls to anything.
	 * @return True if a proper connection with the Tango Core system service is currently established.
	 */
	bool GetIsTangoBound();

	/**
	 * Get whether Tango is currently running.
	 *
	 * Note that it is technically possible for Tango to stop at any time (for instance, if the Tango Core service
	 * is updated on the device), and thus does not gaurentee that Tango will still be bound and running during
	 * subsequent calls to anything.
	 * @return True if Tango is currently running.
	 */
	bool GetIsTangoRunning();

	/**
	 * Update Tango plugin to use a new configuration.
	 */
	void UpdateTangoConfiguration(const FTangoConfiguration& NewConfiguration);

	/**
	 * Reset Tango plugin to use the global project Tango configuration.
	 */
	void ResetTangoConfiguration();

	bool GetCurrentTangoConfig(FTangoConfiguration& OutCurrentTangoConfig);

#if PLATFORM_ANDROID
	/**
	 * Get the current TangoConfig object that Tango is running with. Will return NULL if Tango is not running.
	 */
	TangoConfig GetCurrentLowLevelTangoConfig();
#endif

	/**
	 * Get the current base frame Tango is running on.
	 */
	ETangoReferenceFrame GetCurrentBaseFrame();

	/**
	 * Get the base frame Tango from the given Tango configuration.
	 */
	ETangoReferenceFrame GetBaseFrame(FTangoConfiguration TangoConfig);

	/**
	 * Get Unreal Units per meter, based off of the current map's VR World to Meters setting.
	 * @return Unreal Units per meter.
	 */
	float GetWorldToMetersScale();

	int32 GetDepthCameraFrameRate();
	bool SetDepthCameraFrameRate(int32 NewFrameRate);

public:
	FTangoMotion TangoMotionManager;
	FTangoARCamera TangoARCameraManager;
	FTangoPointCloud TangoPointCloudManager;

	FOnTangoServiceBound OnTangoServiceBoundDelegate;
	FOnTangoServiceUnbound OnTangoServiceUnboundDelegate;

	void StartTangoTracking();
	void StopTangoTracking();

	FTangoLocalAreaLearningEventDelegate TangoLocalAreaLearningEventDelegate;
	FTangoConnectionEventDelegate TangoConnectionEventDelegate;

	bool IsUsingEcef()
	{
		return (LastKnownConfig.MotionTrackingMode == ETangoMotionTrackingMode::VPS ||
			LastKnownConfig.LocalAreaDescriptionID.Len() > 0);
	}

	void RunOnGameThread(TFunction<void()> Func)
	{
		RunOnGameThreadQueue.Enqueue(Func);
	}

	void GetRequiredRuntimePermissionsForConfiguration(const FTangoConfiguration& Config, TArray<FString>& RuntimePermissions)
	{
		RuntimePermissions.Reset();
		if (Config.bEnablePassthroughCamera || Config.bEnableDepthCamera)
		{
			RuntimePermissions.Add("android.permission.CAMERA");
		}
		switch (Config.MotionTrackingMode)
		{
		case ETangoMotionTrackingMode::VPS:
		case ETangoMotionTrackingMode::LOCAL_AREA_LEARNING:
			RuntimePermissions.Add("android.permission.ACCESS_FINE_LOCATION");
		}
	}
	void HandleRuntimePermissionsGranted(const TArray<FString>& Permissions, const TArray<bool>& Granted);

private:
	// Android lifecycle events.
	void OnApplicationCreated();
	void OnApplicationDestroyed();
	void OnApplicationPause();
	void OnApplicationResume();
	void OnApplicationStart();
	void OnApplicationStop();
	void OnAreaDescriptionPermissionResult(bool bWasGranted);
	// Unreal plugin events.
	void OnModuleLoaded();
	void OnModuleUnloaded();

	void OnWorldTickStart(ELevelTick TickType, float DeltaTime);

	bool StartTangoTracking(const FTangoConfiguration& ConfigurationData);
	bool DoStartTangoTracking(const FTangoConfiguration& ConfigurationData);

#if PLATFORM_ANDROID
	// Tango Service bound/unboud events, routed from jni
	void OnTangoServiceBound(JNIEnv * jni, jobject iBinder);
	void OnTangoServiceUnbound();
#endif

	friend class FTangoAndroidHelper;
	friend class FTangoPlugin;

private:

	bool bTangoIsBound; // Whether a proper connection with the Tango Core system service is currently established.
	bool bTangoIsRunning; // Whether Tango is currently running.

	bool bTangoConfigChanged;
	bool bAreaDescriptionPermissionRequested;
	bool bAndroidRuntimePermissionsRequested;
	bool bAndroidRuntimePermissionsGranted;
	bool bStartTangoTrackingRequested; // User called StartTangoTracking
	bool bShouldTangoRestart; // Start tracking on activity start
	float WorldToMeterScale;
	class UTangoAndroidPermissionHandler* PermissionHandler;

	FTangoConfiguration ProjectTangoConfig; // Project Tango config set from Tango Plugin Setting
	FTangoConfiguration RequestTangoConfig; // The current request tango config, could be different than the project config
	FTangoConfiguration LastKnownConfig; // Record of the configuration Tango was last successfully started with

#if PLATFORM_ANDROID
	TangoConfig LowLevelTangoConfig; // Low level Tango Config object in Tango client api
#endif

	static void TangoEventRouter(void *Ptr, const struct TangoEvent* Event);
	void OnTangoEvent(const struct TangoEvent* Event);

    /**
	 * Used as memory barrier in updating and checking whether tango is bound / running.
	 *
	 * Might be overkill because by nature neither of those values can truly be trusted
	 * (i.e. the Tango core service could have crashed/updated but not yet reported a disconnect).
	 */
	FCriticalSection TangoStateLock;

	TQueue<TFunction<void()>> RunOnGameThreadQueue;
};
