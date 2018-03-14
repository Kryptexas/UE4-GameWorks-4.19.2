// Copyright 2017 Google Inc.

#include "GoogleARCoreXRTrackingSystem.h"
#include "Engine/Engine.h"
#include "RHIDefinitions.h"
#include "GameFramework/PlayerController.h"
#include "GoogleARCoreDevice.h"
#include "GoogleARCoreXRCamera.h"
#include "ARSessionConfig.h"

FGoogleARCoreXRTrackingSystem::FGoogleARCoreXRTrackingSystem()
	: ARCoreDeviceInstance(nullptr)
	, bMatchDeviceCameraFOV(false)
	, bEnablePassthroughCameraRendering(false)
	, bHasValidPose(false)
	, CachedPosition(FVector::ZeroVector)
	, CachedOrientation(FQuat::Identity)
	, DeltaControlRotation(FRotator::ZeroRotator)
	, DeltaControlOrientation(FQuat::Identity)
	, LightEstimate(nullptr)
{
	UE_LOG(LogGoogleARCoreTrackingSystem, Log, TEXT("Creating GoogleARCore Tracking System."));
	ARCoreDeviceInstance = FGoogleARCoreDevice::GetInstance();
	check(ARCoreDeviceInstance);
}

FGoogleARCoreXRTrackingSystem::~FGoogleARCoreXRTrackingSystem()
{
}

/////////////////////////////////////////////////////////////////////////////////
// Begin FGoogleARCoreXRTrackingSystem IHeadMountedDisplay Virtual Interface   //
////////////////////////////////////////////////////////////////////////////////
FName FGoogleARCoreXRTrackingSystem::GetSystemName() const
{
	static FName DefaultName(TEXT("FGoogleARCoreXRTrackingSystem"));
	return DefaultName;
}

bool FGoogleARCoreXRTrackingSystem::IsHeadTrackingAllowed() const
{
#if PLATFORM_ANDROID
	return true;
#else
	return false;
#endif
}

bool FGoogleARCoreXRTrackingSystem::GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition)
{
	if (DeviceId == IXRTrackingSystem::HMDDeviceId)
	{
		OutOrientation = CachedOrientation;
		OutPosition = CachedPosition;
		return true;
	}
	else
	{
		return false;
	}
}

FString FGoogleARCoreXRTrackingSystem::GetVersionString() const
{
	FString s = FString::Printf(TEXT("ARCoreHMD - %s, built %s, %s"), *FEngineVersion::Current().ToString(),
		UTF8_TO_TCHAR(__DATE__), UTF8_TO_TCHAR(__TIME__));

	return s;
}

bool FGoogleARCoreXRTrackingSystem::EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type /*= EXRTrackedDeviceType::Any*/)
{
	if (Type == EXRTrackedDeviceType::Any || Type == EXRTrackedDeviceType::HeadMountedDisplay)
	{
		static const int32 DeviceId = IXRTrackingSystem::HMDDeviceId;
		OutDevices.Add(DeviceId);
		return true;
	}
	return false;
}

bool FGoogleARCoreXRTrackingSystem::OnStartGameFrame(FWorldContext& WorldContext)
{
	FTransform CurrentPose;
	if (ARCoreDeviceInstance->GetIsARCoreSessionRunning())
	{
		if (ARCoreDeviceInstance->GetTrackingState() == EGoogleARCoreTrackingState::Tracking)
		{
			CurrentPose = ARCoreDeviceInstance->GetLatestPose();
			CurrentPose *= GetAlignmentTransform();
			bHasValidPose = true;
			CachedTrackingToWorld = ComputeTrackingToWorldTransform(WorldContext);
		}
		else
		{
			bHasValidPose = false;
		}

		if (bHasValidPose)
		{
			CachedOrientation = CurrentPose.GetRotation();
			CachedPosition = CurrentPose.GetTranslation();
		}

		if (LightEstimate == nullptr)
		{
			LightEstimate = NewObject<UARBasicLightEstimate>();
		}
		FGoogleARCoreLightEstimate ARCoreLightEstimate = FGoogleARCoreDevice::GetInstance()->GetLatestLightEstimate();
		if (ARCoreLightEstimate.bIsValid)
		{
			// Try to convert ARCore average pixel intensity to lumen and set the color tempature to pure white.
			float LightLumen = ARCoreLightEstimate.PixelIntensity / 0.18f * 1000;
			LightEstimate->SetLightEstimate(LightLumen, 6500);
		}
		else
		{
			LightEstimate = nullptr;
		}

	}

	return true;
}

