// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
#include "BoneControllers/AnimNode_BoneDrivenController.h"

/////////////////////////////////////////////////////
// FAnimNode_BoneDrivenController

FAnimNode_BoneDrivenController::FAnimNode_BoneDrivenController()
	: SourceComponent(EComponentType::None)
	, TargetComponent(EComponentType::None)
	, Multiplier(1.0f)
	, bUseRange(false)
	, RangeMin(-1.0f)
	, RangeMax(1.0f)
{
}

void FAnimNode_BoneDrivenController::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT("  DrivingBone: %s\nDrivenBone: %s"), *SourceBone.BoneName.ToString(), *TargetBone.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_BoneDrivenController::EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);
	
	// Early out if we're not driving from or to anything
	if(SourceComponent == EComponentType::None || TargetComponent == EComponentType::None || Multiplier == 0.0f)
	{
		return;
	}

	// Get the Local space transform and the ref pose transform to see how the transform for the source bone has changed
	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	const FTransform& SourceOrigRef = BoneContainer.GetRefPoseArray()[SourceBone.BoneIndex];
	FTransform SourceCurr = MeshBases.GetLocalSpaceTransform(SourceBone.GetCompactPoseIndex(BoneContainer));

	// Difference from the ref pose
	FVector RotationDiff = (SourceCurr.GetRotation() * SourceOrigRef.GetRotation().Inverse()).Euler();
	FVector TranslationDiff = SourceCurr.GetLocation() - SourceOrigRef.GetLocation();
	FVector CurrentScale = SourceCurr.GetScale3D();
	FVector RefScale = SourceOrigRef.GetScale3D();
	float ScaleDiff = FMath::Max3(CurrentScale[0], CurrentScale[1], CurrentScale[2]) - FMath::Max3(RefScale[0], RefScale[1], RefScale[2]);
	
	// Difference to apply after processing
	FVector NewRotDiff(FVector::ZeroVector);
	FVector NewTransDiff(FVector::ZeroVector);
	float NewScaleDiff = 0.0f;

	float* SourcePtr = nullptr;
	float* DestPtr = nullptr;

	// Resolve source value
	if(SourceComponent < EComponentType::RotationX)
	{
		SourcePtr = &TranslationDiff[(int32)(SourceComponent - EComponentType::TranslationX)];
	}
	else if(SourceComponent < EComponentType::Scale)
	{
		SourcePtr = &RotationDiff[(int32)(SourceComponent - EComponentType::RotationX)];
	}
	else
	{
		SourcePtr = &ScaleDiff;
	}
	
	// Resolve target value
	if(TargetComponent < EComponentType::RotationX)
	{
		DestPtr = &NewTransDiff[(int32)(TargetComponent - EComponentType::TranslationX)];
	}
	else if(TargetComponent < EComponentType::Scale)
	{
		DestPtr = &NewRotDiff[(int32)(TargetComponent - EComponentType::RotationX)];
	}
	else
	{
		DestPtr = &NewScaleDiff;
	}

	// Set the value; clamped to the given range if necessary.
	if(bUseRange)
	{
		*DestPtr = FMath::Clamp(*SourcePtr * Multiplier, RangeMin, RangeMax);
	}
	else
	{
		*DestPtr = *SourcePtr * Multiplier;
	}

	// Build final transform difference
	FTransform FinalDiff(FQuat::MakeFromEuler(NewRotDiff), NewTransDiff, FVector(NewScaleDiff));

	FCompactPoseBoneIndex TargetBoneIndex = TargetBone.GetCompactPoseIndex(BoneContainer);

	// Starting point for the new transform
	FTransform NewLocal = MeshBases.GetLocalSpaceTransform(TargetBoneIndex);
	NewLocal.AccumulateWithAdditiveScale3D(FinalDiff);

	// If we have a parent, concatenate the transform, otherwise just take the new transform
	FCompactPoseBoneIndex PIdx = MeshBases.GetPose().GetParentBoneIndex(TargetBoneIndex);

	if(PIdx != INDEX_NONE)
	{
		const FTransform& ParentTM = MeshBases.GetComponentSpaceTransform(PIdx);
		
		OutBoneTransforms.Add(FBoneTransform(TargetBoneIndex, NewLocal * ParentTM));
	}
	else
	{
		OutBoneTransforms.Add(FBoneTransform(TargetBoneIndex, NewLocal));
	}

}

bool FAnimNode_BoneDrivenController::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return SourceBone.IsValid(RequiredBones) && TargetBone.IsValid(RequiredBones);
}

void FAnimNode_BoneDrivenController::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	SourceBone.Initialize(RequiredBones);
	TargetBone.Initialize(RequiredBones);
}
