// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AutomationScreenshotOptions.h"

#include "AutomationBlueprintFunctionLibrary.generated.h"

UCLASS()
class FUNCTIONALTESTING_API UAutomationBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
public:
	static bool TakeAutomationScreenshotInternal(const FString& Name, FAutomationScreenshotOptions Options);

	UFUNCTION(BlueprintCallable, Category = "Automation", meta = (Latent, HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", LatentInfo = "LatentInfo", Name = "" ))
	static void TakeAutomationScreenshot(UObject* WorldContextObject, FLatentActionInfo LatentInfo, const FString& Name, FAutomationScreenshotOptions Options);

	UFUNCTION(BlueprintCallable, Category = "Automation", meta = (Latent, HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", LatentInfo = "LatentInfo", NameOverride = "" ))
	static void TakeAutomationScreenshotAtCamera(UObject* WorldContextObject, FLatentActionInfo LatentInfo, ACameraActor* Camera, const FString& NameOverride, FAutomationScreenshotOptions Options);

    //UFUNCTION(BlueprintCallable, Category = "Automation|Performance")
    //static void BeginPerformanceCapture();

    //UFUNCTION(BlueprintCallable, Category = "Automation|Performance")
    //static void EndPerformanceCapture();

	UFUNCTION(BlueprintPure, Category="Automation")
	static bool AreAutomatedTestsRunning();

	UFUNCTION(BlueprintPure, Category="Automation")
	static FAutomationScreenshotOptions GetDefaultScreenshotOptionsForGameplay(EComparisonTolerance Tolerance = EComparisonTolerance::Low);

	UFUNCTION(BlueprintPure, Category="Automation")
	static FAutomationScreenshotOptions GetDefaultScreenshotOptionsForRendering(EComparisonTolerance Tolerance = EComparisonTolerance::Low);
};