void FGoogleARCoreXRTrackingSystem::ConfigARCoreXRCamera(bool bInMatchDeviceCameraFOV, bool bInEnablePassthroughCameraRendering)
{
	bMatchDeviceCameraFOV = bInMatchDeviceCameraFOV;
	bEnablePassthroughCameraRendering = bInEnablePassthroughCameraRendering;

	static_cast<FGoogleARCoreXRCamera*>(GetXRCamera().Get())->ConfigXRCamera(bEnablePassthroughCameraRendering, bEnablePassthroughCameraRendering);
}


void FGoogleARCoreXRTrackingSystem::EnableColorCameraRendering(bool bInEnablePassthroughCameraRendering)
{
	bEnablePassthroughCameraRendering = bInEnablePassthroughCameraRendering;
	static_cast<FGoogleARCoreXRCamera*>(GetXRCamera().Get())->ConfigXRCamera(bEnablePassthroughCameraRendering, bEnablePassthroughCameraRendering);
}

bool FGoogleARCoreXRTrackingSystem::GetColorCameraRenderingEnabled()
{
	return bEnablePassthroughCameraRendering;
}

float FGoogleARCoreXRTrackingSystem::GetWorldToMetersScale() const
{
	if (IsInGameThread() && GWorld != nullptr)
	{
		return GWorld->GetWorldSettings()->WorldToMeters;
	}

	// Default value, assume Unreal units are in centimeters
	return 100.0f;
}

void FGoogleARCoreXRTrackingSystem::OnARSystemInitialized()
{

}

EARTrackingQuality FGoogleARCoreXRTrackingSystem::OnGetTrackingQuality() const
{
	if (!bHasValidPose)
	{
		return EARTrackingQuality::NotTracking;
	}

	return EARTrackingQuality::OrientationAndPosition;
}

void FGoogleARCoreXRTrackingSystem::OnStartARSession(UARSessionConfig* SessionConfig)
{
	FGoogleARCoreDevice::GetInstance()->StartARCoreSessionRequest(SessionConfig);
}

void FGoogleARCoreXRTrackingSystem::OnPauseARSession()
{
	FGoogleARCoreDevice::GetInstance()->PauseARCoreSession();
}

void FGoogleARCoreXRTrackingSystem::OnStopARSession()
{
	FGoogleARCoreDevice::GetInstance()->PauseARCoreSession();
	FGoogleARCoreDevice::GetInstance()->ResetARCoreSession();
}

FARSessionStatus FGoogleARCoreXRTrackingSystem::OnGetARSessionStatus() const
{
	return FGoogleARCoreDevice::GetInstance()->GetSessionStatus();
}

void FGoogleARCoreXRTrackingSystem::OnSetAlignmentTransform(const FTransform& InAlignmentTransform)
{
	const FTransform& NewAlignmentTransform = InAlignmentTransform;

	TArray<UARTrackedGeometry*> AllTrackedGeometries = GetAllTrackedGeometries();
	for (UARTrackedGeometry* TrackedGeometry : AllTrackedGeometries)
	{
		TrackedGeometry->UpdateAlignmentTransform(NewAlignmentTransform);
	}

	TArray<UARPin*> AllARPins = GetAllPins();
	for (UARPin* SomePin : AllARPins)
	{
		SomePin->UpdateAlignmentTransform(NewAlignmentTransform);
	}

	SetAlignmentTransform_Internal(InAlignmentTransform);
}

