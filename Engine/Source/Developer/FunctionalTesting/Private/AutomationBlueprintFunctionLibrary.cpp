// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AutomationBlueprintFunctionLibrary.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "EngineGlobals.h"
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Tests/AutomationCommon.h"
#include "Logging/MessageLog.h"
#include "TakeScreenshotAfterTimeLatentAction.h"
#include "HighResScreenshot.h"
#include "Slate/SceneViewport.h"
#include "Tests/AutomationTestSettings.h"
#include "Slate/WidgetRenderer.h"
#include "DelayAction.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "ShaderCompiler.h"
#include "AutomationBlueprintFunctionLibrary.h"
#include "BufferVisualizationData.h"
#include "Engine/LocalPlayer.h"
#include "ContentStreaming.h"

#define LOCTEXT_NAMESPACE "Automation"

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


void FinishLoadingBeforeScreenshot()
{
	// Force all shader compiling to finish.
	GShaderCompilingManager->FinishAllCompilation();

	// Force all mip maps to load before taking the screenshot.
	UTexture::ForceUpdateTextureStreaming();

	IStreamingManager::Get().StreamAllResources(0.0f);
}


#if (WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)

class FConsoleVariableSwapper
{
public:
	FConsoleVariableSwapper(FString InConsoleVariableName)
		: bModified(false)
		, ConsoleVariableName(InConsoleVariableName)
	{
	}

	void Set(int32 Value)
	{
		IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(*ConsoleVariableName);
		if ( ensure(ConsoleVariable) )
		{
			if ( bModified == false )
			{
				bModified = true;
				OriginalValue = ConsoleVariable->GetInt();
			}

			ConsoleVariable->Set(Value);
		}
	}

	void Restore()
	{
		if ( bModified )
		{
			IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(*ConsoleVariableName);
			if ( ensure(ConsoleVariable) )
			{
				ConsoleVariable->Set(OriginalValue);
			}

			bModified = false;
		}
	}

private:
	bool bModified;
	FString ConsoleVariableName;

	int32 OriginalValue;
};

class FAutomationScreenshotTaker
{
public:
	FAutomationScreenshotTaker(UWorld* InWorld, const FString& InName, FAutomationScreenshotOptions InOptions)
		: World(InWorld)
		, Name(InName)
		, Options(InOptions)
		, DefaultFeature_AntiAliasing(TEXT("r.DefaultFeature.AntiAliasing"))
		, DefaultFeature_AutoExposure(TEXT("r.DefaultFeature.AutoExposure"))
		, DefaultFeature_MotionBlur(TEXT("r.DefaultFeature.MotionBlur"))
		, PostProcessAAQuality(TEXT("r.PostProcessAAQuality"))
		, MotionBlurQuality(TEXT("r.MotionBlurQuality"))
		, ScreenSpaceReflectionQuality(TEXT("r.SSR.Quality"))
		, EyeAdaptationQuality(TEXT("r.EyeAdaptationQuality"))
		, ContactShadows(TEXT("r.ContactShadows"))
	{
		GEngine->GameViewport->OnScreenshotCaptured().AddRaw(this, &FAutomationScreenshotTaker::GrabScreenShot);

		check(IsInGameThread());

		if ( Options.bDisableNoisyRenderingFeatures )
		{
			DefaultFeature_AntiAliasing.Set(0);
			DefaultFeature_AutoExposure.Set(0);
			DefaultFeature_MotionBlur.Set(0);
			PostProcessAAQuality.Set(0);
			MotionBlurQuality.Set(0);
			ScreenSpaceReflectionQuality.Set(0);
			EyeAdaptationQuality.Set(0);
			ContactShadows.Set(0);
		}

		Options.SetToleranceAmounts(Options.Tolerance);

		if ( UGameViewportClient* ViewportClient = GEngine->GameViewport )
		{
			static IConsoleVariable* ICVar = IConsoleManager::Get().FindConsoleVariable(FBufferVisualizationData::GetVisualizationTargetConsoleCommandName());
			if ( ICVar )
			{
				if ( ViewportClient->GetEngineShowFlags() )
				{
					ViewportClient->GetEngineShowFlags()->SetVisualizeBuffer(InOptions.VisualizeBuffer == NAME_None ? false : true);
					ViewportClient->GetEngineShowFlags()->SetTonemapper(InOptions.VisualizeBuffer == NAME_None ? true : false);
					ICVar->Set(*InOptions.VisualizeBuffer.ToString());
				}
			}
		}
	}

