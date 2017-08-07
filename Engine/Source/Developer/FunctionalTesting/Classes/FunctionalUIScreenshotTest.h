// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "FunctionalTest.h"
#include "AutomationScreenshotOptions.h"
#include "Blueprint/UserWidget.h"

#include "FunctionalUIScreenshotTest.generated.h"

UENUM()
enum class EWidgetTestAppearLocation
{
	Viewport,
	PlayerScreen
};

/**
 * 
 */
UCLASS(Blueprintable, MinimalAPI)
class AFunctionalUIScreenshotTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AFunctionalUIScreenshotTest(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual bool CanEditChange(const UProperty* InProperty) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void PrepareTest() override;
	virtual bool IsReady_Implementation() override;
	virtual void StartTest() override;

	void OnScreenshotTakenAndCompared();
	
protected:

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> WidgetClass;

	UPROPERTY()
	UUserWidget* SpawnedWidget;

	UPROPERTY(EditAnywhere, Category = "UI")
	EWidgetTestAppearLocation WidgetLocation;
	
	UPROPERTY()
	class UCameraComponent* ScreenshotCamera;

	UPROPERTY(EditAnywhere, Category="Screenshot", SimpleDisplay)
	FAutomationScreenshotOptions ScreenshotOptions;
};
