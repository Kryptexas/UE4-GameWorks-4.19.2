// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkRemapAsset.h"
#include "LiveLinkTypes.h"
#include "BonePose.h"

ULiveLinkRemapAsset::ULiveLinkRemapAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULiveLinkRemapAsset::BuildPoseForSubject(const FLiveLinkSubjectFrame& InFrame, TSharedPtr<FLiveLinkRetargetContext> InOutContext, FCompactPose& OutPose, FBlendedCurve& OutCurve) const
{
	const TArray<FName>& BoneNames = InFrame.RefSkeleton.GetBoneNames();

	if ((BoneNames.Num() == 0) || (InFrame.Transforms.Num() == 0) || (BoneNames.Num() != InFrame.Transforms.Num()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get live link data %i %i"), BoneNames.Num(), InFrame.Transforms.Num());
		return;
	}

	for (int32 i = 0; i < BoneNames.Num(); ++i)
	{
		FName BoneName = BoneNames[i];
		FTransform BoneTransform = InFrame.Transforms[i];

		int32 MeshIndex = OutPose.GetBoneContainer().GetPoseBoneIndexForBoneName(BoneName);
		if (MeshIndex != INDEX_NONE)
		{
			FCompactPoseBoneIndex CPIndex = OutPose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
			if (CPIndex != INDEX_NONE)
			{
				OutPose[CPIndex] = BoneTransform;
			}
		}
	}

	BuildCurveData(InFrame, OutPose, OutCurve);
}