// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"
#include "Animation/AnimInstanceProxy.h"

/////////////////////////////////////////////////////
// FAnimNode_CopyPoseFromMesh

FAnimNode_CopyPoseFromMesh::FAnimNode_CopyPoseFromMesh()
	: SourceMeshComponent(nullptr)
	, bUseAttachedParent (false)
	, bCopyCurves (false)
{
}

void FAnimNode_CopyPoseFromMesh::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	RefreshMeshComponent(Context.AnimInstanceProxy);
}

void FAnimNode_CopyPoseFromMesh::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{

}

void FAnimNode_CopyPoseFromMesh::RefreshMeshComponent(FAnimInstanceProxy* AnimInstanceProxy)
{
	auto ResetMeshComponent = [this](USkeletalMeshComponent* InMeshComponent, FAnimInstanceProxy* InAnimInstanceProxy)
	{
		if (CurrentlyUsedSourceMeshComponent.IsValid() && CurrentlyUsedSourceMeshComponent.Get() != InMeshComponent)
		{
			ReinitializeMeshComponent(InMeshComponent, InAnimInstanceProxy);
		}
		else if (!CurrentlyUsedSourceMeshComponent.IsValid() && InMeshComponent)
		{
			ReinitializeMeshComponent(InMeshComponent, InAnimInstanceProxy);
		}
	};

	if (SourceMeshComponent.IsValid())
	{
		ResetMeshComponent(SourceMeshComponent.Get(), AnimInstanceProxy);
	}
	else if (bUseAttachedParent)
	{
		USkeletalMeshComponent* Component = AnimInstanceProxy->GetSkelMeshComponent();
		if (Component)
		{
			USkeletalMeshComponent* ParentComponent = Cast<USkeletalMeshComponent>(Component->GetAttachParent());
			if (ParentComponent)
			{
				ResetMeshComponent(ParentComponent, AnimInstanceProxy);
			}
			else
			{
				CurrentlyUsedSourceMeshComponent.Reset();
			}
		}
		else
		{
			CurrentlyUsedSourceMeshComponent.Reset();
		}
	}
	else
	{
		CurrentlyUsedSourceMeshComponent.Reset();
	}
}

void FAnimNode_CopyPoseFromMesh::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	RefreshMeshComponent(Context.AnimInstanceProxy);
}

void FAnimNode_CopyPoseFromMesh::Evaluate_AnyThread(FPoseContext& Output)
{
	FCompactPose& OutPose = Output.Pose;
	OutPose.ResetToRefPose();

	USkeletalMeshComponent* CurrentMeshComponent = CurrentlyUsedSourceMeshComponent.IsValid()? CurrentlyUsedSourceMeshComponent.Get() : nullptr;

	if (CurrentMeshComponent && CurrentMeshComponent->SkeletalMesh && CurrentMeshComponent->IsRegistered())
	{
		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
		const TArray<FTransform>& SourcMeshTransformArray = CurrentMeshComponent->GetComponentSpaceTransforms();
		for(FCompactPoseBoneIndex PoseBoneIndex : OutPose.ForEachBoneIndex())
		{
			const int32& SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(PoseBoneIndex);
			const int32& MeshBoneIndex = RequiredBones.GetSkeletonToPoseBoneIndexArray()[SkeletonBoneIndex];
			const int32* Value = BoneMapToSource.Find(MeshBoneIndex);
			if(Value && *Value!=INDEX_NONE)
			{
				const int32 SourceBoneIndex = *Value;
				const int32 ParentIndex = CurrentMeshComponent->SkeletalMesh->RefSkeleton.GetParentIndex(SourceBoneIndex);
				const FCompactPoseBoneIndex MyParentIndex = RequiredBones.GetParentBoneIndex(PoseBoneIndex);
				// only apply if I also have parent, otherwise, it should apply the space bases
				if (ParentIndex != INDEX_NONE && MyParentIndex != INDEX_NONE)
				{
					const FTransform& ParentTransform = SourcMeshTransformArray[ParentIndex];
					const FTransform& ChildTransform = SourcMeshTransformArray[SourceBoneIndex];
					OutPose[PoseBoneIndex] = ChildTransform.GetRelativeTransform(ParentTransform);
				}
				else
				{
					OutPose[PoseBoneIndex] = SourcMeshTransformArray[SourceBoneIndex];
				}
			}
		}

		if (bCopyCurves)
		{
			UAnimInstance* SourceAnimInstance = CurrentMeshComponent->GetAnimInstance();
			if (SourceAnimInstance)
			{
				// attribute curve contains all list
				const TMap<FName, float>& SourceCurveList = SourceAnimInstance->GetAnimationCurveList(EAnimCurveType::AttributeCurve);

				for (auto Iter = SourceCurveList.CreateConstIterator(); Iter; ++Iter)
				{
					const SmartName::UID_Type* UID = CurveNameToUIDMap.Find(Iter.Key());
					if (UID)
					{
						// set source value to output curve
						Output.Curve.Set(*UID, Iter.Value());
					}
				}
			}
		}
	}
}

void FAnimNode_CopyPoseFromMesh::GatherDebugData(FNodeDebugData& DebugData)
{
}

void FAnimNode_CopyPoseFromMesh::ReinitializeMeshComponent(USkeletalMeshComponent* NewSourceMeshComponent, FAnimInstanceProxy* AnimInstanceProxy)
{
	CurrentlyUsedSourceMeshComponent = NewSourceMeshComponent;
	BoneMapToSource.Reset();
	CurveNameToUIDMap.Reset();

	if (NewSourceMeshComponent && NewSourceMeshComponent->SkeletalMesh && !NewSourceMeshComponent->IsPendingKill())
	{
		USkeletalMeshComponent* TargetMeshComponent = AnimInstanceProxy->GetSkelMeshComponent();
		if (TargetMeshComponent)
		{
			USkeletalMesh* SourceSkelMesh = NewSourceMeshComponent->SkeletalMesh;
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

			if (bCopyCurves)
			{
				USkeleton* SourceSkeleton = SourceSkelMesh->Skeleton;
				USkeleton* TargetSkeleton = TargetSkelMesh->Skeleton;

				// you shouldn't be here if this happened
				check(SourceSkeleton && TargetSkeleton);

				const FSmartNameMapping* SourceContainer = SourceSkeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
				const FSmartNameMapping* TargetContainer = TargetSkeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);

				TArray<FName> SourceCurveNames;
				SourceContainer->FillNameArray(SourceCurveNames);
				for (int32 Index = 0; Index < SourceCurveNames.Num(); ++Index)
				{
					SmartName::UID_Type UID = TargetContainer->FindUID(SourceCurveNames[Index]);
					if (UID != SmartName::MaxUID)
					{
						// has a valid UID, add to the list
						SmartName::UID_Type& Value = CurveNameToUIDMap.Add(SourceCurveNames[Index]);
						Value = UID;
					}
				}
			}
		}
	}
}
