// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AnimNode_LiveLinkPose.h"
#include "ILiveLinkClient.h"

#include "Features/IModularFeatures.h"

#define LOCTEXT_NAMESPACE "LiveLinkAnimNode"

void FAnimNode_LiveLinkPose::Initialize(const FAnimationInitializeContext& Context)
{
	IModularFeatures& ModularFeatures = IModularFeatures::Get();

	if (ModularFeatures.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}
}

void FAnimNode_LiveLinkPose::Evaluate(FPoseContext& Output)
{
	FLiveLinkRefSkeleton RefSkeleton;
	TArray<FTransform> BoneTransforms;
	
	Output.ResetToRefPose();

	if(LiveLinkClient && LiveLinkClient->GetSubjectData(SubjectName, BoneTransforms, RefSkeleton))
	{
		const TArray<FName>& BoneNames = RefSkeleton.GetBoneNames();

		if ((BoneNames.Num() == 0) || (BoneTransforms.Num() == 0) || (BoneNames.Num() != BoneTransforms.Num()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to get live link data %i %i"), BoneNames.Num(), BoneTransforms.Num());
			return;
		}

		for (int32 i = 0; i < BoneNames.Num(); ++i)
		{
			FName BoneName = BoneNames[i];
			FTransform BoneTransform = BoneTransforms[i];

			int32 MeshIndex = Output.Pose.GetBoneContainer().GetPoseBoneIndexForBoneName(BoneName);
			if (MeshIndex != INDEX_NONE)
			{
				FCompactPoseBoneIndex CPIndex = Output.Pose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
				if (CPIndex != INDEX_NONE)
				{
					Output.Pose[CPIndex] = BoneTransform;
				}
			}
		}
	}
}


void FAnimNode_LiveLinkPose::OnLiveLinkClientRegistered(const FName& Type, class IModularFeature* ModularFeature)
{
	if (Type == ILiveLinkClient::ModularFeatureName && !LiveLinkClient)
	{
		LiveLinkClient = static_cast<ILiveLinkClient*>(ModularFeature);
	}
}

void FAnimNode_LiveLinkPose::OnLiveLinkClientUnregistered(const FName& Type, class IModularFeature* ModularFeature)
{
	if (Type == ILiveLinkClient::ModularFeatureName && ModularFeature == LiveLinkClient)
	{
		LiveLinkClient = nullptr;
	}
}

UAnimGraphNode_LiveLinkPose::UAnimGraphNode_LiveLinkPose(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_LiveLinkPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Live Link Pose");
}

FText UAnimGraphNode_LiveLinkPose::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Retrieves the current pose associated with the supplied subject");
}

FText UAnimGraphNode_LiveLinkPose::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Live Link");
}

#undef LOCTEXT_NAMESPACE
