// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_SplineIK.h"
#include "Animation/AnimTypes.h"
#include "AnimationRuntime.h"
#include "AnimInstanceProxy.h"

FAnimNode_SplineIK::FAnimNode_SplineIK() 
	: BoneAxis(EAxis::X)
	, bAutoCalculateSpline(true)
	, PointCount(2)
	, Roll(0.0f)
	, TwistStart(0.0f)
	, TwistEnd(0.0f)
	, Stretch(0.0f)
	, Offset(0.0f)
	, OriginalSplineLength(0.0f)
{
}

void FAnimNode_SplineIK::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" StartBone: %s, EndBone: %s)"), *StartBone.BoneName.ToString(), *EndBone.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_SplineIK::RootInitialize(const FAnimInstanceProxy* Proxy)
{
	if (Proxy->GetSkelMeshComponent() && Proxy->GetSkelMeshComponent()->SkeletalMesh)
	{
		GatherBoneReferences(Proxy->GetSkelMeshComponent()->SkeletalMesh->RefSkeleton);
	}
}

void FAnimNode_SplineIK::EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (CachedBoneReferences.Num() > 0)
	{
		const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();

		TransformSpline();

		const float TotalSplineLength = TransformedSpline.GetSplineLength();
		const float TotalStretchRatio = FMath::Lerp(OriginalSplineLength, TotalSplineLength, Stretch) / OriginalSplineLength;
		TwistBlend.SetValueRange(TwistStart, TwistEnd);

		FVector PreviousPoint;
		int32 StartingLinearIndex = 0;
		float InitialAlpha = 0.0f;
		if (Offset == 0.0f)
		{
			PreviousPoint = TransformedSpline.Position.Points[0].OutVal;
		}
		else
		{
			InitialAlpha = FindParamAtFirstSphereIntersection(TransformedSpline.Position.Points[0].OutVal, Offset, StartingLinearIndex);
			PreviousPoint = TransformedSpline.Position.Eval(InitialAlpha);
		}
		const float TotalSplineAlpha = TransformedSpline.ReparamTable.Points.Last().OutVal;
		const int32 BoneCount = CachedBoneReferences.Num();
		for (int32 BoneIndex = 0; BoneIndex < BoneCount; BoneIndex++)
		{
			const FSplineIKCachedBoneData& BoneData = CachedBoneReferences[BoneIndex];
			if (!BoneData.Bone.IsValid(BoneContainer))
			{
				break;
			}

			FCompactPoseBoneIndex CompactPoseBoneIndex = BoneData.Bone.GetCompactPoseIndex(BoneContainer);
			
			const float SplineAlpha = BoneIndex > 0 ? FindParamAtFirstSphereIntersection(PreviousPoint, BoneData.BoneLength * TotalStretchRatio, StartingLinearIndex) : InitialAlpha;
			TwistBlend.SetAlpha(SplineAlpha / TotalSplineAlpha);

			FTransform BoneTransform;
			BoneTransform.SetLocation(TransformedSpline.Position.Eval(SplineAlpha));
			BoneTransform.SetScale3D(TransformedSpline.Scale.Eval(SplineAlpha));
			
			// Get the rotation that the spline provides
			FQuat SplineRotation = TransformedSpline.Rotation.Eval(SplineAlpha);
			
			// Build roll/twist rotation
			float TotalRoll = Roll + TwistBlend.GetBlendedValue();
			FQuat RollRotation = FRotator(BoneAxis == EAxis::Y ? TotalRoll : 0.0f, BoneAxis == EAxis::X ? TotalRoll : 0.0f, BoneAxis == EAxis::Z ? TotalRoll : 0.0f).Quaternion();

			// Calculate rotation to correct our orientation onto the spline's
			FQuat DirectionCorrectingRotation(FQuat::Identity);
			if (BoneIndex > 0)
			{
				const FTransform& PrevBoneTransform = OutBoneTransforms[BoneIndex - 1].Transform;
				FVector NewBoneDir = BoneTransform.GetLocation() - PrevBoneTransform.GetLocation();

				// Only try and correct direction if we get a non-zero tangent.
				if (NewBoneDir.Normalize())
				{
					// Calculate the direction that bone is currently pointing.
					FTransform ComponentSpaceTransform = MeshBases.GetComponentSpaceTransform(CompactPoseBoneIndex);
					FVector CurrentBoneDir = ComponentSpaceTransform.GetUnitAxis(BoneAxis).GetSafeNormal();

					// Calculate a quaternion that gets us from our current rotation to the desired one.
					DirectionCorrectingRotation = FQuat::FindBetweenNormals(CurrentBoneDir, NewBoneDir);
				}
			}

			BoneTransform.SetRotation(RollRotation * DirectionCorrectingRotation * BoneData.OffsetFromBoneRotation * SplineRotation);

			OutBoneTransforms.Emplace(CompactPoseBoneIndex, BoneTransform);

			PreviousPoint = BoneTransform.GetLocation();
		}
	}
}

