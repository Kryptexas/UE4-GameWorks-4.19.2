// Copyright 2017 Google Inc.

#include "GoogleARCoreDevice.h"
#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"
#include "HAL/ThreadSafeCounter64.h"
#include "GameFramework/WorldSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h" // for FWorldDelegates
#include "Engine/Engine.h"
#include "GeneralProjectSettings.h"
#include "GoogleARCoreXRTrackingSystem.h"
#include "GoogleARCoreAndroidHelper.h"
#include "GoogleARCoreBaseLogCategory.h"

#if PLATFORM_ANDROID
#include "AndroidApplication.h"
#endif

#include "GoogleARCorePermissionHandler.h"

namespace 
{
	EGoogleARCoreSupportStatus ToARCoreSupportedStatus(EGoogleARCoreAPIStatus Status)
	{
		switch (Status)
		{
		case EGoogleARCoreAPIStatus::AR_SUCCESS:
			return EGoogleARCoreSupportStatus::Supported;
		case EGoogleARCoreAPIStatus::AR_UNAVAILABLE_ARCORE_NOT_INSTALLED:
			return EGoogleARCoreSupportStatus::ARCoreAPKNotInstalled;
		case EGoogleARCoreAPIStatus::AR_UNAVAILABLE_DEVICE_NOT_COMPATIBLE:
			return EGoogleARCoreSupportStatus::DeviceNotCompatiable;
		case EGoogleARCoreAPIStatus::AR_UNAVAILABLE_ANDROID_VERSION_NOT_SUPPORTED:
			return EGoogleARCoreSupportStatus::AndroidVersionNotSupported;
		case EGoogleARCoreAPIStatus::AR_UNAVAILABLE_APK_TOO_OLD:
			return EGoogleARCoreSupportStatus::ARCoreAPKTooOld;
		case EGoogleARCoreAPIStatus::AR_UNAVAILABLE_SDK_TOO_OLD:
			return EGoogleARCoreSupportStatus::ARCoreSDKTooOld;
		default:
			UE_LOG(LogGoogleARCoreAPI, Error, TEXT("Unknown conversion from EGoogleARCoreAPIStatus %d to EGoogleARCoreSupportStatus."), static_cast<int>(Status));
			return EGoogleARCoreSupportStatus::Unknown;
		}
	}

	EGoogleARCoreFunctionStatus ToARCoreFunctionStatus(EGoogleARCoreAPIStatus Status)
	{
		switch (Status)
		{
		case EGoogleARCoreAPIStatus::AR_SUCCESS:
			return EGoogleARCoreFunctionStatus::Success;
		case EGoogleARCoreAPIStatus::AR_ERROR_NOT_TRACKING:
			return EGoogleARCoreFunctionStatus::NotTracking;
		case EGoogleARCoreAPIStatus::AR_ERROR_SESSION_PAUSED:
			return EGoogleARCoreFunctionStatus::SessionPaused;
		case EGoogleARCoreAPIStatus::AR_ERROR_RESOURCE_EXHAUSTED:
			return EGoogleARCoreFunctionStatus::ResourceExhausted;
		case EGoogleARCoreAPIStatus::AR_ERROR_NOT_YET_AVAILABLE:
			return EGoogleARCoreFunctionStatus::NotAvailable;
		default:
			UE_LOG(LogGoogleARCoreAPI, Error, TEXT("Unknown conversion from EGoogleARCoreAPIStatus %d to EGoogleARCoreFunctionStatus."), static_cast<int>(Status));
			return EGoogleARCoreFunctionStatus::Unknown;
		}
	}
}

FGoogleARCoreDevice* FGoogleARCoreDevice::GetInstance()
{
	static FGoogleARCoreDevice Inst;
	return &Inst;
}

FGoogleARCoreDevice::FGoogleARCoreDevice()
	: PassthroughCameraTexture(nullptr)
	, PassthroughCameraTextureId(-1)
	, bIsARCoreSessionRunning(false)
	, bForceLateUpdateEnabled(false)
	, bSessionConfigChanged(false)
	, bAndroidRuntimePermissionsRequested(false)
	, bAndroidRuntimePermissionsGranted(false)
	, bPermissionDeniedByUser(false)
	, bStartSessionRequested(false)
	, bShouldSessionRestart(false)
	, bStartSessionQueued(false)
	, WorldToMeterScale(100.0f)
	, PermissionHandler(nullptr)
	, bDisplayOrientationChanged(false)
	, SessionSupportStatus(EGoogleARCoreSupportStatus::Unknown)
	, CurrentSessionStatus(EARSessionStatus::NotStarted)
{
}

