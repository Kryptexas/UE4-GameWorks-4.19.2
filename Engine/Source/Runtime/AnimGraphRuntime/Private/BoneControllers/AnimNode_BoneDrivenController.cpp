// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
#include "BoneControllers/AnimNode_BoneDrivenController.h"

/////////////////////////////////////////////////////
// FAnimNode_BoneDrivenController

FAnimNode_BoneDrivenController::FAnimNode_BoneDrivenController()
	: SourceComponent(EComponentType::None)
	, Multiplier(1.0f)
	, bUseRange(false)
	, RangeMin(-1.0f)
	, RangeMax(1.0f)
	, TargetComponent(EComponentType::None)
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
	if ((SourceComponent == EComponentType::None) || (TargetComponent == EComponentType::None))
	{
		return;
	}

	// Get the Local space transform and the ref pose transform to see how the transform for the source bone has changed
	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	const FTransform& SourceOrigRef = BoneContainer.GetRefPoseArray()[SourceBone.BoneIndex];
	const FTransform SourceCurr = MeshBases.GetLocalSpaceTransform(SourceBone.GetCompactPoseIndex(BoneContainer));

	// Resolve source value
	float SourceValue = 0.0f;
	if (SourceComponent < EComponentType::RotationX)
	{
		const FVector TranslationDiff = SourceCurr.GetLocation() - SourceOrigRef.GetLocation();
		SourceValue = TranslationDiff[(int32)(SourceComponent - EComponentType::TranslationX)];
	}
	else if (SourceComponent < EComponentType::Scale)
	{
		const FVector RotationDiff = (SourceCurr.GetRotation() * SourceOrigRef.GetRotation().Inverse()).Euler();
		SourceValue = RotationDiff[(int32)(SourceComponent - EComponentType::RotationX)];
	}
	else
	{
		const FVector CurrentScale = SourceCurr.GetScale3D();
		const FVector RefScale = SourceOrigRef.GetScale3D();
		const float ScaleDiff = FMath::Max3(CurrentScale[0], CurrentScale[1], CurrentScale[2]) - FMath::Max3(RefScale[0], RefScale[1], RefScale[2]);
		SourceValue = ScaleDiff;
	}
	
	// Determine the resulting value
	float FinalDriverValue = SourceValue;
	if (DrivingCurve != nullptr)
	{
		// Remap thru the curve if set
		FinalDriverValue = DrivingCurve->GetFloatValue(FinalDriverValue);
	}
	else
	{
		// Apply the fixed function remapping/clamping
		FinalDriverValue *= Multiplier;
		if (bUseRange)
		{
			FinalDriverValue = FMath::Clamp(FinalDriverValue, RangeMin, RangeMax);
		}
	}

	// Difference to apply after processing
	FVector NewRotDiff(FVector::ZeroVector);
	FVector NewTransDiff(FVector::ZeroVector);
	float NewScaleDiff = 0.0f;

	// Resolve target value
	float* DestPtr = nullptr;
	if (TargetComponent < EComponentType::RotationX)
	{
		DestPtr = &NewTransDiff[(int32)(TargetComponent - EComponentType::TranslationX)];
	}
	else if (TargetComponent < EComponentType::Scale)
	{
		DestPtr = &NewRotDiff[(int32)(TargetComponent - EComponentType::RotationX)];
	}
	else
	{
		DestPtr = &NewScaleDiff;
	}

	// Set the value
	*DestPtr = FinalDriverValue;

	// Build final transform difference
	const FTransform FinalDiff(FQuat::MakeFromEuler(NewRotDiff), NewTransDiff, FVector(NewScaleDiff));

	const FCompactPoseBoneIndex TargetBoneIndex = TargetBone.GetCompactPoseIndex(BoneContainer);

	// Starting point for the new transform
	FTransform NewLocal = MeshBases.GetLocalSpaceTransform(TargetBoneIndex);
	NewLocal.AccumulateWithAdditiveScale3D(FinalDiff);

	// If we have a parent, concatenate the transform, otherwise just take the new transform
	const FCompactPoseBoneIndex ParentIndex = MeshBases.GetPose().GetParentBoneIndex(TargetBoneIndex);

	if (ParentIndex != INDEX_NONE)
	{
		const FTransform& ParentTM = MeshBases.GetComponentSpaceTransform(ParentIndex);
		
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