bool FAnimNode_SplineIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// If any bone references are valid, evaluate.
	if (CachedBoneReferences.Num() > 0)
	{
		for (FSplineIKCachedBoneData& CachedBoneData : CachedBoneReferences)
		{
			if (CachedBoneData.Bone.IsValid(RequiredBones))
			{
				return true;
			}
		}
	}
	
	return false;
}

void FAnimNode_SplineIK::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	StartBone.Initialize(RequiredBones);
	EndBone.Initialize(RequiredBones);

	GatherBoneReferences(RequiredBones.GetReferenceSkeleton());

	for (FSplineIKCachedBoneData& CachedBoneData : CachedBoneReferences)
	{
		CachedBoneData.Bone.Initialize(RequiredBones);
	}
}

FTransform FAnimNode_SplineIK::GetTransformedSplinePoint(int32 TransformIndex) const
{
	if (TransformedSpline.Rotation.Points.IsValidIndex(TransformIndex))
	{
		return FTransform(TransformedSpline.Rotation.Points[TransformIndex].OutVal, TransformedSpline.Position.Points[TransformIndex].OutVal, TransformedSpline.Scale.Points[TransformIndex].OutVal);
	}

	return FTransform::Identity;
}

FTransform FAnimNode_SplineIK::GetControlPoint(int32 TransformIndex) const
{
	if (ControlPoints.IsValidIndex(TransformIndex))
	{
		return ControlPoints[TransformIndex];
	}

	return FTransform::Identity;
}

void FAnimNode_SplineIK::SetControlPoint(int32 TransformIndex, const FTransform& InTransform)
{
	if (ControlPoints.IsValidIndex(TransformIndex))
	{
		ControlPoints[TransformIndex] = InTransform; 
	}
}

void FAnimNode_SplineIK::SetControlPointLocation(int32 TransformIndex, const FVector& InLocation)
{
	if (ControlPoints.IsValidIndex(TransformIndex))
	{
		ControlPoints[TransformIndex].SetLocation(InLocation);
	}
}

void FAnimNode_SplineIK::SetControlPointRotation(int32 TransformIndex, const FQuat& InRotation)
{
	if (ControlPoints.IsValidIndex(TransformIndex))
	{
		ControlPoints[TransformIndex].SetRotation(InRotation);
	}
}

void FAnimNode_SplineIK::SetControlPointScale(int32 TransformIndex, const FVector& InScale)
{
	if (ControlPoints.IsValidIndex(TransformIndex))
	{
		ControlPoints[TransformIndex].SetScale3D(InScale);
	}
}

void FAnimNode_SplineIK::TransformSpline()
{
	TransformedSpline.Position.Reset();
	TransformedSpline.Rotation.Reset();
	TransformedSpline.Scale.Reset();

	for (int32 PointIndex = 0; PointIndex < BoneSpline.Position.Points.Num(); PointIndex++)
	{
		FTransform ControlPoint;
		if (ControlPoints.IsValidIndex(PointIndex))
		{
			ControlPoint = ControlPoints[PointIndex];
		}

		FTransform PointTransform;
		PointTransform.SetLocation(BoneSpline.Position.Points[PointIndex].OutVal + ControlPoint.GetLocation());
		PointTransform.SetRotation(ControlPoint.GetRotation() * BoneSpline.Rotation.Points[PointIndex].OutVal);
		PointTransform.SetScale3D(BoneSpline.Scale.Points[PointIndex].OutVal * ControlPoint.GetScale3D());

		TransformedSpline.Position.Points.Emplace(BoneSpline.Position.Points[PointIndex]);
		TransformedSpline.Rotation.Points.Emplace(BoneSpline.Rotation.Points[PointIndex]);
		TransformedSpline.Scale.Points.Emplace(BoneSpline.Scale.Points[PointIndex]);

		TransformedSpline.Position.Points[PointIndex].OutVal = PointTransform.GetLocation();
		TransformedSpline.Rotation.Points[PointIndex].OutVal = PointTransform.GetRotation();
		TransformedSpline.Scale.Points[PointIndex].OutVal = PointTransform.GetScale3D();
	}

	TransformedSpline.UpdateSpline();

	BuildLinearApproximation(TransformedSpline);
}