void FGoogleARCoreDevice::OnModuleLoaded()
{
	// Init display orientation.
	OnDisplayOrientationChanged();

	ARCoreSession = FGoogleARCoreSession::CreateARCoreSession();

	if (ARCoreSession->GetSessionCreateStatus() == EGoogleARCoreAPIStatus::AR_SUCCESS)
	{
		FWorldDelegates::OnWorldTickStart.AddRaw(this, &FGoogleARCoreDevice::OnWorldTickStart);
	}
}

void FGoogleARCoreDevice::OnModuleUnloaded()
{
	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
	// clear the unique ptr.
	ARCoreSession.Reset();
}

EGoogleARCoreSupportStatus FGoogleARCoreDevice::GetSupportStatus()
{
	return ToARCoreSupportedStatus(ARCoreSession->GetSessionCreateStatus());
}

bool FGoogleARCoreDevice::GetIsARCoreSessionRunning()
{
	return bIsARCoreSessionRunning;
}

EARSessionStatus FGoogleARCoreDevice::GetSessionStatus()
{
	return CurrentSessionStatus;
}

UARSessionConfig* FGoogleARCoreDevice::GetCurrentSessionConfig()
{
	if (GetIsARCoreSessionRunning() && ARSystem.IsValid())
	{
		// Return the last known config if the session is running.
		return &ARSystem->AccessSessionConfig();
	}
	else
	{
		return nullptr;
	}
}

float FGoogleARCoreDevice::GetWorldToMetersScale()
{
	return WorldToMeterScale;
}

// This function will be called by public function to start AR core session request.
void FGoogleARCoreDevice::StartARCoreSessionRequest(UARSessionConfig* SessionConfig)
{
	UE_LOG(LogGoogleARCore, Log, TEXT("Start ARCore session requested"));
	if (ARCoreSession->GetSessionCreateStatus() != EGoogleARCoreAPIStatus::AR_SUCCESS)
	{
		CurrentSessionStatus = EARSessionStatus::NotSupported;
		bStartSessionRequested = false;
		return;
	}

	if (bIsARCoreSessionRunning)
	{
		if (SessionConfig == AccessSessionConfig())
		{
			UE_LOG(LogGoogleARCore, Warning, TEXT("ARCore session is already running with the requested ARCore config. Request aborted."));
			bStartSessionRequested = false;
			return;
		}

		StopARCoreSession();
	}

	if (bStartSessionRequested)
	{
		UE_LOG(LogGoogleARCore, Warning, TEXT("ARCore session is already starting. This will overriding the previous session config with the new one."))
	}

	bStartSessionRequested = true;
	// Re-request permission if necessary
	bPermissionDeniedByUser = false;

	// Try recreating the ARCoreSession to fix the fatal error.
	if (CurrentSessionStatus == EARSessionStatus::FatalError)
	{
		UE_LOG(LogGoogleARCore, Warning, TEXT("Recreating ARCore session due to fatal error detected."));
		ARCoreSession = FGoogleARCoreSession::CreateARCoreSession();
		CurrentSessionStatus = EARSessionStatus::NotStarted;
	}
}

bool FGoogleARCoreDevice::GetStartSessionRequestFinished()
{
	return !bStartSessionRequested;
}