	virtual ~FAutomationScreenshotTaker()
	{
		check(IsInGameThread());

		DefaultFeature_AntiAliasing.Restore();
		DefaultFeature_AutoExposure.Restore();
		DefaultFeature_MotionBlur.Restore();
		PostProcessAAQuality.Restore();
		MotionBlurQuality.Restore();
		ScreenSpaceReflectionQuality.Restore();
		EyeAdaptationQuality.Restore();
		ContactShadows.Restore();

		if ( UGameViewportClient* ViewportClient = GEngine->GameViewport )
		{
			static IConsoleVariable* ICVar = IConsoleManager::Get().FindConsoleVariable(FBufferVisualizationData::GetVisualizationTargetConsoleCommandName());
			if ( ICVar )
			{
				if ( ViewportClient->GetEngineShowFlags() )
				{
					ViewportClient->GetEngineShowFlags()->SetVisualizeBuffer(false);
					ViewportClient->GetEngineShowFlags()->SetTonemapper(true);
					ICVar->Set(TEXT(""));
				}
			}
		}

		GEngine->GameViewport->OnScreenshotCaptured().RemoveAll(this);

		FAutomationTestFramework::Get().NotifyScreenshotTakenAndCompared();
	}

	void GrabScreenShot(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData)
	{
		check(IsInGameThread());

		FAutomationScreenshotData Data = AutomationCommon::BuildScreenshotData(GWorld->GetName(), Name, InSizeX, InSizeY);

		// Copy the relevant data into the metadata for the screenshot.
		Data.bHasComparisonRules = true;
		Data.ToleranceRed = Options.ToleranceAmount.Red;
		Data.ToleranceGreen = Options.ToleranceAmount.Green;
		Data.ToleranceBlue = Options.ToleranceAmount.Blue;
		Data.ToleranceAlpha = Options.ToleranceAmount.Alpha;
		Data.ToleranceMinBrightness = Options.ToleranceAmount.MinBrightness;
		Data.ToleranceMaxBrightness = Options.ToleranceAmount.MaxBrightness;
		Data.bIgnoreAntiAliasing = Options.bIgnoreAntiAliasing;
		Data.bIgnoreColors = Options.bIgnoreColors;
		Data.MaximumLocalError = Options.MaximumLocalError;
		Data.MaximumGlobalError = Options.MaximumGlobalError;

		FAutomationTestFramework::Get().OnScreenshotCaptured().ExecuteIfBound(InImageData, Data);

		UE_LOG(AutomationFunctionLibrary, Log, TEXT("Screenshot captured as %s"), *Data.Path);

		if ( GIsAutomationTesting )
		{
			FAutomationTestFramework::Get().OnScreenshotCompared.AddRaw(this, &FAutomationScreenshotTaker::OnComparisonComplete);
		}
		else
		{
			delete this;
		}
	}