float FAnimNode_SplineIK::FindParamAtFirstSphereIntersection(const FVector& InOrigin, float InRadius, int32& StartingLinearIndex)
{
	const float RadiusSquared = InRadius * InRadius;
	const int32 LinearCount = LinearApproximation.Num() - 1;
	for (int32 LinearIndex = StartingLinearIndex; LinearIndex < LinearCount; ++LinearIndex)
	{
		const FLinearApproximation& LinearPoint = LinearApproximation[LinearIndex];
		const FLinearApproximation& NextLinearPoint = LinearApproximation[LinearIndex + 1];

		const float InnerDistanceSquared = (InOrigin - LinearPoint.Position).SizeSquared();
		const float OuterDistanceSquared = (InOrigin - NextLinearPoint.Position).SizeSquared();
		if (InnerDistanceSquared <= RadiusSquared && OuterDistanceSquared >= RadiusSquared)
		{
			StartingLinearIndex = LinearIndex;

			const float InnerDistance = FMath::Sqrt(InnerDistanceSquared);
			const float OuterDistance = FMath::Sqrt(OuterDistanceSquared);
			const float InterpParam = FMath::Clamp((InRadius - InnerDistance) / (OuterDistance - InnerDistance), 0.0f, 1.0f);
			
			return FMath::Lerp(LinearPoint.SplineParam, NextLinearPoint.SplineParam, InterpParam);
		}
	}

	StartingLinearIndex = 0;
	return TransformedSpline.ReparamTable.Points.Last().OutVal;
}

void FAnimNode_SplineIK::BuildLinearApproximation(const FSplineCurves& InCurves)
{
	LinearApproximation.Reset();

	const float SplineLength = InCurves.GetSplineLength();
	int32 NumLinearPoints = (int32)(SplineLength * 0.5f);

	for (int32 LinearPointIndex = 0; LinearPointIndex < NumLinearPoints; ++LinearPointIndex)
	{
		const float DistanceAlpha = (float)LinearPointIndex / (float)NumLinearPoints;
		const float SplineDistance = SplineLength * DistanceAlpha;
		const float Param = InCurves.ReparamTable.Eval(SplineDistance, 0.0f);
		LinearApproximation.Emplace(InCurves.Position.Eval(Param, FVector::ZeroVector), Param);
	}

	LinearApproximation.Emplace(InCurves.Position.Points.Last().OutVal, InCurves.ReparamTable.Points.Last().OutVal);
}

void FAnimNode_SplineIK::GatherBoneReferences(const FReferenceSkeleton& RefSkeleton)
{
	CachedBoneReferences.Reset();

	int32 StartIndex = RefSkeleton.FindBoneIndex(StartBone.BoneName);
	int32 EndIndex = RefSkeleton.FindBoneIndex(EndBone.BoneName);

	if (StartIndex != INDEX_NONE && EndIndex != INDEX_NONE)
	{
		// walk up hierarchy towards root from end to start
		int32 BoneIndex = EndIndex;
		while (BoneIndex != StartIndex)
		{
			// we hit the root, so clear the cached bones - we have an invalid chain
			if (BoneIndex == INDEX_NONE)
			{
				CachedBoneReferences.Reset();
				break;
			}

			FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
			CachedBoneReferences.EmplaceAt(0, BoneName, BoneIndex);

			BoneIndex = RefSkeleton.GetParentIndex(BoneIndex);
		}

		if (CachedBoneReferences.Num())
		{
			FName BoneName = RefSkeleton.GetBoneName(StartIndex);
			CachedBoneReferences.EmplaceAt(0, BoneName, StartIndex);

			// reallocate transform array to match bones
			if (bAutoCalculateSpline)
			{
				ControlPoints.SetNum(CachedBoneReferences.Num());
			}
			else
			{
				ControlPoints.SetNum(FMath::Max(2, PointCount));
			}
		}
	}

	BuildBoneSpline(RefSkeleton);
}

