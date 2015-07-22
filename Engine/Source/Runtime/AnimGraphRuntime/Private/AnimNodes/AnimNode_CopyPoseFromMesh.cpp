// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"

/////////////////////////////////////////////////////
// FAnimNode_CopyPoseFromMesh

FAnimNode_CopyPoseFromMesh::FAnimNode_CopyPoseFromMesh()
{
}

void FAnimNode_CopyPoseFromMesh::Initialize(const FAnimationInitializeContext& Context)
{
	ReinitializeMeshComponent(Context.AnimInstance);
}

void FAnimNode_CopyPoseFromMesh::CacheBones(const FAnimationCacheBonesContext& Context)
{
}

void FAnimNode_CopyPoseFromMesh::Update(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	if (CurrentlyUsedMeshComponent.IsValid() && CurrentlyUsedMeshComponent.Get() != SourceMeshComponent)
	{
		ReinitializeMeshComponent(Context.AnimInstance);
	}
	else if (!CurrentlyUsedMeshComponent.IsValid() && SourceMeshComponent)
	{
		ReinitializeMeshComponent(Context.AnimInstance);
	}
}

void FAnimNode_CopyPoseFromMesh::Evaluate(FPoseContext& Output)
{
	FCompactPose& OutPose = Output.Pose;
	OutPose.ResetToRefPose();

	if (CurrentlyUsedMeshComponent.IsValid() && CurrentlyUsedMeshComponent.Get()->SkeletalMesh)
	{
		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
		for(FCompactPoseBoneIndex PoseBoneIndex : OutPose.ForEachBoneIndex())
		{
			const int32& SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(PoseBoneIndex);
			const int32* Value = BoneMapToSource.Find(SkeletonBoneIndex);
			if(Value && *Value!=INDEX_NONE)
			{
				const int32 SourceBoneIndex = *Value;
				const int32 ParentIndex = CurrentlyUsedMeshComponent.Get()->SkeletalMesh->RefSkeleton.GetParentIndex(SourceBoneIndex);
				const FCompactPoseBoneIndex MyParentIndex = RequiredBones.GetParentBoneIndex(PoseBoneIndex);
				// only apply if I also have parent, otherwise, it should apply the space bases
				if (ParentIndex != INDEX_NONE && MyParentIndex != INDEX_NONE)
				{
					const FTransform& ParentTransform = CurrentlyUsedMeshComponent.Get()->GetSpaceBases()[ParentIndex];
					const FTransform& ChildTransform = CurrentlyUsedMeshComponent.Get()->GetSpaceBases()[SourceBoneIndex]; 
					OutPose[PoseBoneIndex] = ChildTransform.GetRelativeTransform(ParentTransform);
				}
				else
				{
					OutPose[PoseBoneIndex] = CurrentlyUsedMeshComponent.Get()->GetSpaceBases()[SourceBoneIndex]; 
				}
			}
		}
	}
}

void FAnimNode_CopyPoseFromMesh::GatherDebugData(FNodeDebugData& DebugData)
{
}

void FAnimNode_CopyPoseFromMesh::ReinitializeMeshComponent(UAnimInstance* AnimInstance)
{
	CurrentlyUsedMeshComponent = SourceMeshComponent;
	BoneMapToSource.Reset();
	if (SourceMeshComponent && SourceMeshComponent->SkeletalMesh && !SourceMeshComponent->IsPendingKill())
	{
		USkeletalMeshComponent* TargetMeshComponent = AnimInstance->GetSkelMeshComponent();
		if (TargetMeshComponent)
		{
			USkeletalMesh* SourceSkelMesh = SourceMeshComponent->SkeletalMesh;
			USkeletalMesh* TargetSkelMesh = TargetMeshComponent->SkeletalMesh;

			if (SourceSkelMesh == TargetSkelMesh)
			{
				for(int32 ComponentSpaceBoneId = 0; ComponentSpaceBoneId < SourceSkelMesh->RefSkeleton.GetNum(); ++ComponentSpaceBoneId)
				{
					BoneMapToSource.Add(ComponentSpaceBoneId, ComponentSpaceBoneId);
				}
			}
			else
			{
				for (int32 ComponentSpaceBoneId=0; ComponentSpaceBoneId<TargetSkelMesh->RefSkeleton.GetNum(); ++ComponentSpaceBoneId)
				{
					FName BoneName = TargetSkelMesh->RefSkeleton.GetBoneName(ComponentSpaceBoneId);
					BoneMapToSource.Add(ComponentSpaceBoneId, SourceSkelMesh->RefSkeleton.FindBoneIndex(BoneName));
				}
			}
		}
	}
}