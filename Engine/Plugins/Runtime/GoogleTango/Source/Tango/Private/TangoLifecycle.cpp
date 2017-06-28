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

#include "TangoLifecycle.h"
#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h" // for FWorldDelegates
#include "Engine/Engine.h"
#include "TangoARHMD.h"
#include "TangoAndroidHelper.h"
#include "TangoPluginPrivate.h"

#if PLATFORM_ANDROID
#include "AndroidApplication.h"

#include "tango_client_api_dynamic.h"
#include "tango_support_api.h"
#endif

#include "TangoAndroidPermissionHandler.h"

namespace
{
#if PLATFORM_ANDROID
	/** Keys for setting configuration values in the Tango client library. */
	struct TangoConfigKeys
	{
		static constexpr const char* ENABLE_MOTION_TRACKING = "config_enable_motion_tracking";
		static constexpr const char* ENABLE_MOTION_TRACKING_AUTO_RECOVERY = "config_enable_auto_recovery";
		static constexpr const char* ENABLE_LOW_LATENCY_IMU_INTEGRATION = "config_enable_low_latency_imu_integration";
		static constexpr const char* ENABLE_DEPTH = "config_enable_depth";
		static constexpr const char* ENABLE_COLOR = "config_enable_color_camera";
		static constexpr const char* ENABLE_HIGH_RATE_POSE = "config_high_rate_pose";
		static constexpr const char* ENABLE_SMOOTH_POSE = "config_smooth_pose";
		static constexpr const char* DEPTH_MODE = "config_depth_mode";
		static constexpr const char* ENABLE_DRIFT_CORRECTION = "config_enable_drift_correction";
		static constexpr const char* ENABLE_CLOUD_ADF = "config_experimental_use_cloud_adf";
		static constexpr const char* DEPTH_CAMERA_FRAMERATE = "config_runtime_depth_framerate";
		static constexpr const char* ENABLE_LEARNING_MODE = "config_enable_learning_mode";
		static constexpr const char* LOAD_AREA_DESCRIPTION_UUID = "config_load_area_description_UUID";
	};

	bool SetTangoAPIConfigBool(TangoConfig Config, const char* Key, bool Value)
	{
		TangoErrorType SetConfigResult;
		SetConfigResult = TangoConfig_setBool_dynamic(Config, Key, Value);
		if (SetConfigResult != TANGO_SUCCESS)
		{
			UE_LOG(LogTango, Warning, TEXT("Failed to set Tango configuration %s to value of %d"), *FString(Key), Value);
		}
		else
		{
			UE_LOG(LogTango, Log, TEXT("Set Tango configuration %s to value of %d"), *FString(Key), Value);
		}
		return SetConfigResult == TANGO_SUCCESS;
	}

	bool SetTangoAPIConfigString(TangoConfig Config, const char* Key, const FString& InValue)
	{
		const char* Value = TCHAR_TO_UTF8(*InValue);
		TangoErrorType SetConfigResult;
		SetConfigResult = TangoConfig_setString_dynamic(Config, Key, Value);
		if (SetConfigResult != TANGO_SUCCESS)
		{
			UE_LOG(LogTango, Warning, TEXT("Failed to set Tango configuration %s to value of %s"), *FString(Key), *InValue);
		}
		else
		{
			UE_LOG(LogTango, Log, TEXT("Set Tango configuration %s to value of %s"), *FString(Key), *InValue);
		}
		return SetConfigResult == TANGO_SUCCESS;
	}

	bool SetTangoAPIConfigInt32(TangoConfig Config, const char* Key, int32 Value)
	{
		TangoErrorType SetConfigResult;
		SetConfigResult = TangoConfig_setInt32_dynamic(Config, Key, Value);
		if (SetConfigResult != TANGO_SUCCESS)
		{
			UE_LOG(LogTango, Warning, TEXT("Failed to set Tango configuration %s to value of %d"), *FString(Key), Value);
		}
		else
		{
			UE_LOG(LogTango, Log, TEXT("Set Tango configuration %s to value of %d"), *FString(Key), Value);
		}
		return SetConfigResult == TANGO_SUCCESS;
	}

