// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ARTypes.h"
#include "ARPin.generated.h"

class FARSystemBase;
class USceneComponent;

UCLASS(BlueprintType, Experimental, Category="AR AugmentedReality")
class AUGMENTEDREALITY_API UARPin : public UObject
{
	GENERATED_BODY()
	
public:
	virtual void InitARPin( const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystemOwner, USceneComponent* InComponentToPin, const FTransform& InLocalToTrackingTransform, UARTrackedGeometry* InTrackedGeometry, const FName InDebugName );

	/**
	 * Maps from a Pin's Local Space to the Tracking Space.
	 * Mapping the origin from the Pin's Local Space to Tracking Space
	 * yield the Pin's position in Tracking Space.
	 */
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Pin")
	FTransform GetLocalToTrackingTransform() const;
	
	
	/**
	 * Convenience function. Same as LocalToTrackingTransform, but
	 * appends the TrackingToWorld Transform.
	 */
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Pin")
	FTransform GetLocalToWorldTransform() const;

	/**
	 * Return the current tracking state of this Pin.
	 */
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Pin")
	EARTrackingState GetTrackingState() const;
	
	/**
	 * The TrackedGeometry (if any) that this this pin is being "stuck" into.
	 */
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Pin")
	UARTrackedGeometry* GetTrackedGeometry() const;
	
	/** @return the PinnedComponent that this UARPin is pinning to the TrackedGeometry */
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Pin")
	USceneComponent* GetPinnedComponent() const;
	
	UFUNCTION()
	virtual void DebugDraw( UWorld* World, const FLinearColor& Color, float Scale = 5.0f, float PersistForSeconds = 0.0f) const;
	
	UFUNCTION(BlueprintPure, Category="AR AugmentedReality|Pin")
	FName GetDebugName() const;
	
	void SetOnARTrackingStateChanged( const FOnARTrackingStateChanged& InHandler );
	
	void SetOnARTransformUpdated( const FOnARTransformUpdated& InHandler );
	
public:
	
	FTransform GetLocalToTrackingTransform_NoAlignment() const;
	
	/** Notify the ARPin about changes to how it is being tracked. */
	void OnTrackingStateChanged(EARTrackingState NewTrackingState);
	
	/** Notify this UARPin that the transform of the Pin has changed */
	void OnTransformUpdated(const FTransform& NewLocalToTrackingTransform);
	
	/** Notify the UARPin that the AlignmentTransform has changing. */
	void UpdateAlignmentTransform( const FTransform& NewAlignmentTransform );
	
	
protected:
	TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> GetARSystem() const;
	
private:
	static uint32 DebugPinId;
	
	UPROPERTY()
	UARTrackedGeometry* TrackedGeometry;
	
	UPROPERTY()
	USceneComponent* PinnedComponent;
	
	UPROPERTY()
	FTransform LocalToTrackingTransform;
	
	UPROPERTY()
	FTransform LocalToAlignedTrackingTransform;

	UPROPERTY()
	EARTrackingState TrackingState;
	
	TWeakPtr<FARSystemBase, ESPMode::ThreadSafe> ARSystem;
	
	FName DebugName;
	
	UPROPERTY(BlueprintAssignable, Category="AR AugmentedReality|Pin")
	FOnARTrackingStateChanged OnARTrackingStateChanged;
	
	UPROPERTY(BlueprintAssignable, Category="AR AugmentedReality|Pin")
	FOnARTransformUpdated OnARTransformUpdated;
	
};
