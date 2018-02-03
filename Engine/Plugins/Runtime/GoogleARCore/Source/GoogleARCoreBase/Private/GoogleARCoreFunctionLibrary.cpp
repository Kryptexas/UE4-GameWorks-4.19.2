// Copyright 2017 Google Inc.

#include "GoogleARCoreFunctionLibrary.h"
#include "UnrealEngine.h"
#include "Engine/Engine.h"
#include "LatentActions.h"
#include "ARBlueprintLibrary.h"

#include "GoogleARCoreAndroidHelper.h"
#include "GoogleARCoreBaseLogCategory.h"
#include "GoogleARCoreDevice.h"
#include "GoogleARCoreXRTrackingSystem.h"

namespace
{
	FGoogleARCoreXRTrackingSystem* GetARCoreHMD()
	{
		if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == FName("FGoogleARCoreXRTrackingSystem")))
		{
			return static_cast<FGoogleARCoreXRTrackingSystem*>(GEngine->XRSystem.Get());
		}

		return nullptr;
	}

	EGoogleARCoreAvailability ToGoogleARCoreAvailability(EGoogleARCoreAvailabilityInternal AvailabilityInternal)
	{
		switch (AvailabilityInternal)
		{
		case EGoogleARCoreAvailabilityInternal::UnkownError:
		case EGoogleARCoreAvailabilityInternal::UnkownTimedOut:
			return EGoogleARCoreAvailability::UnkownError;
		case EGoogleARCoreAvailabilityInternal::UnsupportedDeviceNotCapable:
			return EGoogleARCoreAvailability::UnsupportedDeviceNotCapable;
		case EGoogleARCoreAvailabilityInternal::SupportedNotInstalled:
		case EGoogleARCoreAvailabilityInternal::SupportedApkTooOld:
			return EGoogleARCoreAvailability::SupportedNotInstalled;
		case EGoogleARCoreAvailabilityInternal::SupportedInstalled:
			return EGoogleARCoreAvailability::SupportedInstalled;
		default:
			ensureMsgf(false, TEXT("Unknown conversion from EGoogleARCoreAvailabilityInternal %d to EGoogleARCoreAvailability."), static_cast<int>(AvailabilityInternal));
			return EGoogleARCoreAvailability::UnkownError;
		}
	}

	const float DefaultLineTraceDistance = 100000; // 1000 meter
}

/************************************************************************/
/*  UGoogleARCoreSessionFunctionLibrary | Lifecycle                     */
/************************************************************************/
void UGoogleARCoreSessionFunctionLibrary::CheckARCoreAvailability(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, EGoogleARCoreAvailability& OutAvailability)
{
	ensureMsgf(false, TEXT("UGoogleARCoreSessionFunctionLibrary::CheckARCoreAPKAvailability Not Implemented!"));
}


void UGoogleARCoreSessionFunctionLibrary::InstallARCoreService(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, EGoogleARCoreInstallRequestResult& OutInstallResult)
{
	ensureMsgf(false, TEXT("UGoogleARCoreSessionFunctionLibrary::InstallARCoreService Not Implemented!"));
}

struct FARCoreStartSessionAction : public FPendingLatentAction
{
public:
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FARCoreStartSessionAction(const FLatentActionInfo& InLatentInfo)
		: FPendingLatentAction()
		, ExecutionFunction(InLatentInfo.ExecutionFunction)
		, OutputLink(InLatentInfo.Linkage)
		, CallbackTarget(InLatentInfo.CallbackTarget)
	{}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		bool bSessionStartedFinished = FGoogleARCoreDevice::GetInstance()->GetStartSessionRequestFinished();
		Response.FinishAndTriggerIf(bSessionStartedFinished, ExecutionFunction, OutputLink, CallbackTarget);
	}
#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Starting ARCore tracking session"));
	}
#endif
};

void UGoogleARCoreSessionFunctionLibrary::StartARCoreSession(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, UGoogleARCoreSessionConfig* Configuration)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		if (LatentManager.FindExistingAction<FARCoreStartSessionAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			UARBlueprintLibrary::StartARSession(static_cast<UARSessionConfig*>(Configuration));
			FARCoreStartSessionAction* NewAction = new FARCoreStartSessionAction(LatentInfo);
			LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction);
		}
	}
}

/************************************************************************/
/*  UGoogleARCoreSessionFunctionLibrary | PassthroughCamera             */
/************************************************************************/
bool UGoogleARCoreSessionFunctionLibrary::IsPassthroughCameraRenderingEnabled()
{
	FGoogleARCoreXRTrackingSystem* ARCoreHMD = GetARCoreHMD();
	if (ARCoreHMD)
	{
		return ARCoreHMD->GetColorCameraRenderingEnabled();
	}
	else
	{
		UE_LOG(LogGoogleARCore, Error, TEXT("Failed to find GoogleARCoreXRTrackingSystem: GoogleARCoreXRTrackingSystem is not available."));
	}
	return false;
}

void UGoogleARCoreSessionFunctionLibrary::SetPassthroughCameraRenderingEnabled(bool bEnable)
{
	FGoogleARCoreXRTrackingSystem* ARCoreHMD = GetARCoreHMD();
	if (ARCoreHMD)
	{
		ARCoreHMD->EnableColorCameraRendering(bEnable);
	}
	else
	{
		UE_LOG(LogGoogleARCore, Error, TEXT("Failed to config ARCoreXRCamera: GoogleARCoreXRTrackingSystem is not available."));
	}
}

