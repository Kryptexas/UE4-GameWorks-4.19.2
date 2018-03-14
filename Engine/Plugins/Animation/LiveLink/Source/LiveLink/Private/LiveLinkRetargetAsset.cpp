// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkRetargetAsset.h"

#include "Animation/Skeleton.h"
#include "LiveLinkTypes.h"
#include "BonePose.h"
#include "AnimTypes.h"

ULiveLinkRetargetAsset::ULiveLinkRetargetAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULiveLinkRetargetAsset::ApplyCurveValue(const USkeleton* Skeleton, const FName CurveName, const float CurveValue, FBlendedCurve& OutCurve) const
{
	SmartName::UID_Type UID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, CurveName);
	if (UID != SmartName::MaxUID)
	{
		OutCurve.Set(UID, CurveValue);
	}
}

void ULiveLinkRetargetAsset::BuildCurveData(const FLiveLinkSubjectFrame& InFrame, const FCompactPose& InPose, FBlendedCurve& OutCurve) const
{
	const USkeleton* Skeleton = InPose.GetBoneContainer().GetSkeletonAsset();

	for (int32 CurveIdx = 0; CurveIdx < InFrame.CurveKeyData.CurveNames.Num(); ++CurveIdx)
	{
		const FOptionalCurveElement& Curve = InFrame.Curves[CurveIdx];
		if (Curve.IsValid())
		{
			ApplyCurveValue(Skeleton, InFrame.CurveKeyData.CurveNames[CurveIdx], Curve.Value, OutCurve);
		}
	}
}

void ULiveLinkRetargetAsset::BuildCurveData(const TMap<FName, float>& CurveMap, const FCompactPose& InPose, FBlendedCurve& OutCurve) const
{
	const USkeleton* Skeleton = InPose.GetBoneContainer().GetSkeletonAsset();

	for (const TPair<FName, float>& Pair : CurveMap)
	{
		ApplyCurveValue(Skeleton, Pair.Key, Pair.Value, OutCurve);
	}
}