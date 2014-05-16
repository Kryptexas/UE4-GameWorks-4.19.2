// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Spline.cpp
=============================================================================*/

#include "EnginePrivate.h"


USplineComponent::USplineComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Add 2 keys by default
	int32 PointIndex = SplineInfo.AddPoint(0.f, FVector(0,0,0));
	SplineInfo.Points[PointIndex].InterpMode = CIM_CurveAuto;

	PointIndex = SplineInfo.AddPoint(1.f, FVector(100,0,0));
	SplineInfo.Points[PointIndex].InterpMode = CIM_CurveAuto;

	SplineInfo.AutoSetTangents();
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
	
	const int32 NumSteps = 10; // TODO: Make this adaptive...
	
	// Find range of input
	float Param = SplineInfo.Points[0].InVal;
	const float MaxInput = SplineInfo.Points[SplineInfo.Points.Num()-1].InVal;
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


FVector USplineComponent::GetLocationAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineReparamTable.Eval(Distance, 0.f);
	const FVector Location = SplineInfo.Eval(Param, FVector::ZeroVector);
	return Location;
}


FVector USplineComponent::GetTangentAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineReparamTable.Eval(Distance, 0.f);
	const FVector Tangent = SplineInfo.EvalDerivative(Param, FVector::ZeroVector);
	return Tangent;
}

void USplineComponent::RefreshSplineInputs()
{
	for(int32 KeyIdx=0; KeyIdx<SplineInfo.Points.Num(); KeyIdx++)
	{
		SplineInfo.Points[KeyIdx].InVal = KeyIdx;
	}
}

// Init type name static
const FName FSplineInstanceData::SplineInstanceDataTypeName(TEXT("SplineInstanceData"));

void USplineComponent::GetComponentInstanceData(FComponentInstanceDataCache& Cache) const
{
	TSharedRef<FSplineInstanceData> SplineData = MakeShareable(new FSplineInstanceData());
	SplineData->ComponentName = GetFName();
	SplineData->SplineInfo = SplineInfo;

	// Add to cache
	Cache.AddInstanceData(SplineData);
}

void USplineComponent::ApplyComponentInstanceData(const FComponentInstanceDataCache& Cache)
{
	TArray< TSharedPtr<FComponentInstanceDataBase> > CachedData;
	Cache.GetInstanceDataOfType(FSplineInstanceData::SplineInstanceDataTypeName, CachedData);

	for (int32 DataIdx = 0; DataIdx < CachedData.Num(); DataIdx++)
	{
		check(CachedData[DataIdx].IsValid());
		check(CachedData[DataIdx]->GetDataTypeName() == FSplineInstanceData::SplineInstanceDataTypeName);
		TSharedPtr<FSplineInstanceData> SplineData = StaticCastSharedPtr<FSplineInstanceData>(CachedData[DataIdx]);

		// See if data matches current state
		if (SplineData->ComponentName == GetFName())
		{
			SplineInfo = SplineData->SplineInfo;
			UpdateSplineReparamTable();			
		}
	}
}