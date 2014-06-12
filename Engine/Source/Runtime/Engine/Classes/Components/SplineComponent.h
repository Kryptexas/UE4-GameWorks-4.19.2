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

	/** Number of steps per spline segment to place in the reparameterization table */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Spline, meta=(ClampMin=4, UIMin=4))
	int32 ReparamStepsPerSegment;

	/** Specifies the duration of the spline in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Spline)
	float Duration;

	// Begin UActorComponent interface.
	virtual TSharedPtr<FComponentInstanceDataBase> GetComponentInstanceData() const OVERRIDE;
	virtual FName GetComponentInstanceDataType() const OVERRIDE;
	virtual void ApplyComponentInstanceData(TSharedPtr<FComponentInstanceDataBase> ComponentInstanceData) OVERRIDE;
	// End UActorComponent interface.

	/** Update the SplineReparamTable */
	void UpdateSplineReparamTable();

	/** Clears all the points in the spline */
	UFUNCTION(BlueprintCallable, Category=Spline)
	void ClearSplinePoints();

	/** Adds a world space point to the spline */
	UFUNCTION(BlueprintCallable, Category=Spline)
	void AddSplineWorldPoint(const FVector& Position);

	/** Sets the spline to an array of world space points */
	UFUNCTION(BlueprintCallable, Category=Spline)
	void SetSplineWorldPoints(const TArray<FVector>& Points);

	/** Move an existing point to a new world location */
	UFUNCTION(BlueprintCallable, Category = Spline)
	void SetWorldLocationAtSplinePoint(int32 PointIndex, const FVector& InLocation);

	/** Get the number of points that make up this spline */
	UFUNCTION(BlueprintCallable, Category=Spline)
	int32 GetNumSplinePoints() const;

	/** Get the world location at spline point */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetWorldLocationAtSplinePoint(int32 PointIndex) const;

	/** Get local location and tangent at a spline point */
	UFUNCTION(BlueprintCallable, Category=Spline)
	void GetLocalLocationAndTangentAtSplinePoint(int32 PointIndex, FVector& LocalLocation, FVector& LocalTangent) const;

	/** Returns total length along this spline */
	UFUNCTION(BlueprintCallable, Category=Spline) 
	float GetSplineLength() const;

	/** Given a distance along the length of this spline, return the corresponding input key at that point */
	UFUNCTION(BlueprintCallable, Category=Spline)
	float GetInputKeyAtDistanceAlongSpline(float Distance) const;

	/** Given a distance along the length of this spline, return the point in space where this puts you */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetWorldLocationAtDistanceAlongSpline(float Distance) const;
	
	/** Given a distance along the length of this spline, return a unit direction vector of the spline tangent there. */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetWorldDirectionAtDistanceAlongSpline(float Distance) const;

	/** Given a distance along the length of this spline, return a rotation corresponding to the spline's position and direction there. */
	UFUNCTION(BlueprintCallable, Category = Spline)
	FRotator GetWorldRotationAtDistanceAlongSpline(float Distance) const;

	/** Given a time from 0 to the spline duration, return the point in space where this puts you */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetWorldLocationAtTime(float Time, bool bUseConstantVelocity = false) const;

	/** Given a time from 0 to the spline duration, return a unit direction vector of the spline tangent there. */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FVector GetWorldDirectionAtTime(float Time, bool bUseConstantVelocity = false) const;

	/** Given a time from 0 to the spline duration, return a rotation corresponding to the spline's position and direction there. */
	UFUNCTION(BlueprintCallable, Category=Spline)
	FRotator GetWorldRotationAtTime(float Time, bool bUseConstantVelocity = false) const;

#if WITH_EDITORONLY_DATA
	// UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface
#endif

	/** Walk through keys and set time for each one */
	void RefreshSplineInputs();

private:
	float GetSegmentLength(const int32 Index, const float Param = 1.0f) const;
	float GetSegmentParamFromLength(const int32 Index, const float Length, const float SegmentLength) const;
};



