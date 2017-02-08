// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "RBFSolver.h"
#include "EngineLogs.h"

FQuat FRBFEntry::AsQuat() const
{
	FQuat Result = FQuat::Identity;

	if (Values.Num() >= 3)
	{
		const FRotator Rot = FRotator(Values[1], Values[2], Values[0]); // Pitch, Yaw, Roll (see SetFromRotator)
		Result = FQuat(Rot);
	}

	return Result;
}

void FRBFEntry::SetFromRotator(const FRotator& InRot)
{
	Values.Empty();
	Values.SetNum(3);

	Values[0] = InRot.Roll;
	Values[1] = InRot.Pitch;
	Values[2] = InRot.Yaw;
}

void FRBFEntry::SetFromVector(const FVector& InVector)
{
	Values.Empty();
	Values.SetNum(3);

	Values[0] = InVector.X;
	Values[1] = InVector.Y;
	Values[2] = InVector.Z;
}

//////////////////////////////////////////////////////////////////////////

FRBFParams::FRBFParams()
	: TargetDimensions(3)
	, Radius(1.f)
	, Function(ERBFFunctionType::Gaussian)
	, DistanceMethod(ERBFDistanceMethod::Euclidean)
	, TwistAxis(EBoneAxis::BA_X)
	, WeightThreshold(KINDA_SMALL_NUMBER)
{

}

FVector FRBFParams::GetTwistAxisVector() const
{
	switch (TwistAxis)
	{
	case BA_X:
	default:
		return FVector(1.f, 0.f, 0.f);
	case BA_Y:
		return FVector(0.f, 1.f, 0.f);
	case BA_Z:
		return FVector(0.f, 0.f, 1.f);
	}
}

//////////////////////////////////////////////////////////////////////////

float FRBFSolver::FindDistanceBetweenEntries(const FRBFEntry& A, const FRBFEntry& B, const FRBFParams& Params)
{
	check(A.GetDimensions() == B.GetDimensions());

	if (Params.DistanceMethod == ERBFDistanceMethod::Euclidean)
	{
		float DistSqr = 0.f;

		for (int32 i = 0; i < A.Values.Num(); i++)
		{
			DistSqr += FMath::Square(A.Values[i] - B.Values[i]);
		}

		return FMath::Sqrt(DistSqr);
	}
	else if (Params.DistanceMethod == ERBFDistanceMethod::Quaternion)
	{
		float RadDist = A.AsQuat().AngularDistance(B.AsQuat());
		return FMath::RadiansToDegrees(RadDist);
	}
	else if(Params.DistanceMethod == ERBFDistanceMethod::SwingAngle)
	{
		FVector TwistVector = Params.GetTwistAxisVector();
		FVector VecA = A.AsQuat().RotateVector(TwistVector);
		FVector VecB = B.AsQuat().RotateVector(TwistVector);

		const float Dot = FVector::DotProduct(VecA, VecB);
		const float RadDist = FMath::Acos(Dot);
		return FMath::RadiansToDegrees(RadDist);
	}
	else
	{
		ensureMsgf(false, TEXT("Unknown ERBFDistanceMethod"));
		return 0.f;
	}
}

void FRBFSolver::Solve(const FRBFParams& Params, const TArray<FRBFTarget>& Targets, const FRBFEntry& Input, TArray<FRBFOutputWeight>& OutputWeights)
{
	if (Params.TargetDimensions != Input.GetDimensions())
	{
		UE_LOG(LogAnimation, Warning, TEXT("Input dimensionality is %d, expected %d"), Input.GetDimensions(), Params.TargetDimensions);
		return;
	}

	TArray<float> AllWeights;
	AllWeights.AddZeroed(Targets.Num());

	float TotalWeight = 0.f; // Keep track of total weight generated, to renormalize at the end

	// Iterate over each pose, adding its contribution
	for (int32 TargetIdx = 0; TargetIdx < Targets.Num(); TargetIdx++)
	{
		const FRBFTarget& Target = Targets[TargetIdx];

		// Find distance
		const float Distance = FindDistanceBetweenEntries(Target, Input, Params);
		const float Scaling = FMath::Max(Params.Radius * Target.ScaleFactor, KINDA_SMALL_NUMBER);
		const float X = Distance / Scaling;

		// Evaluate radial basis function to find weight
		float Weight = 0.f;
		if (Params.Function == ERBFFunctionType::Gaussian)
		{
			Weight = FMath::Exp(-(X * X));
		}
		else if (Params.Function == ERBFFunctionType::Exponential)
		{
			Weight = 1.f / FMath::Exp(X);
		}
		else if (Params.Function == ERBFFunctionType::Linear)
		{
			Weight = FMath::Max(1.f - X, 0.f);
		}
		else if (Params.Function == ERBFFunctionType::Cubic)
		{
			Weight = FMath::Max(1.f - (X * X * X), 0.f);
		}
		else if (Params.Function == ERBFFunctionType::Quintic)
		{
			Weight = FMath::Max(1.f - (X * X * X * X * X), 0.f);
		}

		// Add to array of all weights. Don't threshold yet, wait for normalization step.
		AllWeights[TargetIdx] = Weight;

		// Add weight to total
		TotalWeight += Weight;
	}

	// Only normalize and apply if we got some kind of weight
	if (TotalWeight > KINDA_SMALL_NUMBER)
	{
		// If total is >1, we renormalize
		const float WeightScale = (TotalWeight > 1.f) ? 1.f / TotalWeight : 1.f;
		for (int32 TargetIdx = 0; TargetIdx < Targets.Num(); TargetIdx++)
		{
			float NormalizedWeight = AllWeights[TargetIdx] * WeightScale;

			// If big enough, add to output list
			if (NormalizedWeight > Params.WeightThreshold)
			{
				OutputWeights.Add(FRBFOutputWeight(TargetIdx, NormalizedWeight));
			}
		}
	}
}


bool FRBFSolver::FindTargetNeighbourDistances(const FRBFParams& Params, const TArray<FRBFTarget>& Targets, TArray<float>& NeighbourDists)
{
	const int32 NumTargets = Targets.Num();

	NeighbourDists.Empty();
	NeighbourDists.AddZeroed(NumTargets);

	if (NumTargets > 1)
	{
		// Iterate over targets
		for (int32 TargetIdx = 0; TargetIdx < NumTargets; TargetIdx++)
		{
			float& NearestDist = NeighbourDists[TargetIdx];
			NearestDist = BIG_NUMBER; // init to large value

			for (int32 OtherTargetIdx = 0; OtherTargetIdx < NumTargets; OtherTargetIdx++)
			{
				if (OtherTargetIdx != TargetIdx) // If not ourself..
				{
					// Get distance between poses
					float Dist = FindDistanceBetweenEntries(Targets[TargetIdx], Targets[OtherTargetIdx], Params);
					NearestDist = FMath::Min(Dist, NearestDist);
				}
			}

			// Avoid zero dist if poses are all on top of each other
			NearestDist = FMath::Max(NearestDist, KINDA_SMALL_NUMBER);
		}

		return true;
	}
	else
	{
		return false;
	}
}