	void SetupClientAPIConfigForCurrentSettings(void* InOutLowLevelConfig, const FTangoConfiguration& TangoConfig)
	{
		SetTangoAPIConfigBool(InOutLowLevelConfig, TangoConfigKeys::ENABLE_MOTION_TRACKING, true);
		SetTangoAPIConfigBool(InOutLowLevelConfig, TangoConfigKeys::ENABLE_MOTION_TRACKING_AUTO_RECOVERY, true);
		SetTangoAPIConfigBool(InOutLowLevelConfig, TangoConfigKeys::ENABLE_LOW_LATENCY_IMU_INTEGRATION, true);

		SetTangoAPIConfigBool(InOutLowLevelConfig, TangoConfigKeys::ENABLE_DEPTH, TangoConfig.bEnableDepthCamera);
		SetTangoAPIConfigBool(InOutLowLevelConfig, TangoConfigKeys::ENABLE_COLOR, TangoConfig.bEnablePassthroughCamera);
		SetTangoAPIConfigBool(
			InOutLowLevelConfig, TangoConfigKeys::ENABLE_LEARNING_MODE,
			TangoConfig.MotionTrackingMode == ETangoMotionTrackingMode::LOCAL_AREA_LEARNING
		);
		SetTangoAPIConfigString(
			InOutLowLevelConfig,
			TangoConfigKeys::LOAD_AREA_DESCRIPTION_UUID,
			(TangoConfig.MotionTrackingMode == ETangoMotionTrackingMode::DEFAULT ||
			TangoConfig.MotionTrackingMode == ETangoMotionTrackingMode::LOCAL_AREA_LEARNING) ?
			TangoConfig.LocalAreaDescriptionID : FString()
		);
		SetTangoAPIConfigBool(
			InOutLowLevelConfig, TangoConfigKeys::ENABLE_DRIFT_CORRECTION,
			TangoConfig.MotionTrackingMode == ETangoMotionTrackingMode::DRIFT_CORRECTION
		);
		SetTangoAPIConfigBool(
			InOutLowLevelConfig, TangoConfigKeys::ENABLE_CLOUD_ADF,
			TangoConfig.MotionTrackingMode == ETangoMotionTrackingMode::VPS
		);
		SetTangoAPIConfigBool(InOutLowLevelConfig, TangoConfigKeys::ENABLE_HIGH_RATE_POSE, false);
		SetTangoAPIConfigBool(InOutLowLevelConfig, TangoConfigKeys::ENABLE_SMOOTH_POSE, false);
		SetTangoAPIConfigInt32(InOutLowLevelConfig, TangoConfigKeys::DEPTH_MODE, (int32)TANGO_POINTCLOUD_XYZC);
	}
#endif
}

static bool bTangoSupportLibraryIntialized = false;

FTangoDevice* FTangoDevice::GetInstance()
{
	static FTangoDevice Inst;
	return &Inst;
}

FTangoDevice::FTangoDevice()
	: bTangoIsBound(false)
	, bTangoIsRunning(false)
	, bTangoConfigChanged(false)
	, bAreaDescriptionPermissionRequested(false)
	, bAndroidRuntimePermissionsRequested(false)
	, bAndroidRuntimePermissionsGranted(false)
	, bStartTangoTrackingRequested(false)
	, bShouldTangoRestart(false)
	, WorldToMeterScale(100.0f)
	, PermissionHandler(nullptr)
#if PLATFORM_ANDROID
	, LowLevelTangoConfig(nullptr)
#endif
{
}

