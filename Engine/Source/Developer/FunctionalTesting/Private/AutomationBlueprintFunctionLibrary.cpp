// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "FunctionalTestingPrivatePCH.h"

#include "AutomationCommon.h"
#include "AutomationTest.h"
#include "DelayForFramesLatentAction.h"
#include "TakeScreenshotAfterTimeLatentAction.h"
#include "Engine/LatentActionManager.h"
#include "SlateBasics.h"
#include "HighResScreenshot.h"
#include "Slate/SceneViewport.h"
#include "Tests/AutomationTestSettings.h"

#include "AutomationBlueprintFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(BlueprintAssertion, Error, Error)
DEFINE_LOG_CATEGORY_STATIC(AutomationFunctionLibrary, Log, Log)

static TAutoConsoleVariable<int32> CVarAutomationScreenshotResolutionWidth(
	TEXT("AutomationScreenshotResolutionWidth"),
	0,
	TEXT("The width of automation screenshots."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarAutomationScreenshotResolutionHeight(
	TEXT("AutomationScreenshotResolutionHeight"),
	0,
	TEXT("The height of automation screenshots."),
	ECVF_Default);

#if (WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)

class FAutomationScreenshotTaker
{
	FOnScreenshotCaptured&	ScreenshotCapturedDelegate;
	FDelegateHandle			DelegateHandle;
	FString					Name;

public:
	FAutomationScreenshotTaker(FOnScreenshotCaptured& InScreenshotCapturedDelegate, const FString& InName)
		: ScreenshotCapturedDelegate(InScreenshotCapturedDelegate)
		, DelegateHandle()
		, Name(InName)
	{
		DelegateHandle = ScreenshotCapturedDelegate.AddRaw(this, &FAutomationScreenshotTaker::GrabScreenShot);
	}

	void GrabScreenShot(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData)
	{
		FAutomationScreenshotData Data = AutomationCommon::BuildScreenshotData(GWorld->GetName(), Name, InSizeX, InSizeY);
		//Data.bHasComparisonRules = true;

		FAutomationTestFramework::Get().OnScreenshotCaptured().ExecuteIfBound(InImageData, Data);

		UE_LOG(AutomationFunctionLibrary, Log, TEXT("Screenshot captured as %s"), *Data.Path);

		ScreenshotCapturedDelegate.Remove(DelegateHandle);

		delete this;
	}
};

#endif

UAutomationBlueprintFunctionLibrary::UAutomationBlueprintFunctionLibrary(const class FObjectInitializer& Initializer) : Super(Initializer)
{
}

bool UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotInternal(const FString& Name, FIntPoint Resolution)
{
	// Fallback resolution if all else fails for screenshots.
	uint32 ResolutionX = 1280;
	uint32 ResolutionY = 720;

	// First get the default set for the project.
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	if ( AutomationTestSettings->DefaultScreenshotResolution.GetMin() > 0 )
	{
		ResolutionX = (uint32)AutomationTestSettings->DefaultScreenshotResolution.X;
		ResolutionY = (uint32)AutomationTestSettings->DefaultScreenshotResolution.Y;
	}
	
	// If there's an override resolution, use that instead.
	if ( Resolution.GetMin() > 0 )
	{
		ResolutionX = (uint32)Resolution.X;
		ResolutionY = (uint32)Resolution.Y;
	}
	else
	{
		// Failing to find an override, look for a platform override that may have been provided through the
		// device profiles setup, to configure the CVars for controlling the automation screenshot size.
		int32 OverrideWidth = CVarAutomationScreenshotResolutionWidth.GetValueOnGameThread();
		int32 OverrideHeight = CVarAutomationScreenshotResolutionHeight.GetValueOnGameThread();

		if ( OverrideWidth > 0 )
		{
			ResolutionX = (uint32)OverrideWidth;
		}

		if ( OverrideHeight > 0 )
		{
			ResolutionY = (uint32)OverrideHeight;
		}
	}

	// Force all mip maps to load before taking the screenshot.
	UTexture::ForceUpdateTextureStreaming();

#if (WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)
	FAutomationScreenshotTaker* TempObject = new FAutomationScreenshotTaker(GEngine->GameViewport->OnScreenshotCaptured(), Name);
#endif

	FHighResScreenshotConfig& Config = GetHighResScreenshotConfig();
	if ( Config.SetResolution(ResolutionX, ResolutionY, 1.0f) )
	{
		GEngine->GameViewport->GetGameViewport()->TakeHighResScreenShot();
		return true;
	}

	return false;
}

void UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshot(UObject* WorldContextObject, FLatentActionInfo LatentInfo, const FString& Name, FIntPoint Resolution, float DelayBeforeScreenshotSeconds)
{
	if (GIsAutomationTesting)
	{
		if(!FAutomationTestFramework::Get().IsScreenshotAllowed())
		{
			UE_LOG(AutomationFunctionLibrary, Error, TEXT("Attempted to capture screenshot (%s) but screenshots are not enabled. If you're seeing this on TeamCity, please make sure the name of your test contains '_VisualTest'"), *Name);
			return;
		}

		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject))
		{
			FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
			if (LatentActionManager.FindExistingAction<FTakeScreenshotAfterTimeLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
			{
				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTakeScreenshotAfterTimeLatentAction(LatentInfo, Name, Resolution, DelayBeforeScreenshotSeconds));
			}
		}
	}
	else
	{
		UE_LOG(AutomationFunctionLibrary, Log, TEXT("Screenshot not captured - screenshots are only taken during automation tests"));
	}
}

void UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotAtCamera(UObject* WorldContextObject, FLatentActionInfo LatentInfo, ACameraActor* Camera, const FString& NameOverride, FIntPoint Resolution, float DelayBeforeScreenshotSeconds)
{
	if ( Camera == nullptr )
	{
		UE_LOG(AutomationFunctionLibrary, Error, TEXT("A camera is required to TakeAutomationScreenshotAtCamera"));
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);

	if ( PlayerController == nullptr )
	{
		UE_LOG(AutomationFunctionLibrary, Error, TEXT("A player controller is required to TakeAutomationScreenshotAtCamera"));
		return;
	}

	// Move the player, then queue up a screenshot.
	// We need to delay before the screenshot so that the motion blur has time to stop.
	PlayerController->SetViewTarget(Camera, FViewTargetTransitionParams());
	FString ScreenshotName = Camera->GetName();

	if ( !NameOverride.IsEmpty() )
	{
		ScreenshotName = NameOverride;
	}

	if ( UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject) )
	{
		ScreenshotName = FString::Printf(TEXT("%s_%s"), *World->GetName(), *ScreenshotName);

		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if ( LatentActionManager.FindExistingAction<FTakeScreenshotAfterTimeLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr )
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTakeScreenshotAfterTimeLatentAction(LatentInfo, ScreenshotName, Resolution, DelayBeforeScreenshotSeconds));
		}
	}
}

void UAutomationBlueprintFunctionLibrary::BeginPerformanceCapture()
{
    //::BeginPerformanceCapture();
}

void UAutomationBlueprintFunctionLibrary::EndPerformanceCapture()
{
    //::EndPerformanceCapture();
}
