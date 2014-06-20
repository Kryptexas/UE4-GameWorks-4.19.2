// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Spline.cpp
=============================================================================*/

#include "EnginePrivate.h"
#include "Components/SplineComponent.h"
#include "ComponentInstanceDataCache.h"


USplineComponent::USplineComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, ReparamStepsPerSegment(10)
	, Duration(1.0f)
	, bStationaryEndpoints(false)
{
	// Add 2 keys by default
	int32 PointIndex = SplineInfo.AddPoint(0.f, FVector(0,0,0));
	SplineInfo.Points[PointIndex].InterpMode = CIM_CurveAuto;

	PointIndex = SplineInfo.AddPoint(1.f, FVector(100,0,0));
	SplineInfo.Points[PointIndex].InterpMode = CIM_CurveAuto;

	UpdateSpline();
}

void USplineComponent::UpdateSpline()
{
	// Automatically set the tangents on any CurveAuto keys
	SplineInfo.AutoSetTangents(0.0f, bStationaryEndpoints);

	// Start by clearing it
	SplineReparamTable.Reset();
	
	// Nothing to do if no points
	if(SplineInfo.Points.Num() < 2)
	{
		return;
	}

	const int32 NumSegments = SplineInfo.Points.Num() - 1;

	float AccumulatedLength = 0.0f;
	for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
	{
		for (int32 Step = 0; Step < ReparamStepsPerSegment; ++Step)
		{
			const float Param = static_cast<float>(Step) / ReparamStepsPerSegment;
			const float SegmentLength = (Step == 0) ? 0.0f : GetSegmentLength(SegmentIndex, Param);
			SplineReparamTable.AddPoint(SegmentLength + AccumulatedLength, SegmentIndex + Param);
		}
		AccumulatedLength += GetSegmentLength(SegmentIndex, 1.0f);
	}
	SplineReparamTable.AddPoint(AccumulatedLength, static_cast<float>(NumSegments));
}


float USplineComponent::GetSegmentLength(const int32 Index, const float Param) const
{
	check(Index < SplineInfo.Points.Num() - 1);
	check(Param >= 0.0f && Param <= 1.0f);

	// Evaluate the length of a Hermite spline segment.
	// This calculates the integral of |dP/dt| dt, where P(t) is the spline equation with components (x(t), y(t), z(t)).
	// This isn't solvable analytically, so we use a numerical method (Legendre-Gauss quadrature) which performs very well
	// with functions of this type, even with very few samples.  In this case, just 5 samples is sufficient to yield a
	// reasonable result.

	struct FLegendreGaussCoefficient
	{
		float Abscissa;
		float Weight;
	};

	static const FLegendreGaussCoefficient LegendreGaussCoefficients[] =
	{
		{ 0.0f, 0.5688889f },
		{ -0.5384693f, 0.47862867f },
		{ 0.5384693f, 0.47862867f },
		{ -0.90617985f, 0.23692688f },
		{ 0.90617985f, 0.23692688f }
	};

	const auto& StartPoint = SplineInfo.Points[Index];
	const auto& EndPoint = SplineInfo.Points[Index + 1];
	check(EndPoint.InVal - StartPoint.InVal == 1.0f);

	const auto& P0 = StartPoint.OutVal;
	const auto& T0 = StartPoint.LeaveTangent;
	const auto& P1 = EndPoint.OutVal;
	const auto& T1 = EndPoint.ArriveTangent;

	// Cache the coefficients to be fed into the function to calculate the spline derivative at each sample point as they are constant.
	const FVector Coeff1 = ((P0 - P1) * 2.0f + T0 + T1) * 3.0f;
	const FVector Coeff2 = (P1 - P0) * 6.0f - T0 * 4.0f - T1 * 2.0f;
	const FVector Coeff3 = T0;

	const float HalfParam = Param * 0.5f;

	float Length = 0.0f;
	for (const auto& LegendreGaussCoefficient : LegendreGaussCoefficients)
	{
		// Calculate derivative at each Legendre-Gauss sample, and perform a weighted sum
		const float Alpha = HalfParam * (1.0f + LegendreGaussCoefficient.Abscissa);
		const FVector Derivative = (Coeff1 * Alpha + Coeff2) * Alpha + Coeff3;
		Length += Derivative.Size() * LegendreGaussCoefficient.Weight;
	}
	Length *= HalfParam;

	return Length;
}


