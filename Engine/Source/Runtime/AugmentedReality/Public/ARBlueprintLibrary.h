// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ARTypes.h"
#include "ARTraceResult.h"
#include "ARSessionConfig.h"
#include "ARBlueprintLibrary.generated.h"



//UCLASS()
//class AUGMENTEDREALITY_API UARPinEventHandlers : public UBlueprintAsyncActionBase
//{
//	GENERATED_BODY()
//
//public:
//	UFUNCTION(BlueprintCallable, Category="AR AugmentedReality|Pin", meta=(BlueprintInternalUseOnly="true"))
//	static UARPinEventHandlers* HandleARPinEvents( UARPin* Pin );
//
//public:
//	UPROPERTY(BlueprintAssignable)
//	FOnARTrackingStateChanged OnARTrackingStateChanged;
//
//	UPROPERTY(BlueprintAssignable)
//	FOnARTransformUpdated OnARTransformUpdated;
//};


UCLASS(meta=(ScriptName="ARLibrary"))
class AUGMENTEDREALITY_API UARBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session start run running"))
	static void StartARSession(UARSessionConfig* SessionConfig);

	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session stop run running"))
	static void PauseARSession();

	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session stop run running"))
	static void StopARSession();
	
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session start stop run running"))
	static FARSessionStatus GetARSessionStatus();

	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Session", meta = (Keywords = "ar augmentedreality augmented reality session"))
	static UARSessionConfig* GetSessionConfig();
	
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Alignment", meta = (Keywords = "ar augmentedreality augmented reality tracking alignment"))
	static void SetAlignmentTransform( const FTransform& InAlignmentTransform );
	
	
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Trace Result", meta = (AdvancedDisplay="1", Keywords = "ar augmentedreality augmented reality tracking tracing linetrace"))
	static TArray<FARTraceResult> LineTraceTrackedObjects( const FVector2D ScreenCoord, bool bTestFeaturePoints = true, bool bTestGroundPlane = true, bool bTestPlaneExtents = true, bool bTestPlaneBoundaryPolygon = true );
	
	

	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking quality"))
	static EARTrackingQuality GetTrackingQuality();
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking geometry anchor"))
	static TArray<UARTrackedGeometry*> GetAllGeometries();
	
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Tracking", meta = (Keywords = "ar augmentedreality augmented reality tracking"))
	static bool IsSessionTypeSupported(EARSessionType SessionType);
	
	
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Debug", meta = (WorldContext = "WorldContextObject", AdvancedDisplay="1", Keywords = "ar augmentedreality augmented reality tracked geometry debug draw"))
	static void DebugDrawTrackedGeometry( UARTrackedGeometry* TrackedGeometry, UObject* WorldContextObject, FLinearColor Color = FLinearColor(1.0f, 1.0f, 0.0f, 0.75f), float OutlineThickness=5.0f, float PersistForSeconds = 0.0f );
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Debug", meta = (WorldContext = "WorldContextObject", AdvancedDisplay="1", Keywords = "ar augmentedreality augmented reality pin debug draw"))
	static void DebugDrawPin( UARPin* ARPin, UObject* WorldContextObject, FLinearColor Color = FLinearColor(1.0f, 1.0f, 0.0f, 0.75f), float Scale=5.0f, float PersistForSeconds = 0.0f );
	
	
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Light Estimate" )
	static UARLightEstimate* GetCurrentLightEstimate();
	
	
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (AdvancedDisplay="3", Keywords = "ar augmentedreality augmented reality tracking pin tracked geometry pinning anchor"))
	static UARPin* PinComponent( USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry = nullptr, const FName DebugName = NAME_None );
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (AdvancedDisplay="2", Keywords = "ar augmentedreality augmented reality tracking pin tracked geometry pinning anchor"))
	static UARPin* PinComponentToTraceResult( USceneComponent* ComponentToPin, const FARTraceResult& TraceResult, const FName DebugName = NAME_None );
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (Keywords = "ar augmentedreality augmented reality tracking pin tracked geometry pinning anchor"))
	static void UnpinComponent( USceneComponent* ComponentToUnpin );
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (Keywords = "ar augmentedreality augmented reality tracking pin tracked geometry pinning anchor"))
	static void RemovePin( UARPin* PinToRemove );
	
	UFUNCTION(BlueprintCallable, Category = "AR AugmentedReality|Pin", meta = (Keywords = "ar augmentedreality augmented reality tracking pin anchor"))
	static TArray<UARPin*> GetAllPins();
	
public:
	static void RegisterAsARSystem(const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& NewArSystem);
	
private:
	static const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& GetARSystem();
	static TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> RegisteredARSystem;
};


UCLASS(meta=(ScriptName="ARLibrary"))
class AUGMENTEDREALITY_API UARTraceResultLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static float GetDistanceFromCamera( const FARTraceResult& TraceResult );
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static FTransform GetLocalToTrackingTransform( const FARTraceResult& TraceResult );
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static FTransform GetLocalToWorldTransform( const FARTraceResult& TraceResult );
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static UARTrackedGeometry* GetTrackedGeometry( const FARTraceResult& TraceResult );
	
	UFUNCTION( BlueprintPure, Category = "AR AugmentedReality|Trace Result" )
	static EARLineTraceChannels GetTraceChannel( const FARTraceResult& TraceResult );
};
