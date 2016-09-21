// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AutomationBlueprintFunctionLibrary.generated.h"

UCLASS()
class FUNCTIONALTESTING_API UAutomationBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
public:
	static bool TakeAutomationScreenshotInternal(const FString& Name, FIntPoint Resolution);

	UFUNCTION(BlueprintCallable, Category = "StudiosAutomation", meta = (Latent, HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", LatentInfo = "LatentInfo", Name = "", DelayBeforeScreenshotSeconds = "0.2" ))
	static void TakeAutomationScreenshot(UObject* WorldContextObject, FLatentActionInfo LatentInfo, const FString& Name, FIntPoint Resolution, float DelayBeforeScreenshotSeconds);

	UFUNCTION(BlueprintCallable, Category = "StudiosAutomation|Helpers", meta = (Latent, HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", LatentInfo = "LatentInfo", NameOverride = "", DelayBeforeScreenshotSeconds = "0.2"))
	static void TakeAutomationScreenshotAtCamera(UObject* WorldContextObject, FLatentActionInfo LatentInfo, ACameraActor* Camera, const FString& NameOverride, FIntPoint Resolution, float DelayBeforeScreenshotSeconds);

    UFUNCTION(BlueprintCallable, Category = "StudiosAutomation|Performance")
    static void BeginPerformanceCapture();

    UFUNCTION(BlueprintCallable, Category = "StudiosAutomation|Performance")
    static void EndPerformanceCapture();
};