// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SplineComponent.generated.h"



UCLASS(ClassGroup=Shapes, meta=(BlueprintSpawnableComponent))
class ENGINE_API USplineComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** Actual data for spline. Locations and tangents are in world space. */
	UPROPERTY()
	FInterpCurveVector SplineInfo;

	/** Input, distance along curve, output, parameter that puts you there. */
	UPROPERTY()
	FInterpCurveFloat SplineReparamTable;

	// Begin UActorComponent interface.
	virtual TSharedPtr<FComponentInstanceDataBase> GetComponentInstanceData() const OVERRIDE;
	virtual FName GetComponentInstanceDataType() const OVERRIDE;
	virtual void ApplyComponentInstanceData(TSharedPtr<FComponentInstanceDataBase> ComponentInstanceData) OVERRIDE;
	// End UActorComponent interface.

	/** Update the SplineReparamTable */
	void UpdateSplineReparamTable();

	/** Returns total length along this spline */
	UFUNCTION(BlueprintCallable, Category=Spline) 
	float GetSplineLength() const;
	
	/** Given a distance along the length of this spline, return the point in space where this puts you */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetWorldLocationAtDistanceAlongSpline(float Distance) const;
	
	/** Given a distance along the length of this spline, return a unit direction vector of the spline there. */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetWorldDirectionAtDistanceAlongSpline(float Distance) const;

	/** Walk through keys and set time for each one */
	void RefreshSplineInputs();
};



