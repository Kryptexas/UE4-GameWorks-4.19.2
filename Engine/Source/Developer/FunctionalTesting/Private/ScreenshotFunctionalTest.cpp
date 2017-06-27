// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ScreenshotFunctionalTest.h"

#include "Engine/GameViewportClient.h"
#include "AutomationBlueprintFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Misc/AutomationTest.h"

AScreenshotFunctionalTest::AScreenshotFunctionalTest( const FObjectInitializer& ObjectInitializer )
	: AFunctionalTest(ObjectInitializer)
	, ScreenshotOptions(EComparisonTolerance::Low)
{
	ScreenshotCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	ScreenshotCamera->SetupAttachment(RootComponent);
	FieldOfView = ScreenshotCamera->FieldOfView;
}

void AScreenshotFunctionalTest::PrepareTest()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PlayerController->SetViewTarget(this, FViewTargetTransitionParams());

	// It's possible the defaults for certain tolerance levels have changed, so reset them on test start.
	ScreenshotOptions.SetToleranceAmounts(ScreenshotOptions.Tolerance);

	Super::PrepareTest();
}

bool AScreenshotFunctionalTest::IsReady_Implementation()
{
	if ( (GetWorld()->GetTimeSeconds() - RunTime) > ScreenshotOptions.Delay )
	{
		return ( GFrameNumber - RunFrame ) > 5;
	}
	
	return false;
}

void AScreenshotFunctionalTest::StartTest()
{
	Super::StartTest();

	UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotInternal(this, GetName(), ScreenshotOptions);

	FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.AddUObject(this, &AScreenshotFunctionalTest::OnScreenshotTakenAndCompared);
}

void AScreenshotFunctionalTest::OnScreenshotTakenAndCompared()
{
	FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.RemoveAll(this);

	FinishTest(EFunctionalTestResult::Succeeded, TEXT(""));
}

#if WITH_EDITOR

bool AScreenshotFunctionalTest::CanEditChange(const UProperty* InProperty) const
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

void AScreenshotFunctionalTest::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if ( PropertyChangedEvent.Property )
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();

		if ( PropertyName == GET_MEMBER_NAME_CHECKED(FAutomationScreenshotOptions, Tolerance) )
		{
			ScreenshotOptions.SetToleranceAmounts(ScreenshotOptions.Tolerance);
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(AScreenshotFunctionalTest, FieldOfView))
		{
			ScreenshotCamera->SetFieldOfView(FieldOfView);
		}
	}
}

#endif