TArray<FARTraceResult> FGoogleARCoreXRTrackingSystem::OnLineTraceTrackedObjects(const FVector2D ScreenCoord, EARLineTraceChannels TraceChannels)
{
	EGoogleARCoreLineTraceChannel ARCoreTraceChannels = EGoogleARCoreLineTraceChannel::None;
	if (!!(TraceChannels & EARLineTraceChannels::FeaturePoint))
	{
		ARCoreTraceChannels = ARCoreTraceChannels | EGoogleARCoreLineTraceChannel::FeaturePoint;
	}
	if (!!(TraceChannels & EARLineTraceChannels::GroundPlane))
	{
		ARCoreTraceChannels = ARCoreTraceChannels | EGoogleARCoreLineTraceChannel::InfinitePlane;
	}
	if (!!(TraceChannels & EARLineTraceChannels::PlaneUsingBoundaryPolygon))
	{
		ARCoreTraceChannels = ARCoreTraceChannels | EGoogleARCoreLineTraceChannel::PlaneUsingBoundaryPolygon;
	}
	if (!!(TraceChannels & EARLineTraceChannels::PlaneUsingExtent))
	{
		ARCoreTraceChannels = ARCoreTraceChannels | EGoogleARCoreLineTraceChannel::PlaneUsingExtent;
	}

	TArray<FARTraceResult> OutHitResults;
	FGoogleARCoreDevice::GetInstance()->ARLineTrace(ScreenCoord, ARCoreTraceChannels, OutHitResults);
	return OutHitResults;
}

TArray<UARTrackedGeometry*> FGoogleARCoreXRTrackingSystem::OnGetAllTrackedGeometries() const
{
	TArray<UARTrackedGeometry*> AllTrackedGeometry;
	FGoogleARCoreDevice::GetInstance()->GetAllTrackables<UARTrackedGeometry>(AllTrackedGeometry);
	return AllTrackedGeometry;
}

TArray<UARPin*> FGoogleARCoreXRTrackingSystem::OnGetAllPins() const
{
	TArray<UARPin*> AllARPins;
	FGoogleARCoreDevice::GetInstance()->GetAllARPins(AllARPins);
	return AllARPins;
}

bool FGoogleARCoreXRTrackingSystem::OnIsTrackingTypeSupported(EARSessionType SessionType) const
{
	return FGoogleARCoreDevice::GetInstance()->GetIsTrackingTypeSupported(SessionType);
}

UARLightEstimate* FGoogleARCoreXRTrackingSystem::OnGetCurrentLightEstimate() const
{
	return LightEstimate;
}

UARPin* FGoogleARCoreXRTrackingSystem::OnPinComponent(USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry /*= nullptr*/, const FName DebugName /*= NAME_None*/)
{
	UARPin* NewARPin = nullptr;
	// TODO: error handling?
	FGoogleARCoreDevice::GetInstance()->CreateARPin(PinToWorldTransform, TrackedGeometry, ComponentToPin, DebugName, NewARPin);
	return NewARPin;
}

void FGoogleARCoreXRTrackingSystem::OnRemovePin(UARPin* PinToRemove)
{
	FGoogleARCoreDevice::GetInstance()->RemoveARPin(PinToRemove);
}


void FGoogleARCoreXRTrackingSystem::AddReferencedObjects(FReferenceCollector& Collector)
{
	FARSystemBase::AddReferencedObjects(Collector);

	if (LightEstimate != nullptr)
	{
		Collector.AddReferencedObject(LightEstimate);
	}
}

TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> FGoogleARCoreXRTrackingSystem::GetXRCamera(int32 DeviceId /*= HMDDeviceId*/)
{
	check(DeviceId == HMDDeviceId);

	if (!XRCamera.IsValid())
	{
		XRCamera = FSceneViewExtensions::NewExtension<FGoogleARCoreXRCamera>(*this, DeviceId);
	}
	return XRCamera;
}
