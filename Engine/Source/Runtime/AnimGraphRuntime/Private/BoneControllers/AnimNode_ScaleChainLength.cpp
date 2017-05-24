// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_ScaleChainLength.h"
#include "SceneManagement.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimInstanceProxy.h"

/////////////////////////////////////////////////////
// FAnimNode_ScaleChainLength

FAnimNode_ScaleChainLength::FAnimNode_ScaleChainLength()
{
}

void FAnimNode_ScaleChainLength::Initialize(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize(Context);
	InputPose.Initialize(Context);
}

void FAnimNode_ScaleChainLength::Update(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);
	InputPose.Update(Context);
}

void FAnimNode_ScaleChainLength::CacheBones(const FAnimationCacheBonesContext& Context)
{
	InputPose.CacheBones(Context);

	// LOD change, recache bone indices.
	bBoneIndicesCached = false;
}

void FAnimNode_ScaleChainLength::Evaluate(FPoseContext& Output)
{
	// Evaluate incoming pose into our output buffer.
	InputPose.Evaluate(Output);

	const float FinalAlpha = AlphaScaleBias.ApplyTo(Alpha);
	if (!FAnimWeight::IsRelevant(FinalAlpha))
	{
		return;
	}

	const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();

	if (!bBoneIndicesCached)
	{
		bBoneIndicesCached = true;

		ChainStartBone.Initialize(BoneContainer);
		ChainEndBone.Initialize(BoneContainer);
		ChainBoneIndices.Reset();

		// Make sure we have valid start/end bones, and that end is a child of start.
		// Cache this, so we only evaluate on init and LOD changes.
		const bool bBoneSetupIsValid = ChainStartBone.IsValid(BoneContainer) && ChainEndBone.IsValid(BoneContainer) &&
			BoneContainer.BoneIsChildOf(ChainEndBone.GetCompactPoseIndex(BoneContainer), ChainStartBone.GetCompactPoseIndex(BoneContainer));

		if (bBoneSetupIsValid)
		{
			FCompactPoseBoneIndex StartBoneIndex = ChainStartBone.GetCompactPoseIndex(BoneContainer);
			FCompactPoseBoneIndex BoneIndex = ChainEndBone.GetCompactPoseIndex(BoneContainer);
			ChainBoneIndices.Add(BoneIndex);
			if (BoneIndex != INDEX_NONE)
			{
				FCompactPoseBoneIndex ParentBoneIndex = BoneContainer.GetParentBoneIndex(BoneIndex);
				while ((ParentBoneIndex != INDEX_NONE) && (ParentBoneIndex != StartBoneIndex))
				{
					BoneIndex = ParentBoneIndex;
					ParentBoneIndex = BoneContainer.GetParentBoneIndex(BoneIndex);
					ChainBoneIndices.Insert(BoneIndex, 0);
				};
				ChainBoneIndices.Insert(StartBoneIndex, 0);
			}
		}
	}

	// Need at least Start/End bones to be valid.
	if (ChainBoneIndices.Num() < 2)
	{
		return;
	}

	const FVector TargetLocationCompSpace = Output.AnimInstanceProxy->GetSkelMeshCompLocalToWorld().InverseTransformPosition(TargetLocation);

	// Allocate transforms to get component space transform of chain start bone.
	FCSPose<FCompactPose> CSPose;
	CSPose.InitPose(Output.Pose);

	const FTransform StartTransformCompSpace = CSPose.GetComponentSpaceTransform(ChainBoneIndices[0]);

	const float DesiredChainLength = (TargetLocationCompSpace - StartTransformCompSpace.GetLocation()).Size();
	const float ChainLengthScale = !FMath::IsNearlyZero(DefaultChainLength) ? (DesiredChainLength / DefaultChainLength) : 1.f;
	const float ChainLengthScaleWithAlpha = FMath::LerpStable(1.f, ChainLengthScale, FinalAlpha);

	// If we're not going to scale anything, early out.
	if (FMath::IsNearlyEqual(ChainLengthScaleWithAlpha, 1.f))
	{
		return;
	}

	// Scale translation of all bones.
	FCompactPose& BSPose = Output.Pose;
	for (const FCompactPoseBoneIndex& BoneIndex : ChainBoneIndices)
	{
		// Get bone space transform, scale transition.
		BSPose[BoneIndex].ScaleTranslation(ChainLengthScaleWithAlpha);
	}
}

void FAnimNode_ScaleChainLength::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugData.AddDebugItem(DebugLine);
	InputPose.GatherDebugData(DebugData);
}
