// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ScreenshotFunctionalTestBase.h"

#include "Engine/GameViewportClient.h"
#include "AutomationBlueprintFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif
#include "EngineGlobals.h"
#include "Misc/AutomationTest.h"
#include "Slate/SceneViewport.h"
#include "RenderingThread.h"
#include "Tests/AutomationCommon.h"
#include "Logging/LogMacros.h"
#include "UObject/AutomationObjectVersion.h"

#define	WITH_AUTOMATION_TESTS			(WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)
#define	WITH_EDITOR_AUTOMATION_TESTS	(WITH_EDITOR && WITH_AUTOMATION_TESTS)

DEFINE_LOG_CATEGORY_STATIC(LogScreenshotFunctionalTest, Log, Log)

AScreenshotFunctionalTestBase::AScreenshotFunctionalTestBase(const FObjectInitializer& ObjectInitializer)
	: AFunctionalTest(ObjectInitializer)
	, ScreenshotOptions(EComparisonTolerance::Low)
	, bNeedsViewSettingsRestore(false)
	, bNeedsViewportRestore(false)
{
	ScreenshotCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	ScreenshotCamera->SetupAttachment(RootComponent);

#if WITH_AUTOMATION_TESTS
	ScreenshotEnvSetup = MakeShareable(new FAutomationTestScreenshotEnvSetup());
#endif
}

void AScreenshotFunctionalTestBase::PrepareTest()
{
	Super::PrepareTest();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetViewTarget(this, FViewTargetTransitionParams());
	}

	PrepareForScreenshot();
}

bool AScreenshotFunctionalTestBase::IsReady_Implementation()
{
	if ((GetWorld()->GetTimeSeconds() - RunTime) > ScreenshotOptions.Delay)
	{
		return (GFrameNumber - RunFrame) > 5;
	}

	return false;
}

void AScreenshotFunctionalTestBase::StartTest()
{
	Super::StartTest();

	FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.AddUObject(this, &AScreenshotFunctionalTestBase::OnScreenshotTakenAndCompared);

	RequestScreenshot();
}

void AScreenshotFunctionalTestBase::OnScreenshotTakenAndCompared()
{
	RestoreViewSettings();

	FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.RemoveAll(this);

	FinishTest(EFunctionalTestResult::Succeeded, TEXT(""));
}

void AScreenshotFunctionalTestBase::PrepareForScreenshot()
{
	check(GEngine->GameViewport && GEngine->GameViewport->GetGameViewport());
	check(IsInGameThread());
	check(!bNeedsViewSettingsRestore && !bNeedsViewportRestore);

#if WITH_AUTOMATION_TESTS
	bool bApplyScreenshotSettings = true;
	FSceneViewport* GameViewport = GEngine->GameViewport->GetGameViewport();

#if WITH_EDITOR_AUTOMATION_TESTS
	// In the editor we can only attempt to resize a standalone viewport
	UWorld* World = GetWorld();
	UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);	

	const bool bIsPIEViewport = GameViewport->IsPlayInEditorViewport();
	const bool bIsNewViewport = World && EditorEngine && EditorEngine->WorldIsPIEInNewViewport(World);

	bApplyScreenshotSettings = !bIsPIEViewport || bIsNewViewport;
#endif	

	if (bApplyScreenshotSettings)
	{
		ScreenshotEnvSetup->Setup(ScreenshotOptions);
		FlushRenderingCommands();
		bNeedsViewSettingsRestore = true;

		// Some platforms (such as consoles) require fixed width/height 
		// back-buffers so we cannot adjust the viewport size
		if (!FPlatformProperties::HasFixedResolution())
		{
			ViewportRestoreSize = GameViewport->GetSize();
			FIntPoint ScreenshotViewportSize = UAutomationBlueprintFunctionLibrary::GetAutomationScreenshotSize(ScreenshotOptions);
			GameViewport->SetViewportSize(ScreenshotViewportSize.X, ScreenshotViewportSize.Y);
			bNeedsViewportRestore = true;
		}
	}
#endif
}

