// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SplineComponent.generated.h"

UCLASS(HeaderGroup=Spline, MinimalAPI)
class USplineComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** Actual data for spline. Locations and tangents are in world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SplineComponent)
	FInterpCurveVector SplineInfo;

	/**
	 * This is how curvy this spline is.  1.0f is straight and anything below that is curvy!
	 * We are doing a simplistic calculate of:  vsize(points) / Length Of Spline 
	 **/
	UPROPERTY(Category=SplineComponent, VisibleAnywhere, BlueprintReadWrite)
	float SplineCurviness;

	/** Color of spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SplineComponent)
	FColor SplineColor;

	/** Resolution to draw spline at */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SplineComponent)
	float SplineDrawRes;

	/** Size of arrow on end of spline. If zero, no arrow drawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SplineComponent)
	float SplineArrowSize;

	/** If true, this spline is for whatever reason disabled, and will be drawn in red. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SplineComponent)
	uint32 bSplineDisabled:1;

	/** Input, distance along curve, output, parameter that puts you there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SplineComponent)
	FInterpCurveFloat SplineReparamTable;


	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End UPrimitiveComponent interface.
	
	/** This will update the spline curviness value **/
	virtual void UpdateSplineCurviness();

	/** Update the SplineReparamTable */
	virtual void UpdateSplineReparamTable();

	/** Returns total length along this spline */
	virtual float GetSplineLength() const;
	
	/** Given a distance along the length of this spline, return the point in space where this puts you */
	virtual FVector GetLocationAtDistanceAlongSpline(float Distance) const;
	
	/** Given a distance along the length of this spline, return the direction of the spline there. Note, result is non-unit length. */
	virtual FVector GetTangentAtDistanceAlongSpline(float Distance) const;
};



