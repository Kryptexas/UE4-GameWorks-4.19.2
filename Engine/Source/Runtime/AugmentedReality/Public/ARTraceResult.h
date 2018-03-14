// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ARTypes.h"
#include "ARTraceResult.generated.h"

class FARSystemBase;

/**
 * A result of an intersection found during a hit-test.
 */
USTRUCT( BlueprintType, Category="AR AugmentedReality", meta=(Experimental))
struct AUGMENTEDREALITY_API FARTraceResult
{
	GENERATED_BODY();
	
	FARTraceResult();
	
	FARTraceResult( const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& InARSystem, float InDistanceFromCamera, EARLineTraceChannels InTraceChannel, const FTransform& InLocalToTrackingTransform, UARTrackedGeometry* InTrackedGeometry );
	
	float GetDistanceFromCamera() const;
	
	FTransform GetLocalToTrackingTransform() const;
	
	FTransform GetLocalToWorldTransform() const;
	
	UARTrackedGeometry* GetTrackedGeometry() const;
	
	EARLineTraceChannels GetTraceChannel() const;
	
private:
	
	/** Distance (in Unreal Units) between the camera and the point where the line trace contacted tracked geometry. */
	UPROPERTY()
	float DistanceFromCamera;
	
	/** The trace channel that generated this trace result. (used for filtering) */
	UPROPERTY()
	EARLineTraceChannels TraceChannel;
	
	/**
	 * The transformation matrix that defines the intersection's rotation, translation and scale
	 * relative to the world.
	 */
	UPROPERTY()
	FTransform LocalToTrackingTransform;
	
	/**
	 * A pointer to the geometry data that was intersected by this trace, if any.
	 */
	UPROPERTY()
	UARTrackedGeometry* TrackedGeometry;
	
	/** A reference to the AR system that creates this hit test result. */
	TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> ARSystem;
};

UCLASS()
class UARTraceResultDummy : public UObject
{
	GENERATED_BODY()
};
