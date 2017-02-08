// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AnimNode_ControlRig.h"
#include "HierarchicalRig.h"
#include "ControlRigComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AnimInstanceProxy.h"
#include "GameFramework/Actor.h"

FAnimNode_ControlRig::FAnimNode_ControlRig()
	: CachedControlRig(nullptr)
{
}

void FAnimNode_ControlRig::SetControlRig(UControlRig* InControlRig)
{
	CachedControlRig = InControlRig;
}

UControlRig* FAnimNode_ControlRig::GetControlRig() const
{
	return CachedControlRig.Get();
}

void FAnimNode_ControlRig::RootInitialize(const FAnimInstanceProxy* InProxy)
{
	FAnimNode_SkeletalControlBase::RootInitialize(InProxy);

	if (UControlRig* ControlRig = CachedControlRig.Get())
	{
		ControlRig->Initialize();

		if (UHierarchicalRig* HierControlRig = Cast<UHierarchicalRig>(ControlRig))
		{
			USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(HierControlRig->GetBoundObject());
			if (Component && Component->SkeletalMesh)
			{
				UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(ControlRig->GetClass());
				if (BlueprintClass)
				{
					UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy);
					HierControlRig->NodeMappingContainer = Component->SkeletalMesh->GetNodeMappingContainer(Blueprint);
				}
			}
		}
	}
}

void FAnimNode_ControlRig::GatherDebugData(FNodeDebugData& DebugData)
{

}

void FAnimNode_ControlRig::UpdateInternal(const FAnimationUpdateContext& Context)
{
	if (UControlRig* ControlRig = CachedControlRig.Get())
	{
		ControlRig->PreEvaluate();
		ControlRig->Evaluate();
		ControlRig->PostEvaluate();
	}
}

void FAnimNode_ControlRig::EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(CachedControlRig.Get()))
	{
		for (int32 Index = 0; Index < NodeNames.Num(); ++Index)
		{
			if (NodeNames[Index] != NAME_None)
			{
				FBoneTransform NewTransform;
				NewTransform.BoneIndex = FCompactPoseBoneIndex(Index);
				NewTransform.Transform = HierarchicalRig->GetMappedGlobalTransform(NodeNames[Index]);
				OutBoneTransforms.Add(NewTransform);
			}
		}
	}
}

void FAnimNode_ControlRig::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(CachedControlRig.Get()))
	{
		// @todo: thread-safe? probably not in editor, but it may not be a big issue in editor
		const UNodeMappingContainer* Mapper = HierarchicalRig->NodeMappingContainer;

		// fill up node names
		const TArray<FBoneIndexType>& RequiredBonesArray = RequiredBones.GetBoneIndicesArray();
		const int32 NumBones = RequiredBonesArray.Num();
		NodeNames.Reset(NumBones);
		NodeNames.AddDefaulted(NumBones);

		const FReferenceSkeleton& RefSkeleton = RequiredBones.GetReferenceSkeleton();
		// now fill up node name
		if (Mapper)
		{
			for (int32 Index = 0; Index < NumBones; ++Index)
			{
				// get bone name, and find reverse mapping
				FName TargetNodeName = RefSkeleton.GetBoneName(RequiredBonesArray[Index]);
				FName SourceName = Mapper->GetSourceName(TargetNodeName);
				NodeNames[Index] = SourceName;
			}
		}
		else
		{
			for (int32 Index = 0; Index < NumBones; ++Index)
			{
				NodeNames[Index] = RefSkeleton.GetBoneName(RequiredBonesArray[Index]);
			}
		}
	}

	UE_LOG(LogAnimation, Log, TEXT("%s : %d"), *GetNameSafe(CachedControlRig.Get()), NodeNames.Num())
}

bool FAnimNode_ControlRig::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return CachedControlRig.IsValid();
}