// Tango Service Bind/Unbind
void FTangoDevice::OnModuleLoaded()
{
	if (FTangoAndroidHelper::IsTangoCoreUpToDate())
	{
		ProjectTangoConfig = GetDefault<UTangoEditorSettings>()->TangoConfiguration;
		RequestTangoConfig = ProjectTangoConfig;
		TangoARCameraManager.SetDefaultCameraOverlayMaterial(GetDefault<UTangoCameraOverlayMaterialLoader>()->DefaultCameraOverlayMaterial);

#if PLATFORM_ANDROID
		if (!FTangoAndroidHelper::InitiateTangoServiceBind())
		{
			UE_LOG(LogTango, Error, TEXT("Unable to bind to Tango service. Things may not work."));
		}
#endif
		FWorldDelegates::OnWorldTickStart.AddRaw(this, &FTangoDevice::OnWorldTickStart);
	}
	else
	{
		if (!FTangoAndroidHelper::IsTangoCorePresent())
		{
			UE_LOG(LogTango, Error, TEXT("Fatal error: No TangoCore present on this device"))
		}
		else
		{
			UE_LOG(LogTango, Error, TEXT("Fatal error: TangoCore version on this device isn't up to date"));
		}
	}
}

void FTangoDevice::OnModuleUnloaded()
{
	if (FTangoAndroidHelper::IsTangoCoreUpToDate())
	{
#if PLATFORM_ANDROID
		FTangoAndroidHelper::UnbindTangoService();
#endif
		FWorldDelegates::OnWorldTickStart.RemoveAll(this);
	}
}

void FTangoDevice::TangoEventRouter(void*, const struct TangoEvent* Event)
{
	FTangoDevice::GetInstance()->OnTangoEvent(Event);
}

void FTangoDevice::OnTangoEvent(const struct TangoEvent* InEvent)
{
#if PLATFORM_ANDROID
	if (InEvent != nullptr)
	{
		UE_LOG(LogTango, Log, TEXT("TangoEvent: %d: %s"), InEvent->type, *FString(InEvent->event_value));
		if (InEvent->type == TANGO_EVENT_AREA_LEARNING)
		{
			FTangoLocalAreaLearningEvent Event;
			Event.EventType = ETangoLocalAreaLearningEventType::SAVE_PROGRESS;
			Event.EventValue = FMath::RoundToInt(atof(InEvent->event_value) * 100.0f);
			RunOnGameThread([=]()->void {
				TangoLocalAreaLearningEventDelegate.Broadcast(Event);
			});
		}
	}
#endif
}


#if PLATFORM_ANDROID
// Handling Tango service bound/unbound events.
void FTangoDevice::OnTangoServiceBound(JNIEnv * jni, jobject iBinder)
{
	TangoErrorType BindCompletionResult;
	{
		FScopeLock ScopeLock(&TangoStateLock);

		// Complete binding setup by sending the binder to the Tango client api shared-object.
		BindCompletionResult = TangoService_setBinder_dynamic(jni, iBinder);

		bTangoIsBound = (BindCompletionResult == TANGO_SUCCESS);
	}

	if (BindCompletionResult != TANGO_SUCCESS)
	{
		UE_LOG(LogTango, Error, TEXT("Critical Error in Tango Plugin: Could not set binder in Tango client library."));
	}
	else
	{
		UE_LOG(LogTango, Log, TEXT("Tango binder set in tango client library."));

		OnTangoServiceBoundDelegate.Broadcast();
		if (TangoService_connectOnTangoEvent_dynamic(&FTangoDevice::TangoEventRouter) != TANGO_SUCCESS)
		{
			UE_LOG(LogTango, Error, TEXT("connectOnTangoEvent failed"));
		}
	}

}

void FTangoDevice::OnTangoServiceUnbound()
{
	FScopeLock ScopeLock(&TangoStateLock);
	bTangoIsBound = false;
	bTangoIsRunning = false;
	OnTangoServiceUnboundDelegate.Broadcast();
}
#endif

