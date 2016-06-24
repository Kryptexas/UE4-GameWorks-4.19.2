// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
#include "AnimNodes/AnimNode_PoseByName.h"
#include "Animation/AnimInstanceProxy.h"

/////////////////////////////////////////////////////
// FAnimPoseByNameNode

void FAnimNode_PoseByName::Initialize(const FAnimationInitializeContext& Context)
{
	FAnimNode_AssetPlayerBase::Initialize(Context);

	if (PoseName != NAME_None)
	{
		USkeleton* Skeleton = Context.AnimInstanceProxy->GetSkeleton();
		PoseUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, PoseName);
	}
	else
	{
		PoseUID = FSmartNameMapping::MaxUID;
	}
}

void FAnimNode_PoseByName::CacheBones(const FAnimationCacheBonesContext& Context) 
{
}

void FAnimNode_PoseByName::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);
}

void FAnimNode_PoseByName::Evaluate(FPoseContext& Output)
{
	if ((PoseAsset!= NULL) && (Output.AnimInstanceProxy->IsSkeletonCompatible(PoseAsset->GetSkeleton())))
	{
		FBlendedCurve PoseCurve;
		PoseCurve.InitFrom(Output.Curve);

		if (PoseUID != FSmartNameMapping::MaxUID)
		{
			PoseCurve.Set(PoseUID, 1.f, ACF_DrivesPose);
		}

		// only give pose curve, we don't set any more curve here
		PoseAsset->GetAnimationPose(Output.Pose, PoseCurve, FAnimExtractContext());

		// this curve data doesn't go out, but it is intentional
	}
	else
	{
		Output.ResetToRefPose();
	}
}

void FAnimNode_PoseByName::OverrideAsset(UAnimationAsset* NewAsset)
{
	if(UPoseAsset* NewPoseAsset = Cast<UPoseAsset>(NewAsset))
	{
		PoseAsset = NewPoseAsset;
	}
}

void FAnimNode_PoseByName::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	
	DebugLine += FString::Printf(TEXT("('%s' Pose: %s)"), *GetNameSafe(PoseAsset), *PoseName.ToString());
	DebugData.AddDebugItem(DebugLine, true);
}