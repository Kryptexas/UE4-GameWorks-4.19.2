// Copyright 2017 Google Inc.

#include "GoogleARCoreFunctionLibrary.h"
#include "UnrealEngine.h"
#include "Engine/Engine.h"
#include "LatentActions.h"

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

	const float DefaultLineTraceDistance = 100000; // 1000 meter
}

/************************************************************************/
/*  UGoogleARCoreSessionFunctionLibrary | Lifecycle                     */
/************************************************************************/
EGoogleARCoreSupportStatus UGoogleARCoreSessionFunctionLibrary::GetARCoreSupportStatus()
{
	return FGoogleARCoreDevice::GetInstance()->GetSupportStatus();
}

UARSessionConfig* UGoogleARCoreSessionFunctionLibrary::GetCurrentSessionConfig()
{
	return FGoogleARCoreDevice::GetInstance()->GetCurrentSessionConfig();
}

void UGoogleARCoreSessionFunctionLibrary::GetSessionRequiredRuntimPermissionsWithConfig(
	UARSessionConfig* Config,
	TArray<FString>& RuntimePermissions)
{
	FGoogleARCoreDevice::GetInstance()->GetRequiredRuntimePermissionsForConfiguration(*Config, RuntimePermissions);
}

EARSessionStatus UGoogleARCoreSessionFunctionLibrary::GetARCoreSessionStatus()
{
	return FGoogleARCoreDevice::GetInstance()->GetSessionStatus();
}

EGoogleARCoreTrackingState UGoogleARCoreFrameFunctionLibrary::GetTrackingState()
{
	return FGoogleARCoreDevice::GetInstance()->GetTrackingState();
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

void UGoogleARCoreSessionFunctionLibrary::StartSession(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo)
{
	StartSessionWithConfig(WorldContextObject, LatentInfo, NewObject<UARSessionConfig>());
}

void UGoogleARCoreSessionFunctionLibrary::StartSessionWithConfig(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, UARSessionConfig* Configuration)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		if (LatentManager.FindExistingAction<FARCoreStartSessionAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			FGoogleARCoreDevice::GetInstance()->StartARCoreSessionRequest(Configuration);
			FARCoreStartSessionAction* NewAction = new FARCoreStartSessionAction(LatentInfo);
			LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction);
		}
	}
}

