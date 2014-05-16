// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SplineComponent.generated.h"

/** Used to store spline data during RerunConstructionScripts */
class FSplineInstanceData : public FComponentInstanceDataBase
{
public:
	static const FName SplineInstanceDataTypeName;

	virtual ~FSplineInstanceData()
	{}

	// Begin FComponentInstanceDataBase interface
	virtual FName GetDataTypeName() const OVERRIDE
	{
		return SplineInstanceDataTypeName;
	}
	// End FComponentInstanceDataBase interface

	/** Name of component */
	FName ComponentName;

	/** Lightmaps from LODData */
	FInterpCurveVector SplineInfo;
};

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
	virtual void GetComponentInstanceData(FComponentInstanceDataCache& Cache) const OVERRIDE;
	virtual void ApplyComponentInstanceData(const FComponentInstanceDataCache& Cache) OVERRIDE;
	// End UActorComponent interface.

	/** Update the SplineReparamTable */
	void UpdateSplineReparamTable();

	/** Returns total length along this spline */
	UFUNCTION(BlueprintCallable, Category=Spline) 
	float GetSplineLength() const;
	
	/** Given a distance along the length of this spline, return the point in space where this puts you */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetLocationAtDistanceAlongSpline(float Distance) const;
	
	/** Given a distance along the length of this spline, return the direction of the spline there. Note, result is non-unit length. */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetTangentAtDistanceAlongSpline(float Distance) const;

	/** Walk through keys and set time for each one */
	void RefreshSplineInputs();
};



