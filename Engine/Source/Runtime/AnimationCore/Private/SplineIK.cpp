// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SplineIK.h"

namespace AnimationCore
{

void SolveSplineIK(const TArray<FTransform>& BoneTransforms, const FInterpCurveVector& PositionSpline, const FInterpCurveQuat& RotationSpline, const FInterpCurveVector& ScaleSpline, const float TotalSplineAlpha, const float TotalSplineLength, const FFloatMapping& Twist, const float Roll, const float Stretch, const float Offset, const EAxis::Type BoneAxis, const FFindParamAtFirstSphereIntersection& FindParamAtFirstSphereIntersection, const TArray<FQuat>& BoneOffsetRotations, const TArray<float>& BoneLengths, const float OriginalSplineLength, TArray<FTransform>& OutBoneTransforms)
{
	check(BoneTransforms.Num() == BoneLengths.Num());
	check(BoneTransforms.Num() == BoneOffsetRotations.Num());
	check(Twist.IsBound());
	check(FindParamAtFirstSphereIntersection.IsBound());

	OutBoneTransforms.Reset();

	const float TotalStretchRatio = FMath::Lerp(OriginalSplineLength, TotalSplineLength, Stretch) / OriginalSplineLength;

	FVector PreviousPoint;
	int32 StartingLinearIndex = 0;
	float InitialAlpha = 0.0f;
	if (Offset == 0.0f)
	{
		PreviousPoint = PositionSpline.Points[0].OutVal;
	}
	else
	{
		InitialAlpha = FindParamAtFirstSphereIntersection.Execute(PositionSpline.Points[0].OutVal, Offset, StartingLinearIndex);
		PreviousPoint = PositionSpline.Eval(InitialAlpha);
	}
//	const float TotalSplineAlpha = ReparamTable.Points.Last().OutVal;
	const int32 BoneCount = BoneTransforms.Num();
	for (int32 BoneIndex = 0; BoneIndex < BoneCount; BoneIndex++)
	{
		const float SplineAlpha = BoneIndex > 0 ? FindParamAtFirstSphereIntersection.Execute(PreviousPoint, BoneLengths[BoneIndex] * TotalStretchRatio, StartingLinearIndex) : InitialAlpha;

		FTransform BoneTransform;
		BoneTransform.SetLocation(PositionSpline.Eval(SplineAlpha));
		BoneTransform.SetScale3D(ScaleSpline.Eval(SplineAlpha));

		// Get the rotation that the spline provides
		FQuat SplineRotation = RotationSpline.Eval(SplineAlpha);

		// Build roll/twist rotation
		const float TotalRoll = Roll + Twist.Execute(SplineAlpha / TotalSplineAlpha);
		FQuat RollRotation = FRotator(BoneAxis == EAxis::Y ? TotalRoll : 0.0f, BoneAxis == EAxis::X ? TotalRoll : 0.0f, BoneAxis == EAxis::Z ? TotalRoll : 0.0f).Quaternion();

		// Calculate rotation to correct our orientation onto the spline's
		FQuat DirectionCorrectingRotation(FQuat::Identity);
		if (BoneIndex > 0)
		{
			FVector NewBoneDir = BoneTransform.GetLocation() - OutBoneTransforms[BoneIndex - 1].GetLocation();

			// Only try and correct direction if we get a non-zero tangent.
			if (NewBoneDir.Normalize())
			{
				// Calculate the direction that bone is currently pointing.
				const FVector CurrentBoneDir = BoneTransforms[BoneIndex].GetUnitAxis(BoneAxis).GetSafeNormal();

				// Calculate a quaternion that gets us from our current rotation to the desired one.
				DirectionCorrectingRotation = FQuat::FindBetweenNormals(CurrentBoneDir, NewBoneDir);
			}
		}

		BoneTransform.SetRotation(RollRotation * DirectionCorrectingRotation * BoneOffsetRotations[BoneIndex] * SplineRotation);

		OutBoneTransforms.Add(BoneTransform);

		PreviousPoint = BoneTransform.GetLocation();
	}
}

}