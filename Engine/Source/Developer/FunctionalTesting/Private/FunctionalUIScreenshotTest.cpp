// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FunctionalUIScreenshotTest.h"

#include "Engine/GameViewportClient.h"
#include "AutomationBlueprintFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Misc/AutomationTest.h"

AFunctionalUIScreenshotTest::AFunctionalUIScreenshotTest( const FObjectInitializer& ObjectInitializer )
	: AFunctionalTest(ObjectInitializer)
	, ScreenshotOptions(EComparisonTolerance::Low)
{
	ScreenshotCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	ScreenshotCamera->SetupAttachment(RootComponent);

	WidgetLocation = EWidgetTestAppearLocation::Viewport;
}

void AFunctionalUIScreenshotTest::PrepareTest()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PlayerController->SetViewTarget(this, FViewTargetTransitionParams());

	// It's possible the defaults for certain tolerance levels have changed, so reset them on test start.
	ScreenshotOptions.SetToleranceAmounts(ScreenshotOptions.Tolerance);

	// Spawn the widget
	SpawnedWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);

	if (WidgetLocation == EWidgetTestAppearLocation::Viewport)
	{
		SpawnedWidget->AddToViewport();
	}
	else
	{
		SpawnedWidget->AddToPlayerScreen();
	}

	UAutomationBlueprintFunctionLibrary::FinishLoadingBeforeScreenshot();

	Super::PrepareTest();
}

bool AFunctionalUIScreenshotTest::IsReady_Implementation()
{
	if ( (GetWorld()->GetTimeSeconds() - StartTime) > ScreenshotOptions.Delay )
	{
		return ( GFrameNumber - RunFrame ) > 5;
	}
	
	return false;
}

void AFunctionalUIScreenshotTest::StartTest()
{
	Super::StartTest();

	if (SpawnedWidget)
	{
		SpawnedWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotOfUI_Immediate(this, GetName(), ScreenshotOptions);

	FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.AddUObject(this, &AFunctionalUIScreenshotTest::OnScreenshotTakenAndCompared);
}

void AFunctionalUIScreenshotTest::OnScreenshotTakenAndCompared()
{
	FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.RemoveAll(this);

	if (SpawnedWidget)
	{
		SpawnedWidget->RemoveFromParent();
	}

	FinishTest(EFunctionalTestResult::Succeeded, TEXT(""));
}

#if WITH_EDITOR

bool AFunctionalUIScreenshotTest::CanEditChange(const UProperty* InProperty) const
{
	bool bIsEditable = Super::CanEditChange(InProperty);
	if ( bIsEditable && InProperty )
	{
		const FName PropertyName = InProperty->GetFName();

		if ( PropertyName == GET_MEMBER_NAME_CHECKED(FAutomationScreenshotOptions, ToleranceAmount) )
		{
			bIsEditable = ScreenshotOptions.Tolerance == EComparisonTolerance::Custom;
		}
		else if ( PropertyName == TEXT("ObservationPoint") )
		{
			// You can't ever observe from anywhere but the camera on the screenshot test.
			bIsEditable = false;
		}
	}

	return bIsEditable;
}

void AFunctionalUIScreenshotTest::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if ( PropertyChangedEvent.Property )
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();

		if ( PropertyName == GET_MEMBER_NAME_CHECKED(FAutomationScreenshotOptions, Tolerance) )
		{
			ScreenshotOptions.SetToleranceAmounts(ScreenshotOptions.Tolerance);
		}
	}
}

#endif