float USplineComponent::GetSegmentParamFromLength(const int32 Index, const float Length, const float SegmentLength) const
{
	if (SegmentLength == 0.0f)
	{
		return 0.0f;
	}

	// Given a function P(x) which yields points along a spline with x = 0...1, we can define a function L(t) to be the
	// Euclidian length of the spline from P(0) to P(t):
	//
	//    L(t) = integral of |dP/dt| dt
	//         = integral of sqrt((dx/dt)^2 + (dy/dt)^2 + (dz/dt)^2) dt
	//
	// This method evaluates the inverse of this function, i.e. given a length d, it obtains a suitable value for t such that:
	//    L(t) - d = 0
	//
	// We use Newton-Raphson to iteratively converge on the result:
	//
	//    t' = t - f(t) / (df/dt)
	//
	// where: t is an initial estimate of the result, obtained through basic linear interpolation,
	//        f(t) is the function whose root we wish to find = L(t) - d,
	//        (df/dt) = d(L(t))/dt = |dP/dt|

	check(Index < SplineInfo.Points.Num() - 1);
	check(Length >= 0.0f && Length <= SegmentLength);
	check(SplineInfo.Points[Index + 1].InVal - SplineInfo.Points[Index].InVal == 1.0f);

	float Param = Length / SegmentLength;  // initial estimate for t

	// two iterations of Newton-Raphson is enough
	for (int32 Iteration = 0; Iteration < 2; ++Iteration)
	{
		float TangentMagnitude = SplineInfo.EvalDerivative(Index + Param, FVector::ZeroVector).Size();
		if (TangentMagnitude > 0.0f)
		{
			Param -= (GetSegmentLength(Index, Param) - Length) / TangentMagnitude;
			Param = FMath::Clamp(Param, 0.0f, 1.0f);
		}
	}

	return Param;
}


void USplineComponent::ClearSplinePoints()
{
	SplineInfo.Reset();
	SplineReparamTable.Reset();
}


void USplineComponent::AddSplineWorldPoint(const FVector& Position)
{
	float InputKey = static_cast<float>(SplineInfo.Points.Num());
	int32 PointIndex = SplineInfo.AddPoint(InputKey, ComponentToWorld.InverseTransformPosition(Position));
	SplineInfo.Points[PointIndex].InterpMode = CIM_CurveAuto;

	UpdateSpline();
}


void USplineComponent::SetSplineWorldPoints(const TArray<FVector>& Points)
{
	SplineInfo.Reset();
	float InputKey = 0.0f;
	for (const auto& Point : Points)
	{
		int32 PointIndex = SplineInfo.AddPoint(InputKey, ComponentToWorld.InverseTransformPosition(Point));
		SplineInfo.Points[PointIndex].InterpMode = CIM_CurveAuto;
		InputKey += 1.0f;
	}

	UpdateSpline();
}

void USplineComponent::SetWorldLocationAtSplinePoint(int32 PointIndex, const FVector& InLocation)
{
	if((PointIndex >= 0) && (PointIndex < SplineInfo.Points.Num()))
	{
		SplineInfo.Points[PointIndex].OutVal = InLocation;
		UpdateSpline();
	}
}

int32 USplineComponent::GetNumSplinePoints() const
{
	return SplineInfo.Points.Num();
}

FVector USplineComponent::GetWorldLocationAtSplinePoint(int32 PointIndex) const
{
	FVector LocalLocation(0,0,0);
	if ((PointIndex >= 0) && (PointIndex < SplineInfo.Points.Num()))
	{
		LocalLocation = SplineInfo.Points[PointIndex].OutVal;
	}
	return ComponentToWorld.TransformPosition(LocalLocation);
}