	void OnComparisonComplete(bool bWasNew, bool bWasSimilar, double MaxLocalDifference, double GlobalDifference, FString ErrorMessage)
	{
		FAutomationTestFramework::Get().OnScreenshotCompared.RemoveAll(this);

		if ( bWasNew )
		{
			UE_LOG(AutomationFunctionLibrary, Warning, TEXT("New Screenshot '%s' was discovered!  Please add a ground truth version of it."), *Name);
		}
		else
		{
			if ( bWasSimilar )
			{
				UE_LOG(AutomationFunctionLibrary, Display, TEXT("Screenshot '%s' was similar!  Global Difference = %f, Max Local Difference = %f"), *Name, GlobalDifference, MaxLocalDifference);
			}
			else
			{
				if ( ErrorMessage.IsEmpty() )
				{
					UE_LOG(AutomationFunctionLibrary, Error, TEXT("Screenshot '%s' test failed, Screnshots were different!  Global Difference = %f, Max Local Difference = %f"), *Name, GlobalDifference, MaxLocalDifference);
				}
				else
				{
					UE_LOG(AutomationFunctionLibrary, Error, TEXT("Screenshot '%s' test failed;  Error = %s"), *Name, *ErrorMessage);
				}
			}
		}

		delete this;
	}

private:

	TWeakObjectPtr<UWorld> World;
	
	FString	Name;
	FAutomationScreenshotOptions Options;

	FConsoleVariableSwapper DefaultFeature_AntiAliasing;
	FConsoleVariableSwapper DefaultFeature_AutoExposure;
	FConsoleVariableSwapper DefaultFeature_MotionBlur;
	FConsoleVariableSwapper PostProcessAAQuality;
	FConsoleVariableSwapper MotionBlurQuality;
	FConsoleVariableSwapper ScreenSpaceReflectionQuality;
	FConsoleVariableSwapper EyeAdaptationQuality;
	FConsoleVariableSwapper ContactShadows;
};

#endif

UAutomationBlueprintFunctionLibrary::UAutomationBlueprintFunctionLibrary(const class FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

bool UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotInternal(UObject* WorldContextObject, const FString& Name, FAutomationScreenshotOptions Options)
{
	FinishLoadingBeforeScreenshot();

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
	if ( Options.Resolution.GetMin() > 0 )
	{
		ResolutionX = (uint32)Options.Resolution.X;
		ResolutionY = (uint32)Options.Resolution.Y;
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

#if (WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)
	FAutomationScreenshotTaker* TempObject = new FAutomationScreenshotTaker(WorldContextObject ? WorldContextObject->GetWorld() : nullptr, Name, Options);
#endif

	//static IConsoleVariable* HighResScreenshotDelay = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HighResScreenshotDelay"));
	//check(HighResScreenshotDelay);
	//HighResScreenshotDelay->Set(10);

    if ( FPlatformProperties::HasFixedResolution() )
    {
	    FScreenshotRequest::RequestScreenshot(false);
	    return true;
    }
	else
	{
	    FHighResScreenshotConfig& Config = GetHighResScreenshotConfig();

	    if ( Config.SetResolution(ResolutionX, ResolutionY, 1.0f) )
	    {
			if ( !GEngine->GameViewport->GetGameViewport()->TakeHighResScreenShot() )
			{
				// If we failed to take the screenshot, we're going to need to cleanup the automation screenshot taker.
#if (WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)
				delete TempObject;
#endif 
				return false;
			}

			return true; //-V773
		}
	}

	return false;
}

void UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshot(UObject* WorldContextObject, FLatentActionInfo LatentInfo, const FString& Name, const FAutomationScreenshotOptions& Options)
{
	if ( GIsAutomationTesting )
	{
		if ( UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject) )
		{
			FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
			if ( LatentActionManager.FindExistingAction<FTakeScreenshotAfterTimeLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr )
			{
				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTakeScreenshotAfterTimeLatentAction(LatentInfo, Name, Options));
			}
		}
	}
	else
	{
		UE_LOG(AutomationFunctionLibrary, Log, TEXT("Screenshot not captured - screenshots are only taken during automation tests"));
	}
}

void UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotAtCamera(UObject* WorldContextObject, FLatentActionInfo LatentInfo, ACameraActor* Camera, const FString& NameOverride, const FAutomationScreenshotOptions& Options)
{
	if ( Camera == nullptr )
	{
		FMessageLog("PIE").Error(LOCTEXT("CameraRequired", "A camera is required to TakeAutomationScreenshotAtCamera"));
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);

	if ( PlayerController == nullptr )
	{
		FMessageLog("PIE").Error(LOCTEXT("PlayerRequired", "A player controller is required to TakeAutomationScreenshotAtCamera"));
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
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTakeScreenshotAfterTimeLatentAction(LatentInfo, ScreenshotName, Options));
		}
	}
}

void UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotOfUI(UObject* WorldContextObject, FLatentActionInfo LatentInfo, const FString& Name, const FAutomationScreenshotOptions& Options)
{
	FinishLoadingBeforeScreenshot();

	if ( UWorld* World = WorldContextObject->GetWorld() )
	{
		if ( UGameViewportClient* GameViewport = WorldContextObject->GetWorld()->GetGameViewport() )
		{
			TSharedPtr<SViewport> Viewport = GameViewport->GetGameViewportWidget();
			if ( Viewport.IsValid() )
			{
				TArray<FColor> OutColorData;
				FIntVector OutSize;
				if ( FSlateApplication::Get().TakeScreenshot(Viewport.ToSharedRef(), OutColorData, OutSize) )
				{
#if (WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)
					FAutomationScreenshotTaker* TempObject = new FAutomationScreenshotTaker(GEngine->GetWorldFromContextObject(WorldContextObject), Name, Options);

					FAutomationScreenshotData Data = AutomationCommon::BuildScreenshotData(GWorld->GetName(), Name, OutSize.X, OutSize.Y);

					// Copy the relevant data into the metadata for the screenshot.
					Data.bHasComparisonRules = true;
					Data.ToleranceRed = Options.ToleranceAmount.Red;
					Data.ToleranceGreen = Options.ToleranceAmount.Green;
					Data.ToleranceBlue = Options.ToleranceAmount.Blue;
					Data.ToleranceAlpha = Options.ToleranceAmount.Alpha;
					Data.ToleranceMinBrightness = Options.ToleranceAmount.MinBrightness;
					Data.ToleranceMaxBrightness = Options.ToleranceAmount.MaxBrightness;
					Data.bIgnoreAntiAliasing = Options.bIgnoreAntiAliasing;
					Data.bIgnoreColors = Options.bIgnoreColors;
					Data.MaximumLocalError = Options.MaximumLocalError;
					Data.MaximumGlobalError = Options.MaximumGlobalError;

					GEngine->GameViewport->OnScreenshotCaptured().Broadcast(OutSize.X, OutSize.Y, OutColorData);
#endif
				} //-V773

				FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
				if ( LatentActionManager.FindExistingAction<FTakeScreenshotAfterTimeLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr )
				{
					LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FWaitForScreenshotComparisonLatentAction(LatentInfo));
				}
			}
		}
	}
}

//void UAutomationBlueprintFunctionLibrary::BeginPerformanceCapture()
//{
//    //::BeginPerformanceCapture();
//}
//
//void UAutomationBlueprintFunctionLibrary::EndPerformanceCapture()
//{
//    //::EndPerformanceCapture();
//}

bool UAutomationBlueprintFunctionLibrary::AreAutomatedTestsRunning()
{
	return GIsAutomationTesting;
}

FAutomationScreenshotOptions UAutomationBlueprintFunctionLibrary::GetDefaultScreenshotOptionsForGameplay(EComparisonTolerance Tolerance)
{
	FAutomationScreenshotOptions Options;
	Options.Tolerance = Tolerance;
	Options.bDisableNoisyRenderingFeatures = true;
	Options.bIgnoreAntiAliasing = true;
	Options.SetToleranceAmounts(Tolerance);

	return Options;
}

FAutomationScreenshotOptions UAutomationBlueprintFunctionLibrary::GetDefaultScreenshotOptionsForRendering(EComparisonTolerance Tolerance)
{
	FAutomationScreenshotOptions Options;
	Options.Tolerance = Tolerance;
	Options.bDisableNoisyRenderingFeatures = true;
	Options.bIgnoreAntiAliasing = true;
	Options.SetToleranceAmounts(Tolerance);

	return Options;
}

#undef LOCTEXT_NAMESPACE
