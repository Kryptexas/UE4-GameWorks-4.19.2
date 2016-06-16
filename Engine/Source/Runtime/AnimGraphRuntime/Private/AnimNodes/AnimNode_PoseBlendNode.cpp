// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
#include "AnimNodes/AnimNode_PoseBlendNode.h"
#include "Animation/AnimInstanceProxy.h"

/////////////////////////////////////////////////////
// FAnimPoseByNameNode

void FAnimNode_PoseBlendNode::Initialize(const FAnimationInitializeContext& Context)
{
	FAnimNode_AssetPlayerBase::Initialize(Context);

	SourcePose.Initialize(Context);
}

void FAnimNode_PoseBlendNode::CacheBones(const FAnimationCacheBonesContext& Context) 
{
	SourcePose.CacheBones(Context);
}

void FAnimNode_PoseBlendNode::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);
	SourcePose.Update(Context);
}

void FAnimNode_PoseBlendNode::Evaluate(FPoseContext& Output)
{
	SourcePose.Evaluate(Output);

	if ((PoseAsset!= NULL) && (Output.AnimInstanceProxy->IsSkeletonCompatible(PoseAsset->GetSkeleton())))
	{
		// only give pose curve, we don't set any more curve here
		PoseAsset->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext());
	}
	// @else it doesn't do anything - it might contain invalid transform but I don't know if overriding transform is the right way 
	// because the node might contain incoming transform
}

void FAnimNode_PoseBlendNode::OverrideAsset(UAnimationAsset* NewAsset)
{
	if(UPoseAsset* NewPoseAsset = Cast<UPoseAsset>(NewAsset))
	{
		PoseAsset = NewPoseAsset;
	}
}

void FAnimNode_PoseBlendNode::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	
	DebugLine += FString::Printf(TEXT("('%s')"), *GetNameSafe(PoseAsset));
	DebugData.AddDebugItem(DebugLine, true);
	SourcePose.GatherDebugData(DebugData.BranchFlow(1.f));
}