void USplineComponent::GetLocalLocationAndTangentAtSplinePoint(int32 PointIndex, FVector& LocalLocation, FVector& LocalTangent) const
{
	LocalTangent = FVector(0, 0, 0);
	LocalLocation = FVector(0, 0, 0);
	if ((PointIndex >= 0) && (PointIndex < SplineInfo.Points.Num()))
	{
		LocalTangent = SplineInfo.Points[PointIndex].LeaveTangent;
		LocalLocation = SplineInfo.Points[PointIndex].OutVal;
	}
}

float USplineComponent::GetSplineLength() const
{
	const int32 NumPoints = SplineReparamTable.Points.Num();

	// This is given by the input of the last entry in the remap table
	if (NumPoints > 0)
	{
		return SplineReparamTable.Points[NumPoints - 1].InVal;
	}
	
	return 0.f;
}


float USplineComponent::GetInputKeyAtDistanceAlongSpline(float Distance) const
{
	const int32 NumPoints = SplineInfo.Points.Num();

	if (NumPoints < 2)
	{
		return 0.0f;
	}

	const float TimeMultiplier = Duration / (NumPoints - 1.0f);
	return SplineReparamTable.Eval(Distance, 0.f) * TimeMultiplier;
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


FRotator USplineComponent::GetWorldRotationAtDistanceAlongSpline(float Distance) const
{
	const float Param = SplineReparamTable.Eval(Distance, 0.f);
	const FVector Forward = SplineInfo.EvalDerivative(Param, FVector(1.0f, 0.0f, 0.0f));
	return Forward.Rotation() + ComponentToWorld.Rotator();
}


FVector USplineComponent::GetWorldLocationAtTime(float Time, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FVector::ZeroVector;
	}

	if (bUseConstantVelocity)
	{
		return GetWorldLocationAtDistanceAlongSpline(Time / Duration * GetSplineLength());
	}

	const float TimeMultiplier = (SplineInfo.Points.Num() - 1.0f) / Duration;
	const FVector Location = SplineInfo.Eval(Time * TimeMultiplier, FVector::ZeroVector);
	return ComponentToWorld.TransformPosition(Location);
}


FVector USplineComponent::GetWorldDirectionAtTime(float Time, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FVector::ZeroVector;
	}

	if (bUseConstantVelocity)
	{
		return GetWorldDirectionAtDistanceAlongSpline(Time / Duration * GetSplineLength());
	}

	const float TimeMultiplier = (SplineInfo.Points.Num() - 1.0f) / Duration;
	const FVector Tangent = SplineInfo.EvalDerivative(Time * TimeMultiplier, FVector::ZeroVector).SafeNormal();
	return ComponentToWorld.TransformVectorNoScale(Tangent);
}


FRotator USplineComponent::GetWorldRotationAtTime(float Time, bool bUseConstantVelocity) const
{
	if (Duration == 0.0f)
	{
		return FRotator::ZeroRotator;
	}

	if (bUseConstantVelocity)
	{
		return GetWorldRotationAtDistanceAlongSpline(Time / Duration * GetSplineLength());
	}

	const float TimeMultiplier = (SplineInfo.Points.Num() - 1.0f) / Duration;
	const FVector Forward = SplineInfo.EvalDerivative(Time * TimeMultiplier, FVector(1.0f, 0.0f, 0.0f));
	return Forward.Rotation() + ComponentToWorld.Rotator();
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
	UpdateSpline();
}

#if WITH_EDITORONLY_DATA
void USplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		const FName PropertyName(PropertyChangedEvent.Property->GetFName());
		if (PropertyName == GET_MEMBER_NAME_CHECKED(USplineComponent, ReparamStepsPerSegment) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(USplineComponent, bStationaryEndpoints))
		{
			UpdateSpline();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif