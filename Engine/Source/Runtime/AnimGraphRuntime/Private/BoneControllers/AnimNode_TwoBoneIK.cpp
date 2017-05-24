// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_TwoBoneIK.h"
#include "AnimationRuntime.h"
#include "TwoBoneIK.h"
#include "Animation/AnimInstanceProxy.h"

DECLARE_CYCLE_STAT(TEXT("TwoBoneIK Eval"), STAT_TwoBoneIK_Eval, STATGROUP_Anim);


/////////////////////////////////////////////////////
// FAnimNode_TwoBoneIK

FAnimNode_TwoBoneIK::FAnimNode_TwoBoneIK()
	: EffectorLocation(FVector::ZeroVector)
	, JointTargetLocation(FVector::ZeroVector)
	, bTakeRotationFromEffectorSpace(false)
	, bMaintainEffectorRelRot(false)
	, bAllowStretching(false)
	, StretchLimits_DEPRECATED(FVector2D::ZeroVector)
	, StartStretchRatio(1.f)
	, MaxStretchScale(1.2f)
	, EffectorLocationSpace(BCS_ComponentSpace)
	, JointTargetLocationSpace(BCS_ComponentSpace)
{
}

void FAnimNode_TwoBoneIK::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" IKBone: %s)"), *IKBone.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_TwoBoneIK::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_TwoBoneIK_Eval);

	check(OutBoneTransforms.Num() == 0);

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidLimb = false;

	FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKBone.GetCompactPoseIndex(BoneContainer);

	FCompactPoseBoneIndex UpperLimbIndex(INDEX_NONE);
	const FCompactPoseBoneIndex LowerLimbIndex = BoneContainer.GetParentBoneIndex(IKBoneCompactPoseIndex);
	if (LowerLimbIndex == INDEX_NONE)
	{
		bInvalidLimb = true;
	}
	else
	{
		UpperLimbIndex = BoneContainer.GetParentBoneIndex(LowerLimbIndex);
		if (UpperLimbIndex == INDEX_NONE)
		{
			bInvalidLimb = true;
		}
	}

	const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	const int32 EffectorBoneIndex = bInBoneSpace ? BoneContainer.GetPoseBoneIndexForBoneName(EffectorSpaceBoneName) : INDEX_NONE;
	const FCompactPoseBoneIndex EffectorSpaceBoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(EffectorBoneIndex));

	if (bInBoneSpace && (EffectorSpaceBoneIndex == INDEX_NONE))
	{
		bInvalidLimb = true;
	}

	// If we walked past the root, this controlled is invalid, so return no affected bones.
	if( bInvalidLimb )
	{
		return;
	}

	// Get Local Space transforms for our bones. We do this first in case they already are local.
	// As right after we get them in component space. (And that does the auto conversion).
	// We might save one transform by doing local first...
	const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);

	// Now get those in component space...
	FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(LowerLimbIndex);
	FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();

	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform EffectorTransform(EffectorLocation);
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, EffectorTransform, EffectorSpaceBoneIndex, EffectorLocationSpace);

	// Get joint target (used for defining plane that joint should be in).
	FTransform JointTargetTransform(JointTargetLocation);
	FCompactPoseBoneIndex JointTargetSpaceBoneIndex(INDEX_NONE);

	if (JointTargetLocationSpace == BCS_ParentBoneSpace || JointTargetLocationSpace == BCS_BoneSpace)
	{
		int32 Index = BoneContainer.GetPoseBoneIndexForBoneName(JointTargetSpaceBoneName);
		JointTargetSpaceBoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));
	}

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, JointTargetTransform, JointTargetSpaceBoneIndex, JointTargetLocationSpace);

	FVector	JointTargetPos = JointTargetTransform.GetTranslation();

	// This is our reach goal.
	FVector DesiredPos = EffectorTransform.GetTranslation();

	// IK solver
	UpperLimbCSTransform.SetLocation(RootPos);
	LowerLimbCSTransform.SetLocation(InitialJointPos);
	EndBoneCSTransform.SetLocation(InitialEndPos);

	AnimationCore::SolveTwoBoneIK(UpperLimbCSTransform, LowerLimbCSTransform, EndBoneCSTransform, JointTargetPos, DesiredPos, bAllowStretching, StartStretchRatio, MaxStretchScale);

	// Update transform for upper bone.
	{
		// Order important. First bone is upper limb.
		OutBoneTransforms.Add( FBoneTransform(UpperLimbIndex, UpperLimbCSTransform) );
	}

	// Update transform for lower bone.
	{
		// Order important. Second bone is lower limb.
		OutBoneTransforms.Add( FBoneTransform(LowerLimbIndex, LowerLimbCSTransform) );
	}

	// Update transform for end bone.
	{
		// only allow bTakeRotationFromEffectorSpace during bone space
		if (bInBoneSpace && bTakeRotationFromEffectorSpace)
		{
			EndBoneCSTransform.SetRotation(EffectorTransform.GetRotation());
		}
		else if (bMaintainEffectorRelRot)
		{
			EndBoneCSTransform = EndBoneLocalTransform * LowerLimbCSTransform;
		}
		// Order important. Third bone is End Bone.
		OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));
	}

	// Make sure we have correct number of bones
	check(OutBoneTransforms.Num() == 3);
}

bool FAnimNode_TwoBoneIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	return (IKBone.IsValid(RequiredBones));
}

void FAnimNode_TwoBoneIK::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	IKBone.Initialize(RequiredBones);
}