bool FTangoDevice::GetIsTangoBound()
{
	bool bRetVal;
	{
		FScopeLock ScopeLock(&TangoStateLock);
		bRetVal = bTangoIsBound;
	}

	return bRetVal;
}

bool FTangoDevice::GetIsTangoRunning()
{
	bool bRetVal;
	{
		FScopeLock ScopeLock(&TangoStateLock);
		bRetVal = bTangoIsBound && bTangoIsRunning;
	}

	return bRetVal;
}

#if PLATFORM_ANDROID
TangoConfig FTangoDevice::GetCurrentLowLevelTangoConfig()
{
	return LowLevelTangoConfig;
}
#endif

void FTangoDevice::UpdateTangoConfiguration(const FTangoConfiguration& InMapConfiguration)
{
	RequestTangoConfig = InMapConfiguration;
	bTangoConfigChanged = !(RequestTangoConfig == LastKnownConfig);
	UE_LOG(LogTango, Log, TEXT("Tango configuration updated"));
}

void FTangoDevice::ResetTangoConfiguration()
{
	RequestTangoConfig = ProjectTangoConfig;
	bTangoConfigChanged = !(RequestTangoConfig == LastKnownConfig);
	UE_LOG(LogTango, Log, TEXT("Tango configuration reset"));
}

bool FTangoDevice::GetCurrentTangoConfig(FTangoConfiguration& OutCurrentTangoConfig)
{
	if (!GetIsTangoRunning())
	{
		OutCurrentTangoConfig = ProjectTangoConfig;
		return false;
	}

	OutCurrentTangoConfig = LastKnownConfig;

	return true;
}

ETangoReferenceFrame FTangoDevice::GetCurrentBaseFrame()
{
	return GetBaseFrame(LastKnownConfig);
}

ETangoReferenceFrame FTangoDevice::GetBaseFrame(FTangoConfiguration TangoConfig)
{
	switch (TangoConfig.MotionTrackingMode)
	{
	case ETangoMotionTrackingMode::DEFAULT:
		if (TangoConfig.LocalAreaDescriptionID.Len() == 0)
		{
			return ETangoReferenceFrame::START_OF_SERVICE;
		}
		break;
	case ETangoMotionTrackingMode::LOCAL_AREA_LEARNING:
	case ETangoMotionTrackingMode::DRIFT_CORRECTION:
		return ETangoReferenceFrame::AREA_DESCRIPTION;
	case ETangoMotionTrackingMode::VPS:
		break;
	}
	return ETangoReferenceFrame::GLOBAL_WGS84;
}

float FTangoDevice::GetWorldToMetersScale()
{
	return WorldToMeterScale;
}

void FTangoDevice::OnWorldTickStart(ELevelTick TickType, float DeltaTime)
{
	WorldToMeterScale = GWorld->GetWorldSettings()->WorldToMeters;
	TFunction<void()> Func;
	while (RunOnGameThreadQueue.Dequeue(Func))
	{
		Func();
	}
	if (bTangoConfigChanged)
	{
		UE_LOG(LogTango, Log, TEXT("Tango Config Changed"));
		// Invalidate runtime permissions
		bAndroidRuntimePermissionsRequested = false;
		bAndroidRuntimePermissionsGranted = false;
		if (bTangoIsRunning)
		{
			StopTangoTracking();
		}
		bTangoConfigChanged = false;
	}

	if (!bTangoIsRunning && bTangoIsBound && (RequestTangoConfig.bAutoConnect || bStartTangoTrackingRequested))
	{
		if (StartTangoTracking(RequestTangoConfig))
		{
			bStartTangoTrackingRequested = false;
			ETangoReferenceFrame CurrentBaseFrame = GetCurrentBaseFrame();
			UE_LOG(LogTango, Log, TEXT("Current Base Frame: %d"), (int32)CurrentBaseFrame);
			TangoMotionManager.UpdateBaseFrame(CurrentBaseFrame);
			TangoPointCloudManager.UpdateBaseFrame(CurrentBaseFrame);
		}
	}
	if (bTangoIsRunning)
	{
		TangoMotionManager.UpdateTangoPoses();
		if (LastKnownConfig.bEnableDepthCamera)
		{
			TangoPointCloudManager.UpdatePointCloud();
		}
	}
}