// Note that this function will only be registered when ARCore is supported.
void FGoogleARCoreDevice::OnWorldTickStart(ELevelTick TickType, float DeltaTime)
{
	// Allocate passthrough camera texture if necessary.
	if (PassthroughCameraTexture == nullptr)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			UpdateCameraImageUV,
			FGoogleARCoreDevice*, ARCoreDevicePtr, this,
			{
				ARCoreDevicePtr->AllocatePassthroughCameraTexture_RenderThread();
			}
		);
		FlushRenderingCommands();
	}

	WorldToMeterScale = GWorld->GetWorldSettings()->WorldToMeters;
	TFunction<void()> Func;
	while (RunOnGameThreadQueue.Dequeue(Func))
	{
		Func();
	}

	if (!bIsARCoreSessionRunning && bStartSessionRequested)
	{
		if (bPermissionDeniedByUser)
		{
			CurrentSessionStatus = EARSessionStatus::PermissionNotGranted;
			bStartSessionRequested = false;
		}
		else
		{
			CheckAndRequrestPermission(*AccessSessionConfig());
			// Either we don't need to request permission or the permission request is done.
			// Queue the session start task on UiThread
			if (!bAndroidRuntimePermissionsRequested && !bStartSessionQueued)
			{
				FGoogleARCoreAndroidHelper::QueueStartSessionOnUiThread();
				bStartSessionQueued = true;
			}
		}
	}

	if (bIsARCoreSessionRunning)
	{
		// Update ARFrame
		FVector2D ViewportSize(1, 1);
		if (GEngine && GEngine->GameViewport)
		{
			ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();
		}
		ARCoreSession->SetDisplayGeometry(FGoogleARCoreAndroidHelper::GetDisplayRotation(), ViewportSize.X, ViewportSize.Y);

		EGoogleARCoreAPIStatus Status = ARCoreSession->Update(WorldToMeterScale);
		if (Status == EGoogleARCoreAPIStatus::AR_ERROR_FATAL)
		{
			ARCoreSession->Pause();
			bIsARCoreSessionRunning = false;
			CurrentSessionStatus = EARSessionStatus::FatalError;
		}
		else
		{
			CameraBlitter.DoBlit(PassthroughCameraTextureId, FIntPoint(1080, 1920));
		}
	}
}

void FGoogleARCoreDevice::CheckAndRequrestPermission(const UARSessionConfig& ConfigurationData)
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
				if (!UARCoreAndroidPermissionHandler::CheckRuntimePermission(RuntimePermissions[i]))
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
				PermissionHandler = NewObject<UARCoreAndroidPermissionHandler>();
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

void FGoogleARCoreDevice::HandleRuntimePermissionsGranted(const TArray<FString>& RuntimePermissions, const TArray<bool>& Granted)
{
	bool bGranted = true;
	for (int32 i = 0; i < RuntimePermissions.Num(); i++)
	{
		if (!Granted[i])
		{
			bGranted = false;
			UE_LOG(LogGoogleARCore, Warning, TEXT("Android runtime permission denied: %s"), *RuntimePermissions[i]);
		}
		else
		{
			UE_LOG(LogGoogleARCore, Log, TEXT("Android runtime permission granted: %s"), *RuntimePermissions[i]);
		}
	}
	bAndroidRuntimePermissionsRequested = false;
	bAndroidRuntimePermissionsGranted = bGranted;

	if (!bGranted)
	{
		bPermissionDeniedByUser = true;
	}
}

void FGoogleARCoreDevice::StartSessionWithRequestedConfig()
{
	StartSession(NewObject<UARSessionConfig>());
	bStartSessionRequested = false;
	bStartSessionQueued = false;
}

void FGoogleARCoreDevice::StartSession(UARSessionConfig* ConfigurationData)
{

	check(ConfigurationData != nullptr);


	EGoogleARCoreAPIStatus Status = ARCoreSession->ConfigSession(*ConfigurationData);

	if (Status != EGoogleARCoreAPIStatus::AR_SUCCESS)
	{
		UE_LOG(LogGoogleARCore, Error, TEXT("ARCore Session start failed with error status %d"), static_cast<int>(Status));
		CurrentSessionStatus = EARSessionStatus::UnsupportedConfiguration;
		return;
	}


	check(PassthroughCameraTextureId != -1);
	ARCoreSession->SetCameraTextureId(PassthroughCameraTextureId);

	Status = ARCoreSession->Resume();

	if (Status != EGoogleARCoreAPIStatus::AR_SUCCESS)
	{
		UE_LOG(LogGoogleARCore, Error, TEXT("ARCore Session start failed with error status %d"), static_cast<int>(Status));
		check(Status == EGoogleARCoreAPIStatus::AR_ERROR_FATAL);
		// If we failed here, the only reason would be fatal error.
		CurrentSessionStatus = EARSessionStatus::FatalError;
		return;
	}

	
	if (GEngine->XRSystem.IsValid())
	{
		FGoogleARCoreXRTrackingSystem* ARCoreTrackingSystem = static_cast<FGoogleARCoreXRTrackingSystem*>(GEngine->XRSystem.Get());
		if (ARCoreTrackingSystem)
		{
			const bool bMatchFOV = ConfigurationData->ShouldRenderCameraOverlay();
			ARCoreTrackingSystem->ConfigARCoreXRCamera(bMatchFOV, ConfigurationData->ShouldRenderCameraOverlay());
		}
		else
		{
			UE_LOG(LogGoogleARCore, Error, TEXT("ERROR: GoogleARCoreXRTrackingSystem is not available."));
		}
	}


	bIsARCoreSessionRunning = true;
	CurrentSessionStatus = EARSessionStatus::Running;
	UE_LOG(LogGoogleARCore, Log, TEXT("ARCore session started successfully."));
}