void FAnimNode_SplineIK::BuildBoneSpline(const FReferenceSkeleton& RefSkeleton)
{
	if (CachedBoneReferences.Num() > 0)
	{
		const TArray<FTransform>& RefBonePose = RefSkeleton.GetRefBonePose();

		TArray<FTransform> ComponentSpaceTransforms;
		FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, RefSkeleton.GetRefBonePose(), ComponentSpaceTransforms);

		// Build cached bone info
		for (int32 BoneIndex = 0; BoneIndex < CachedBoneReferences.Num(); BoneIndex++)
		{
			if (BoneIndex > 0)
			{
				FSplineIKCachedBoneData& BoneData = CachedBoneReferences[BoneIndex];
				const FTransform& Transform = ComponentSpaceTransforms[BoneData.RefSkeletonIndex];

				const FSplineIKCachedBoneData& ParentBoneData = CachedBoneReferences[BoneIndex - 1];
				const FTransform& ParentTransform = ComponentSpaceTransforms[ParentBoneData.RefSkeletonIndex];
				const FVector BoneDir = Transform.GetLocation() - ParentTransform.GetLocation();
				BoneData.BoneLength = BoneDir.Size();

				// Calculate a quaternion that gets us from our current rotation to the desired one.
				FVector TransformedAxis = Transform.GetRotation().RotateVector(FMatrix::Identity.GetUnitAxis(BoneAxis)).GetSafeNormal();
				BoneData.OffsetFromBoneRotation = FQuat::FindBetweenNormals(BoneDir.GetSafeNormal(), TransformedAxis);
			}
		}

		// Setup curve params in component space
		BoneSpline.Position.Reset();
		BoneSpline.Rotation.Reset();
		BoneSpline.Scale.Reset();

		const int32 ClampedPointCount = FMath::Max(2, PointCount);
		if (bAutoCalculateSpline || ClampedPointCount == CachedBoneReferences.Num())
		{
			// We are auto-calculating, so use each bone as a control point
			for (int32 BoneIndex = 0; BoneIndex < CachedBoneReferences.Num(); BoneIndex++)
			{
				FSplineIKCachedBoneData& BoneData = CachedBoneReferences[BoneIndex];
				const float CurveAlpha = (float)BoneIndex;

				const FTransform& Transform = ComponentSpaceTransforms[BoneData.RefSkeletonIndex];
				BoneSpline.Position.Points.Emplace(CurveAlpha, Transform.GetLocation(), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
				BoneSpline.Rotation.Points.Emplace(CurveAlpha, Transform.GetRotation(), FQuat::Identity, FQuat::Identity, CIM_Linear);
				BoneSpline.Scale.Points.Emplace(CurveAlpha, Transform.GetScale3D(), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
			}
		}
		else
		{
			// We are not auto-calculating, so we need to build an approximation to the curve. First we build a curve using our transformed curve
			// as a temp storage area, then we evaluate the curve at appropriate points to approximate the bone chain with a new cubic.
			TransformedSpline.Position.Reset();
			TransformedSpline.Rotation.Reset();
			TransformedSpline.Scale.Reset();

			// Build the linear spline
			float TotalBoneCount = (float)(CachedBoneReferences.Num() - 1);
			for (int32 BoneIndex = 0; BoneIndex < CachedBoneReferences.Num(); BoneIndex++)
			{
				const FSplineIKCachedBoneData& BoneData = CachedBoneReferences[BoneIndex];
				const float CurveAlpha = (float)BoneIndex / TotalBoneCount;

				const FTransform& Transform = ComponentSpaceTransforms[BoneData.RefSkeletonIndex];
				TransformedSpline.Position.Points.Emplace(CurveAlpha, Transform.GetLocation(), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
				TransformedSpline.Rotation.Points.Emplace(CurveAlpha, Transform.GetRotation(), FQuat::Identity, FQuat::Identity, CIM_Linear);
				TransformedSpline.Scale.Points.Emplace(CurveAlpha, Transform.GetScale3D(), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
			}

			TransformedSpline.UpdateSpline();

			// now build the approximation

			float TotalPointCount = (float)(ClampedPointCount - 1);
			for (int32 PointIndex = 0; PointIndex < ClampedPointCount; ++PointIndex)
			{
				const float CurveAlpha = (float)PointIndex / TotalPointCount;

				BoneSpline.Position.Points.Emplace(CurveAlpha, TransformedSpline.Position.Eval(CurveAlpha), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
				BoneSpline.Rotation.Points.Emplace(CurveAlpha, TransformedSpline.Rotation.Eval(CurveAlpha), FQuat::Identity, FQuat::Identity, CIM_Linear);
				BoneSpline.Scale.Points.Emplace(CurveAlpha, TransformedSpline.Scale.Eval(CurveAlpha), FVector::ZeroVector, FVector::ZeroVector, CIM_CurveAuto);
			}

			// clear the transformed spline so we dont end up using it
			TransformedSpline.Position.Reset();
			TransformedSpline.Rotation.Reset();
			TransformedSpline.Scale.Reset();
		}

		BoneSpline.UpdateSpline();

		OriginalSplineLength = BoneSpline.GetSplineLength();

		BuildLinearApproximation(BoneSpline);
	}
}