void UGoogleARCoreSessionFunctionLibrary::GetPassthroughCameraImageUV(const TArray<float>& InUV, TArray<float>& OutUV)
{
	FGoogleARCoreDevice::GetInstance()->GetPassthroughCameraImageUVs(InUV, OutUV);
}

void UGoogleARCoreSessionFunctionLibrary::GetAllPlanes(TArray<UARPlaneGeometry*>& OutPlaneList)
{
	FGoogleARCoreDevice::GetInstance()->GetAllTrackables<UARPlaneGeometry>(OutPlaneList);
}

void UGoogleARCoreSessionFunctionLibrary::GetAllTrackablePoints(TArray<UARTrackedPoint*>& OutTrackablePointList)
{
	FGoogleARCoreDevice::GetInstance()->GetAllTrackables<UARTrackedPoint>(OutTrackablePointList);
}

template< class T > 
void UGoogleARCoreSessionFunctionLibrary::GetAllTrackable(TArray<T*>& OutTrackableList) 
{
	FGoogleARCoreDevice::GetInstance()->GetAllTrackables<T>(OutTrackableList);
}

template void UGoogleARCoreSessionFunctionLibrary::GetAllTrackable<UARTrackedGeometry>(TArray<UARTrackedGeometry*>& OutTrackableList);
template void UGoogleARCoreSessionFunctionLibrary::GetAllTrackable<UARPlaneGeometry>(TArray<UARPlaneGeometry*>& OutTrackableList);
template void UGoogleARCoreSessionFunctionLibrary::GetAllTrackable<UARTrackedPoint>(TArray<UARTrackedPoint*>& OutTrackableList);

/************************************************************************/
/*  UGoogleARCoreFrameFunctionLibrary                                   */
/************************************************************************/
EGoogleARCoreTrackingState UGoogleARCoreFrameFunctionLibrary::GetTrackingState()
{
	return FGoogleARCoreDevice::GetInstance()->GetTrackingState();
}

void UGoogleARCoreFrameFunctionLibrary::GetPose(FTransform& LastePose)
{
	LastePose = FGoogleARCoreDevice::GetInstance()->GetLatestPose();
}

bool UGoogleARCoreFrameFunctionLibrary::ARCoreLineTrace(UObject* WorldContextObject, const FVector2D& ScreenPosition, TSet<EGoogleARCoreLineTraceChannel> TraceChannels, TArray<FARTraceResult>& OutHitResults)
{
	EGoogleARCoreLineTraceChannel TraceChannelValue = EGoogleARCoreLineTraceChannel::None;
	for (EGoogleARCoreLineTraceChannel Channel : TraceChannels)
	{
		TraceChannelValue = TraceChannelValue | Channel;
	}

	FGoogleARCoreDevice::GetInstance()->ARLineTrace(ScreenPosition, TraceChannelValue, OutHitResults);
	return OutHitResults.Num() > 0;
}

void UGoogleARCoreFrameFunctionLibrary::GetUpdatedARPins(TArray<UARPin*>& OutAnchorList)
{
	FGoogleARCoreDevice::GetInstance()->GetUpdatedARPins(OutAnchorList);
}

void UGoogleARCoreFrameFunctionLibrary::GetUpdatedPlanes(TArray<UARPlaneGeometry*>& OutPlaneList)
{
	FGoogleARCoreDevice::GetInstance()->GetUpdatedTrackables<UARPlaneGeometry>(OutPlaneList);
}

void UGoogleARCoreFrameFunctionLibrary::GetUpdatedTrackablePoints(TArray<UARTrackedPoint*>& OutTrackablePointList)
{
	FGoogleARCoreDevice::GetInstance()->GetUpdatedTrackables<UARTrackedPoint>(OutTrackablePointList);
}

void UGoogleARCoreFrameFunctionLibrary::GetLightEstimation(FGoogleARCoreLightEstimate& LightEstimation)
{
	LightEstimation = FGoogleARCoreDevice::GetInstance()->GetLatestLightEstimate();
}

EGoogleARCoreFunctionStatus UGoogleARCoreFrameFunctionLibrary::GetPointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud)
{
	return FGoogleARCoreDevice::GetInstance()->GetLatestPointCloud(OutLatestPointCloud);
}

EGoogleARCoreFunctionStatus UGoogleARCoreFrameFunctionLibrary::AcquirePointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud)
{
	return FGoogleARCoreDevice::GetInstance()->AcquireLatestPointCloud(OutLatestPointCloud);
}

#if PLATFORM_ANDROID
EGoogleARCoreFunctionStatus UGoogleARCoreFrameFunctionLibrary::GetCameraMetadata(const ACameraMetadata*& OutCameraMetadata)
{
	return FGoogleARCoreDevice::GetInstance()->GetLatestCameraMetadata(OutCameraMetadata);
}
#endif

template< class T >
void UGoogleARCoreFrameFunctionLibrary::GetUpdatedTrackable(TArray<T*>& OutTrackableList)
{
	FGoogleARCoreDevice::GetInstance()->GetUpdatedTrackables<T>(OutTrackableList);
}

template void UGoogleARCoreFrameFunctionLibrary::GetUpdatedTrackable<UARTrackedGeometry>(TArray<UARTrackedGeometry*>& OutTrackableList);
template void UGoogleARCoreFrameFunctionLibrary::GetUpdatedTrackable<UARPlaneGeometry>(TArray<UARPlaneGeometry*>& OutTrackableList);
template void UGoogleARCoreFrameFunctionLibrary::GetUpdatedTrackable<UARTrackedPoint>(TArray<UARTrackedPoint*>& OutTrackableList);