// Called from blueprint library
void FTangoDevice::StartTangoTracking()
{
	if (bTangoIsRunning)
	{
		return;
	}
	UE_LOG(LogTango, Log, TEXT("Start Tango tracking requested"));
	bStartTangoTrackingRequested = true;
}

void FTangoDevice::HandleRuntimePermissionsGranted(const TArray<FString>& RuntimePermissions, const TArray<bool>& Granted)
{
	bool bGranted = true;
	for (int32 i = 0; i < RuntimePermissions.Num(); i++)
	{
		if (!Granted[i])
		{
			bGranted = false;
			UE_LOG(LogTango, Error, TEXT("Android runtime permission denied: %s"), *RuntimePermissions[i]);
		}
		else
		{
			UE_LOG(LogTango, Log, TEXT("Android runtime permission granted: %s"), *RuntimePermissions[i]);
		}
	}
	bAndroidRuntimePermissionsGranted = bGranted;
}

bool FTangoDevice::StartTangoTracking(const FTangoConfiguration& ConfigurationData)
{
	if (!FTangoAndroidHelper::IsTangoCoreUpToDate())
	{
		return false;
	}
	if (ConfigurationData.bAutoRequestRuntimePermissions)
	{
		if (!bAndroidRuntimePermissionsRequested)
		{
			TArray<FString> RuntimePermissions;
			TArray<FString> NeededPermissions;
			GetRequiredRuntimePermissionsForConfiguration(ConfigurationData, RuntimePermissions);
			if (RuntimePermissions.Num() > 0)
			{
				for (int32 i = 0; i < RuntimePermissions.Num(); i++)
				{
					if (!UTangoAndroidPermissionHandler::CheckRuntimePermission(RuntimePermissions[i]))
					{
						NeededPermissions.Add(RuntimePermissions[i]);
					}
				}
			}
			if (NeededPermissions.Num() > 0)
			{
				bAndroidRuntimePermissionsGranted = false;
				bAndroidRuntimePermissionsRequested = true;
				if (PermissionHandler == nullptr)
				{
					PermissionHandler = NewObject<UTangoAndroidPermissionHandler>();
					PermissionHandler->AddToRoot();
				}
				PermissionHandler->RequestRuntimePermissions(NeededPermissions);
			}
			else
			{
				bAndroidRuntimePermissionsGranted = true;
			}
		}
	}
	if (bAndroidRuntimePermissionsRequested && !bAndroidRuntimePermissionsGranted)
	{
		return false;
	}

	if (ConfigurationData.MotionTrackingMode == ETangoMotionTrackingMode::VPS ||
		ConfigurationData.MotionTrackingMode == ETangoMotionTrackingMode::LOCAL_AREA_LEARNING)
	{
		if (!FTangoAndroidHelper::HasAreaDescriptionPermission())
		{
			if (!bAreaDescriptionPermissionRequested)
			{
				bAreaDescriptionPermissionRequested = true;
				FTangoAndroidHelper::RequestAreaDescriptionPermission();
			}
			return false;
		}
	}
	return DoStartTangoTracking(ConfigurationData);
}