void FGoogleARCoreDevice::SetARSystem(TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> InARSystem)
{
	check(InARSystem.IsValid());
	ARSystem = InARSystem;
	ARCoreSession->SetARSystem(ARSystem.ToSharedRef());
}

TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> FGoogleARCoreDevice::GetARSystem()
{
	return ARSystem;
}

void FGoogleARCoreDevice::StopARCoreSession()
{
	UE_LOG(LogGoogleARCore, Log, TEXT("Stopping ARCore session."));
	if (!bIsARCoreSessionRunning)
	{
		UE_LOG(LogGoogleARCore, Log, TEXT("Could not stop ARCore tracking session because there is no running tracking session!"));
		return;
	}

	EGoogleARCoreAPIStatus Status = ARCoreSession->Pause();

	if (Status == EGoogleARCoreAPIStatus::AR_ERROR_FATAL)
	{
		CurrentSessionStatus = EARSessionStatus::FatalError;
	}

	bIsARCoreSessionRunning = false;
	CurrentSessionStatus = EARSessionStatus::NotStarted;
	UE_LOG(LogGoogleARCore, Log, TEXT("ARCore session stopped"));
}

void FGoogleARCoreDevice::AllocatePassthroughCameraTexture_RenderThread()
{
	FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	FRHIResourceCreateInfo CreateInfo;

	PassthroughCameraTexture = RHICmdList.CreateTextureExternal2D(1, 1, PF_R8G8B8A8, 1, 1, 0, CreateInfo);

	void* NativeResource = PassthroughCameraTexture->GetNativeResource();
	check(NativeResource);
	PassthroughCameraTextureId = *reinterpret_cast<uint32*>(NativeResource);
}

FTextureRHIRef FGoogleARCoreDevice::GetPassthroughCameraTexture()
{
	return PassthroughCameraTexture;
}

FMatrix FGoogleARCoreDevice::GetPassthroughCameraProjectionMatrix(FIntPoint ViewRectSize) const
{
	return ARCoreSession->GetLatestFrame()->GetProjectionMatrix();
}

void FGoogleARCoreDevice::GetPassthroughCameraImageUVs(const TArray<float>& InUvs, TArray<float>& OutUVs) const
{
	ARCoreSession->GetLatestFrame()->TransformDisplayUvCoords(InUvs, OutUVs);
}

EGoogleARCoreTrackingState FGoogleARCoreDevice::GetTrackingState() const
{
	return ARCoreSession->GetLatestFrame()->GetCameraTrackingState();
}

FTransform FGoogleARCoreDevice::GetLatestPose() const
{
	return ARCoreSession->GetLatestFrame()->GetCameraPose();
}

EGoogleARCoreFunctionStatus FGoogleARCoreDevice::GetLatestPointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud) const
{
	if (!bIsARCoreSessionRunning)
	{
		return EGoogleARCoreFunctionStatus::SessionPaused;
	}

	return ToARCoreFunctionStatus(ARCoreSession->GetLatestFrame()->GetPointCloud(OutLatestPointCloud));
}

