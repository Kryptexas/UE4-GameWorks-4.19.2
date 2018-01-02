// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARSystem.h"
#include "ARBlueprintLibrary.generated.h"


UCLASS(meta=(ScriptName="ARLibrary"))
class AUGMENTEDREALITY_API UARBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//UFUNCTION(BlueprintCallable, Category = "AugmentedReality|LineTrace", meta = (WorldContext = "WorldContextObject", Keywords = "ar augmentedreality augmented reality line trace hit test"))
	//static bool ARLineTraceFromScreenPoint(UObject* WorldContextObject, const FVector2D ScreenPosition, TArray<FARTraceResult>& OutHitResults);
	
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking quality"))
	static EARTrackingQuality GetTrackingQuality();

	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|StartStop", meta = (Keywords = "ar augmentedreality augmented reality start run running"))
	static bool StartAR();
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|StartStop", meta = (Keywords = "ar augmentedreality augmented reality stop run running"))
	static void StopAR();
	
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|StartStop", meta = (Keywords = "ar augmentedreality augmented reality start stop run running"))
	static bool IsARActive();
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking tracing linetrace"))
	static TArray<FARTraceResult> LineTraceTrackedObjects( const FVector2D ScreenCoord );
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking geometry"))
	static TArray<UARTrackedGeometry*> GetAllGeometries();
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static FTransform GetLocalToTrackingTransform( const FARTraceResult& TraceResult );
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static FTransform GetLocalToWorldTransform( const FARTraceResult& TraceResult );
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static UARTrackedGeometry* GetTrackedGeometry( const FARTraceResult& TraceResult );
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Debug", meta = (WorldContext = "WorldContextObject", Keywords = "ar augmentedreality augmented reality tracked geometry debug draw"))
	static void DebugDrawTrackedGeometry( UARTrackedGeometry* TrackedGeometry, UObject* WorldContextObject, FLinearColor Color = FLinearColor(1.0f, 1.0f, 0.0f, 0.75f), float OutlineThickness=2.0f, float PersistForSeconds = 0.0f );
	
public:
	static void RegisterAsARSystem(const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& NewArSystem);
	
private:
	static const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& GetARSystem();
	static TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> RegisteredARSystem;
};
