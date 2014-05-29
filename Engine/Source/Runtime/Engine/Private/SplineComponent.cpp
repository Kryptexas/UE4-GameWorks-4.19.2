// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Spline.cpp
=============================================================================*/

#include "EnginePrivate.h"
#include "Components/SplineComponent.h"
#include "ComponentInstanceDataCache.h"


USplineComponent::USplineComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Add 2 keys by default
	int32 PointIndex = SplineInfo.AddPoint(0.f, FVector(0,0,0));
	SplineInfo.Points[PointIndex].InterpMode = CIM_CurveAuto;

	PointIndex = SplineInfo.AddPoint(1.f, FVector(100,0,0));
	SplineInfo.Points[PointIndex].InterpMode = CIM_CurveAuto;

	SplineInfo.AutoSetTangents();
	UpdateSplineReparamTable();
}

void USplineComponent::UpdateSplineReparamTable()
{
	// Start by clearing it
	SplineReparamTable.Reset();
	
	// Nothing to do if no points
	if(SplineInfo.Points.Num() < 2)
	{
		return;
	}
	
	const int32 NumKeys = SplineInfo.Points.Num(); // Number of keys on spline
	const int32 NumSteps = 10 * (NumKeys-1); // TODO: Make this adaptive...
	
	// Find range of input
	float Param = SplineInfo.Points[0].InVal;
	const float MaxInput = SplineInfo.Points[NumKeys-1].InVal;
	const float Interval = (MaxInput - Param)/((float)(NumSteps-1)); 
	
	// Add first entry, using first point on curve, total distance will be 0
	FVector OldSplinePos = SplineInfo.Eval(Param, FVector::ZeroVector);
	float TotalDist = 0.f;
	SplineReparamTable.AddPoint(TotalDist, Param);
	Param += Interval;

	// Then work over rest of points	
	for(int32 i=1; i<NumSteps; i++)
	{
		// Iterate along spline at regular param intervals
		const FVector NewSplinePos = SplineInfo.Eval(Param, FVector::ZeroVector);
		
		TotalDist += (NewSplinePos - OldSplinePos).Size();
		OldSplinePos = NewSplinePos;

		SplineReparamTable.AddPoint(TotalDist, Param);

		// move along
		Param += Interval;
	}
}

float USplineComponent::GetSplineLength() const
{
	// This is given by the input of the last entry in the remap table
	if(SplineReparamTable.Points.Num() > 0)
	{
		return SplineReparamTable.Points[SplineReparamTable.Points.Num()-1].InVal;
	}
	
	return 0.f;
}


FVector USplineComponent::GetWorldLocationAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineReparamTable.Eval(Distance, 0.f);
	const FVector Location = SplineInfo.Eval(Param, FVector::ZeroVector);
	return ComponentToWorld.TransformPosition(Location);
}


FVector USplineComponent::GetWorldDirectionAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineReparamTable.Eval(Distance, 0.f);
	const FVector Tangent = SplineInfo.EvalDerivative(Param, FVector::ZeroVector).SafeNormal();
	return ComponentToWorld.TransformVectorNoScale(Tangent);
}

void USplineComponent::RefreshSplineInputs()
{
	for(int32 KeyIdx=0; KeyIdx<SplineInfo.Points.Num(); KeyIdx++)
	{
		SplineInfo.Points[KeyIdx].InVal = KeyIdx;
	}
}

/** Used to store spline data during RerunConstructionScripts */
class FSplineInstanceData : public FComponentInstanceDataBase
{
public:
	FSplineInstanceData(const USplineComponent* SourceComponent)
		: FComponentInstanceDataBase(SourceComponent)
		, SplineInfo(SourceComponent->SplineInfo)
	{
	}

	/** Lightmaps from LODData */
	FInterpCurveVector SplineInfo;
};

FName USplineComponent::GetComponentInstanceDataType() const
{
	static const FName SplineInstanceDataTypeName(TEXT("SplineInstanceData"));
	return SplineInstanceDataTypeName;
}

TSharedPtr<FComponentInstanceDataBase> USplineComponent::GetComponentInstanceData() const
{
	return MakeShareable(new FSplineInstanceData(this));
}

void USplineComponent::ApplyComponentInstanceData(TSharedPtr<FComponentInstanceDataBase> ComponentInstanceData)
{
	SplineInfo = StaticCastSharedPtr<FSplineInstanceData>(ComponentInstanceData)->SplineInfo;
	UpdateSplineReparamTable();
}