void UGoogleARCoreSessionFunctionLibrary::StopSession()
{
	FGoogleARCoreDevice::GetInstance()->StopARCoreSession();
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

/************************************************************************/
/*  UGoogleARCoreSessionFunctionLibrary | ARAnchor                      */
/************************************************************************/
EGoogleARCoreFunctionStatus UGoogleARCoreSessionFunctionLibrary::SpawnARAnchorActor(UObject* WorldContextObject, UClass* ARAnchorActorClass, const FTransform& ARAnchorWorldTransform, AGoogleARCoreAnchorActor*& OutARAnchorActor)
{
	UARPin* NewARAnchorObject = nullptr;

	if (!ARAnchorActorClass->IsChildOf(AGoogleARCoreAnchorActor::StaticClass()))
	{
		UE_LOG(LogGoogleARCore, Error, TEXT("Failed to spawn GoogleARAnchorActor. The requested ARAnchorActorClass is not a child of AGoogleARCoreAnchorActor."));
		return EGoogleARCoreFunctionStatus::InvalidType;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World == nullptr)
	{
		return EGoogleARCoreFunctionStatus::Unknown;
	}

	OutARAnchorActor = World->SpawnActor<AGoogleARCoreAnchorActor>(ARAnchorActorClass, FTransform::Identity);

	EGoogleARCoreFunctionStatus Status = FGoogleARCoreDevice::GetInstance()->CreateARPin(ARAnchorWorldTransform, nullptr, OutARAnchorActor->GetRootComponent(), FName(TEXT("ARCore_Pin")), NewARAnchorObject);
	if (Status != EGoogleARCoreFunctionStatus::Success)
	{
		OutARAnchorActor->Destroy();
		OutARAnchorActor = nullptr;
		return Status;
	}
	OutARAnchorActor->SetARAnchor(NewARAnchorObject);
	
	return EGoogleARCoreFunctionStatus::Success;
}

EGoogleARCoreFunctionStatus UGoogleARCoreSessionFunctionLibrary::SpawnARAnchorActorFromHitTest(UObject* WorldContextObject, UClass* ARAnchorActorClass, FARTraceResult HitTestResult, AGoogleARCoreAnchorActor*& OutARAnchorActor)
{
	UARPin* NewARAnchorObject = nullptr;

	if (!ARAnchorActorClass->IsChildOf(AGoogleARCoreAnchorActor::StaticClass()))
	{
		UE_LOG(LogGoogleARCore, Error, TEXT("Failed to spawn GoogleARAnchorActor. The requested ARAnchorActorClass is not a child of AGoogleARCoreAnchorActor."));
		return EGoogleARCoreFunctionStatus::InvalidType;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World == nullptr)
	{
		return EGoogleARCoreFunctionStatus::Unknown;
	}

	EGoogleARCoreFunctionStatus Status;
	Status = UGoogleARCoreSessionFunctionLibrary::CreateARAnchorObjectFromHitTestResult(HitTestResult, NewARAnchorObject);
	if (Status != EGoogleARCoreFunctionStatus::Success)
	{
		return Status;
	}

	OutARAnchorActor = World->SpawnActor<AGoogleARCoreAnchorActor>(ARAnchorActorClass, NewARAnchorObject->GetLocalToWorldTransform());
	if (OutARAnchorActor)
	{
		OutARAnchorActor->SetARAnchor(NewARAnchorObject);
	}

	return EGoogleARCoreFunctionStatus::Success;
}

EGoogleARCoreFunctionStatus UGoogleARCoreSessionFunctionLibrary::CreateARAnchorObject(const FTransform& ARAnchorWorldTransform, UARPin*& OutARAnchorObject)
{
	return FGoogleARCoreDevice::GetInstance()->CreateARPin(ARAnchorWorldTransform, nullptr, nullptr, FName(TEXT("ARCore_Pin")),  OutARAnchorObject);
}

EGoogleARCoreFunctionStatus UGoogleARCoreSessionFunctionLibrary::CreateARAnchorObjectFromHitTestResult(FARTraceResult HitTestResult, UARPin*& OutARAnchorObject)
{
	return FGoogleARCoreDevice::GetInstance()->CreateARPin(HitTestResult.GetLocalToWorldTransform(), HitTestResult.GetTrackedGeometry(), nullptr, FName(TEXT("ARCore_Pin_HitTest")), OutARAnchorObject);
}

void UGoogleARCoreSessionFunctionLibrary::RemoveGoogleARAnchorObject(UARPin* ARAnchorObject)
{
	return FGoogleARCoreDevice::GetInstance()->RemoveARPin(static_cast<UARPin*> (ARAnchorObject));
}

void UGoogleARCoreSessionFunctionLibrary::GetAllPlanes(TArray<UARPlaneGeometry*>& OutPlaneList)
{
	FGoogleARCoreDevice::GetInstance()->GetAllTrackables<UARPlaneGeometry>(OutPlaneList);
}

void UGoogleARCoreSessionFunctionLibrary::GetAllTrackablePoints(TArray<UARTrackedPoint*>& OutTrackablePointList)
{
	FGoogleARCoreDevice::GetInstance()->GetAllTrackables<UARTrackedPoint>(OutTrackablePointList);
}

void UGoogleARCoreSessionFunctionLibrary::GetAllAnchors(TArray<UARPin*>& OutAnchorList)
{
	FGoogleARCoreDevice::GetInstance()->GetAllARPins(OutAnchorList);
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
void UGoogleARCoreFrameFunctionLibrary::GetLatestPose(FTransform& LastePose)
{
	LastePose = FGoogleARCoreDevice::GetInstance()->GetLatestPose();
}

bool UGoogleARCoreFrameFunctionLibrary::ARLineTrace(UObject* WorldContextObject, const FVector2D& ScreenPosition, EARLineTraceChannels TraceChannels, TArray<FARTraceResult>& OutHitResults)
{
	FGoogleARCoreDevice::GetInstance()->ARLineTrace(ScreenPosition, TraceChannels, OutHitResults);
	return OutHitResults.Num() > 0;
}

void UGoogleARCoreFrameFunctionLibrary::GetUpdatedAnchors(TArray<UARPin*>& OutAnchorList)
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

void UGoogleARCoreFrameFunctionLibrary::GetLatestLightEstimation(FGoogleARCoreLightEstimate& LightEstimation)
{
	LightEstimation = FGoogleARCoreDevice::GetInstance()->GetLatestLightEstimate();
}

EGoogleARCoreFunctionStatus UGoogleARCoreFrameFunctionLibrary::GetLatestPointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud)
{
	return FGoogleARCoreDevice::GetInstance()->GetLatestPointCloud(OutLatestPointCloud);
}

EGoogleARCoreFunctionStatus UGoogleARCoreFrameFunctionLibrary::AcquireLatestPointCloud(UGoogleARCorePointCloud*& OutLatestPointCloud)
{
	return FGoogleARCoreDevice::GetInstance()->AcquireLatestPointCloud(OutLatestPointCloud);
}

#if PLATFORM_ANDROID
EGoogleARCoreFunctionStatus UGoogleARCoreFrameFunctionLibrary::GetLatestCameraMetadata(const ACameraMetadata*& OutCameraMetadata)
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