EGoogleARCoreFunctionStatus FGoogleARCoreDevice::AcquireLatestPointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud) const
{
	if (!bIsARCoreSessionRunning)
	{
		return EGoogleARCoreFunctionStatus::SessionPaused;
	}

	return ToARCoreFunctionStatus(ARCoreSession->GetLatestFrame()->AcquirePointCloud(OutLatestPointCloud));
}

#if PLATFORM_ANDROID
EGoogleARCoreFunctionStatus FGoogleARCoreDevice::GetLatestCameraMetadata(const ACameraMetadata*& OutCameraMetadata) const
{
	if (!bIsARCoreSessionRunning)
	{
		return EGoogleARCoreFunctionStatus::SessionPaused;
	}

	return ToARCoreFunctionStatus(ARCoreSession->GetLatestFrame()->GetCameraMetadata(OutCameraMetadata));
}
#endif
FGoogleARCoreLightEstimate FGoogleARCoreDevice::GetLatestLightEstimate() const
{
	if (!bIsARCoreSessionRunning)
	{
		return FGoogleARCoreLightEstimate();
	}

	return ARCoreSession->GetLatestFrame()->GetLightEstimate();
}

void FGoogleARCoreDevice::ARLineTrace(const FVector2D& ScreenPosition, EARLineTraceChannels TraceChannels, TArray<FARTraceResult>& OutHitResults)
{
	OutHitResults.Empty();
	ARCoreSession->GetLatestFrame()->ARLineTrace(ScreenPosition, TraceChannels, OutHitResults);
}

EGoogleARCoreFunctionStatus FGoogleARCoreDevice::CreateARPin(const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry, USceneComponent* ComponentToPin, const FName DebugName, UARPin*& OutARAnchorObject)
{
	if (!bIsARCoreSessionRunning)
	{
		return EGoogleARCoreFunctionStatus::SessionPaused;
	}

	FTransform PinTransformInTrackingSpace = PinToWorldTransform * ARSystem->GetTrackingToWorldTransform().Inverse();

	return ToARCoreFunctionStatus(ARCoreSession->CreateARAnchor(PinTransformInTrackingSpace, TrackedGeometry, ComponentToPin, DebugName, OutARAnchorObject));
}

void FGoogleARCoreDevice::RemoveARPin(UARPin* ARAnchorObject)
{
	ARCoreSession->DetachAnchor(ARAnchorObject);
}

void FGoogleARCoreDevice::GetAllARPins(TArray<UARPin*>& ARCoreAnchorList)
{
	ARCoreSession->GetAllAnchors(ARCoreAnchorList);
}

void FGoogleARCoreDevice::GetUpdatedARPins(TArray<UARPin*>& ARCoreAnchorList)
{
	ARCoreSession->GetLatestFrame()->GetUpdatedAnchors(ARCoreAnchorList);
}

// Functions that are called on Android lifecycle events.
void FGoogleARCoreDevice::OnApplicationCreated()
{
}

void FGoogleARCoreDevice::OnApplicationDestroyed()
{
}

void FGoogleARCoreDevice::OnApplicationPause()
{
	UE_LOG(LogGoogleARCore, Log, TEXT("OnPause Called: %d"), bIsARCoreSessionRunning);
	bShouldSessionRestart = bIsARCoreSessionRunning;
	if (bIsARCoreSessionRunning)
	{
		StopARCoreSession();
	}
}

void FGoogleARCoreDevice::OnApplicationResume()
{
	UE_LOG(LogGoogleARCore, Log, TEXT("OnResume Called: %d"), bShouldSessionRestart);
	// Try to ask for permission if it is denied by user.
	if (bShouldSessionRestart)
	{
		bShouldSessionRestart = false;
		StartSession(AccessSessionConfig());
	}
}

void FGoogleARCoreDevice::OnApplicationStop()
{
}

void FGoogleARCoreDevice::OnApplicationStart()
{
}

// TODO: we probably don't need this.
void FGoogleARCoreDevice::OnDisplayOrientationChanged()
{
	FGoogleARCoreAndroidHelper::UpdateDisplayRotation();
	bDisplayOrientationChanged = true;
}

UARSessionConfig* FGoogleARCoreDevice::AccessSessionConfig() const
{
	return (ARSystem.IsValid())
		? &ARSystem->AccessSessionConfig()
		: nullptr;
}