bool FTangoDevice::DoStartTangoTracking(const FTangoConfiguration& ConfigurationData)
{
	UE_LOG(LogTango, Log, TEXT("Start Tango tracking..."));

#if PLATFORM_ANDROID

	TangoConfig TangoConfiguration = TangoService_getConfig_dynamic(TANGO_CONFIG_DEFAULT);

	if (TangoConfiguration != nullptr)
	{
		// Apply settings . . .
		SetupClientAPIConfigForCurrentSettings(TangoConfiguration, ConfigurationData);

		// Start Tango.
		TangoErrorType ConnectError;
		{
			FScopeLock ScopeLock(&TangoStateLock);
			if (bTangoIsRunning)
			{
				UE_LOG(LogTango, Log, TEXT("Could not start Tango because Tango is already running!"));
				TangoConfig_free_dynamic(TangoConfiguration);
				return false;
			}

			if (!TangoMotionManager.ConnectOnPoseAvailable(GetBaseFrame(ConfigurationData)))
			{
				UE_LOG(LogTango, Error, TEXT("Failed to connect Tango On PoseAvailable"));
				TangoConfig_free_dynamic(TangoConfiguration);
				return false;
			}

			if (ConfigurationData.bEnablePassthroughCamera)
			{
				if (!TangoARCameraManager.ConnectTangoColorCamera())
				{
					UE_LOG(LogTango, Error, TEXT("Failed to connect Tango Color Camera"));
					TangoConfig_free_dynamic(TangoConfiguration);
					return false;
				}
			}

			if (ConfigurationData.bEnableDepthCamera)
			{
				if (!TangoPointCloudManager.ConnectPointCloud(TangoConfiguration))
				{
					UE_LOG(LogTango, Error, TEXT("Failed to connect Tango Point Cloud"));
					TangoConfig_free_dynamic(TangoConfiguration);
					return false;
				}
			}

			if (GEngine->HMDDevice.IsValid())
			{
				FTangoARHMD* TangoHMD = static_cast<FTangoARHMD*>(GEngine->HMDDevice.Get());
				if (TangoHMD)
				{
					TangoHMD->ConfigTangoHMD(ConfigurationData.bLinkCameraToTangoDevice, ConfigurationData.bEnablePassthroughCameraRendering, ConfigurationData.bSyncOnLateUpdate);
				}
				else
				{
					UE_LOG(LogTango, Error, TEXT("ERROR: TangoHMD is not available."));
				}
			}
			TangoARCameraManager.SetSyncGameFramerateWithCamera(ConfigurationData.bSyncGameFrameRateWithPassthroughCamera);
			ConnectError = TangoService_connect_dynamic(this, TangoConfiguration);
			bTangoIsRunning = ConnectError == TANGO_SUCCESS;
		}
		if (LowLevelTangoConfig != nullptr)
		{
			TangoConfig_free_dynamic(LowLevelTangoConfig);
		}
		LowLevelTangoConfig = TangoConfiguration;
		FString ConfigString(TangoConfig_toString_dynamic(TangoConfiguration));
		UE_LOG(LogTango, Log, TEXT("Tango Config: %s"), *ConfigString);
		if (!bTangoIsRunning)
		{
			UE_LOG(LogTango, Error, TEXT("Starting Tango failed with TangoErrorType of %d"), ConnectError);
			return false;
		}
		if (ConfigurationData.bEnableDepthCamera)
		{
			// set the depth camera frame rate
			if (SetTangoAPIConfigInt32(
				LowLevelTangoConfig,
				TangoConfigKeys::DEPTH_CAMERA_FRAMERATE,
				ConfigurationData.InitialDepthCameraFrameRate
			))
			{
				if (TangoService_setRuntimeConfig_dynamic(LowLevelTangoConfig) != TANGO_SUCCESS)
				{
					UE_LOG(LogTango, Error, TEXT("Can't set initial depth camera frame rate: setRuntimeConfig failed"));
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTango, Error, TEXT("Could not allocate Tango configuration object, cannot start Tango."));
		return false;
	}

	if (!bTangoSupportLibraryIntialized)
	{
		TangoSupport_initialize(
			TangoService_getPoseAtTime_dynamic,
			TangoService_getCameraIntrinsics_dynamic);
		bTangoSupportLibraryIntialized = true;
	}
#endif
	UE_LOG(LogTango, Log, TEXT("Tango Tracking started successfully"));

	LastKnownConfig = ConfigurationData;
	TangoConnectionEventDelegate.Broadcast(true);
	return true;
}

void FTangoDevice::StopTangoTracking()
{
	UE_LOG(LogTango, Log, TEXT("Stop Tango Tracking."));
	FScopeLock ScopeLock(&TangoStateLock);
	if (!bTangoIsRunning)
	{
		UE_LOG(LogTango, Log, TEXT("Could not stop Tango because Tango is not running!"));
		return;
	}

#if PLATFORM_ANDROID
	if (LastKnownConfig.bEnableDepthCamera)
	{
		TangoPointCloudManager.DisconnectPointCloud();
	}

	TangoService_disconnect_dynamic();
	if (LowLevelTangoConfig != nullptr)
	{
		TangoConfig_free_dynamic(LowLevelTangoConfig);
		LowLevelTangoConfig = nullptr;
	}

	TangoMotionManager.DisconnectOnPoseAvailable();

	if (LastKnownConfig.bEnablePassthroughCamera)
	{
		TangoARCameraManager.DisconnectTangoColorCamera();
	}
#endif
	bTangoIsRunning = false;
	TangoConnectionEventDelegate.Broadcast(false);
	TangoMotionManager.UpdateTangoPoses(); // invalidate current pose
}

// Functions that are called on Android lifecycle events.
void FTangoDevice::OnApplicationCreated()
{
}

void FTangoDevice::OnApplicationDestroyed()
{
}

void FTangoDevice::OnApplicationPause()
{
	TangoPointCloudManager.OnPause();
}

void FTangoDevice::OnAreaDescriptionPermissionResult(bool bWasGranted)
{
	UE_LOG(LogTango, Log, TEXT("OnAreaPermissionResult Called: %d"), bWasGranted);
	RunOnGameThread([]() -> void {
		// @TODO: fire event to user so they can take action if denied
	});
}

void FTangoDevice::OnApplicationResume()
{
	TangoPointCloudManager.OnResume();
}

void FTangoDevice::OnApplicationStop()
{
	UE_LOG(LogTango, Log, TEXT("OnStop Called"));
	bShouldTangoRestart = bTangoIsRunning;
	StopTangoTracking();
}

void FTangoDevice::OnApplicationStart()
{
	UE_LOG(LogTango, Log, TEXT("OnStart Called: %d"), bShouldTangoRestart);
	if (bShouldTangoRestart)
	{
		bShouldTangoRestart = false;
		RunOnGameThread([this]() -> void {
			StartTangoTracking(LastKnownConfig);
		});
	}
}

int32 FTangoDevice::GetDepthCameraFrameRate()
{
#if PLATFORM_ANDROID
	return LastKnownConfig.InitialDepthCameraFrameRate;
#else
	return 0;
#endif
}

bool FTangoDevice::SetDepthCameraFrameRate(int32 NewFrameRate)
{
#if PLATFORM_ANDROID
	if (!GetIsTangoRunning() || !LastKnownConfig.bEnableDepthCamera)
	{
		return false;
	}
	if (!SetTangoAPIConfigInt32(
		LowLevelTangoConfig,
		TangoConfigKeys::DEPTH_CAMERA_FRAMERATE,
		NewFrameRate))
	{
		return false;
	}
	if (TangoService_setRuntimeConfig_dynamic(LowLevelTangoConfig) != TANGO_SUCCESS)
	{
		UE_LOG(
			LogTango,
			Error,
			TEXT("TangoService_setRuntimeConfig failed to set depth camera frame rate to %d"),
			NewFrameRate);
		return false;
	}
	// Sync LastKnownConfig
	LastKnownConfig.InitialDepthCameraFrameRate = NewFrameRate;
	return true;
#else
	return false;
#endif
}