void AScreenshotFunctionalTestBase::OnScreenShotCaptured(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData)
{
	check(GEngine->GameViewport);

	GEngine->GameViewport->OnScreenshotCaptured().RemoveAll(this);
#if WITH_AUTOMATION_TESTS
	FAutomationScreenshotData Data = AutomationCommon::BuildScreenshotData(GetWorld()->GetName(), GetName(), InSizeX, InSizeY);

	// Copy the relevant data into the metadata for the screenshot.
	Data.bHasComparisonRules = true;
	Data.ToleranceRed = ScreenshotOptions.ToleranceAmount.Red;
	Data.ToleranceGreen = ScreenshotOptions.ToleranceAmount.Green;
	Data.ToleranceBlue = ScreenshotOptions.ToleranceAmount.Blue;
	Data.ToleranceAlpha = ScreenshotOptions.ToleranceAmount.Alpha;
	Data.ToleranceMinBrightness = ScreenshotOptions.ToleranceAmount.MinBrightness;
	Data.ToleranceMaxBrightness = ScreenshotOptions.ToleranceAmount.MaxBrightness;
	Data.bIgnoreAntiAliasing = ScreenshotOptions.bIgnoreAntiAliasing;
	Data.bIgnoreColors = ScreenshotOptions.bIgnoreColors;
	Data.MaximumLocalError = ScreenshotOptions.MaximumLocalError;
	Data.MaximumGlobalError = ScreenshotOptions.MaximumGlobalError;

	if (GIsAutomationTesting)
	{
		FAutomationTestFramework::Get().OnScreenshotCompared.AddUObject(this, &AScreenshotFunctionalTestBase::OnComparisonComplete);
	}

	FAutomationTestFramework::Get().OnScreenshotCaptured().ExecuteIfBound(InImageData, Data);

	UE_LOG(LogScreenshotFunctionalTest, Log, TEXT("Screenshot captured as %s"), *Data.Path);
#endif
}

void AScreenshotFunctionalTestBase::RequestScreenshot()
{
	check(IsInGameThread());
	check(GEngine->GameViewport);

	// Make sure any screenshot request has been processed
	FlushRenderingCommands();

	UGameViewportClient* GameViewportClient = GEngine->GameViewport;
	GameViewportClient->OnScreenshotCaptured().AddUObject(this, &AScreenshotFunctionalTestBase::OnScreenShotCaptured);
}

void AScreenshotFunctionalTestBase::OnComparisonComplete(bool bWasNew, bool bWasSimilar, double MaxLocalDifference, double GlobalDifference, FString ErrorMessage)
{
	FAutomationTestFramework::Get().OnScreenshotCompared.RemoveAll(this);

	if (bWasNew)
	{
		UE_LOG(LogScreenshotFunctionalTest, Warning, TEXT("New Screenshot '%s' was discovered!  Please add a ground truth version of it."), *GetName());
	}
	else
	{
		if (bWasSimilar)
		{
			UE_LOG(LogScreenshotFunctionalTest, Display, TEXT("Screenshot '%s' was similar!  Global Difference = %f, Max Local Difference = %f"), *GetName(), GlobalDifference, MaxLocalDifference);
		}
		else
		{
			if (ErrorMessage.IsEmpty())
			{
				UE_LOG(LogScreenshotFunctionalTest, Error, TEXT("Screenshot '%s' test failed, Screnshots were different!  Global Difference = %f, Max Local Difference = %f"), *GetName(), GlobalDifference, MaxLocalDifference);
			}
			else
			{
				UE_LOG(LogScreenshotFunctionalTest, Error, TEXT("Screenshot '%s' test failed;  Error = %s"), *GetName(), *ErrorMessage);
			}
		}
	}

	FAutomationTestFramework::Get().NotifyScreenshotTakenAndCompared();
}

void AScreenshotFunctionalTestBase::RestoreViewSettings()
{
	check(GEngine->GameViewport && GEngine->GameViewport->GetGameViewport());
	check(IsInGameThread());
	
#if WITH_AUTOMATION_TESTS
	if (bNeedsViewSettingsRestore)
	{
		ScreenshotEnvSetup->Restore();
	}

	if (!FPlatformProperties::HasFixedResolution() && bNeedsViewportRestore)
	{	
		FSceneViewport* GameViewport = GEngine->GameViewport->GetGameViewport();
		GameViewport->SetViewportSize(ViewportRestoreSize.X, ViewportRestoreSize.Y);
	}
#endif

	bNeedsViewSettingsRestore = false;
	bNeedsViewportRestore = false;
}

#if WITH_EDITOR

bool AScreenshotFunctionalTestBase::CanEditChange(const UProperty* InProperty) const
{
	bool bIsEditable = Super::CanEditChange(InProperty);
	if (bIsEditable && InProperty)
	{
		const FName PropertyName = InProperty->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(FAutomationScreenshotOptions, ToleranceAmount))
		{
			bIsEditable = ScreenshotOptions.Tolerance == EComparisonTolerance::Custom;
		}
		else if (PropertyName == TEXT("ObservationPoint"))
		{
			// You can't ever observe from anywhere but the camera on the screenshot test.
			bIsEditable = false;
		}
	}

	return bIsEditable;
}

void AScreenshotFunctionalTestBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(FAutomationScreenshotOptions, Tolerance))
		{
			ScreenshotOptions.SetToleranceAmounts(ScreenshotOptions.Tolerance);
		}
	}
}

#endif

void AScreenshotFunctionalTestBase::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAutomationObjectVersion::GUID);

	if (Ar.CustomVer(FAutomationObjectVersion::GUID) < FAutomationObjectVersion::DefaultToScreenshotCameraCutAndFixedTonemapping)
	{
		ScreenshotOptions.bDisableTonemapping = true;
	}
}

