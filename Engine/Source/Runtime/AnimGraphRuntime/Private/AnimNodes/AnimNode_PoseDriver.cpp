// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/AnimNode_PoseDriver.h"
#include "AnimationRuntime.h"
#include "Serialization/CustomVersion.h"
#include "Animation/AnimInstanceProxy.h"
#include "RBF/RBFSolver.h"


FAnimNode_PoseDriver::FAnimNode_PoseDriver()
{
	RadialScaling_DEPRECATED = 0.25f;
	Type_DEPRECATED = EPoseDriverType::SwingOnly;

	DriveSource = EPoseDriverSource::Rotation;
	DriveOutput = EPoseDriverOutput::DrivePoses;

	RBFParams.DistanceMethod = ERBFDistanceMethod::SwingAngle;
}

void FAnimNode_PoseDriver::Initialize(const FAnimationInitializeContext& Context)
{
	FAnimNode_PoseHandler::Initialize(Context);

	SourcePose.Initialize(Context);

	CacheDrivenIDs(Context.AnimInstanceProxy->GetSkeleton());
}

void FAnimNode_PoseDriver::CacheDrivenIDs(USkeleton* Skeleton)
{
	// Cache UIDs for driving curves
	if (DriveOutput == EPoseDriverOutput::DriveCurves)
	{
		for (FPoseDriverTarget& PoseTarget : PoseTargets)
		{
			PoseTarget.DrivenUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, PoseTarget.DrivenName);
		}
	}
	// If not driving curves, init to INDEX_NONE
	else
	{
		for (FPoseDriverTarget& PoseTarget : PoseTargets)
		{
			PoseTarget.DrivenUID = INDEX_NONE;
		}
	}
}

void FAnimNode_PoseDriver::CacheBones(const FAnimationCacheBonesContext& Context)
{
	FAnimNode_PoseHandler::CacheBones(Context);
	// Init pose input
	SourcePose.CacheBones(Context);
	// Init bone ref
	SourceBone.Initialize(Context.AnimInstanceProxy->GetRequiredBones());
	EvalSpaceBone.Initialize(Context.AnimInstanceProxy->GetRequiredBones());
}

void FAnimNode_PoseDriver::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	FAnimNode_PoseHandler::UpdateAssetPlayer(Context);
	SourcePose.Update(Context);
}

void FAnimNode_PoseDriver::GatherDebugData(FNodeDebugData& DebugData)
{
	FAnimNode_PoseHandler::GatherDebugData(DebugData);
	SourcePose.GatherDebugData(DebugData.BranchFlow(1.f));
}



void FAnimNode_PoseDriver::GetRBFTargets(TArray<FRBFTarget>& OutTargets) const
{
	OutTargets.Empty();
	OutTargets.AddZeroed(PoseTargets.Num());

	for (int32 TargetIdx = 0; TargetIdx < PoseTargets.Num(); TargetIdx++)
	{
		FRBFTarget& RBFTarget = OutTargets[TargetIdx];
		const FPoseDriverTarget& PoseTarget = PoseTargets[TargetIdx];

		if (DriveSource == EPoseDriverSource::Translation)
		{
			RBFTarget.SetFromVector(PoseTarget.TargetTranslation);
		}
		else
		{
			RBFTarget.SetFromRotator(PoseTarget.TargetRotation);
		}

		RBFTarget.ScaleFactor = PoseTarget.TargetScale;
	}
}


void FAnimNode_PoseDriver::Evaluate(FPoseContext& Output)
{
	// Udpate DrivenIDs if needed
	if (bCachedDrivenIDsAreDirty)
	{
		CacheDrivenIDs(Output.AnimInstanceProxy->GetSkeleton());
	}

	FPoseContext SourceData(Output);
	SourcePose.Evaluate(SourceData);

	// Get the index of the source bone
	const FBoneContainer& BoneContainer = SourceData.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex SourceCompactIndex = SourceBone.GetCompactPoseIndex(BoneContainer);

	// Do nothing if bone is not found/LOD-ed out
	if (SourceCompactIndex == INDEX_NONE)
	{
		Output = SourceData;
		return;
	}

	// If evaluating in alternative bone space, have to build component space pose
	if (EvalSpaceBone.IsValid(BoneContainer))
	{
		FCSPose<FCompactPose> CSPose;
		CSPose.InitPose(SourceData.Pose);

		const FCompactPoseBoneIndex EvalSpaceCompactIndex = EvalSpaceBone.GetCompactPoseIndex(BoneContainer);
		FTransform EvalSpaceCompSpace = CSPose.GetComponentSpaceTransform(EvalSpaceCompactIndex);
		FTransform SourceBoneCompSpace = CSPose.GetComponentSpaceTransform(SourceCompactIndex);

		SourceBoneTM = SourceBoneCompSpace.GetRelativeTransform(EvalSpaceCompSpace);
	}
	// If just evaluating in local space, just grab from local space pose
	else
	{
		SourceBoneTM = SourceData.Pose[SourceCompactIndex];
	}

	// Build RBFInput entry
	FRBFEntry Input;
	if (DriveSource == EPoseDriverSource::Translation)
	{
		Input.SetFromVector(SourceBoneTM.GetTranslation());
	}
	else
	{
		Input.SetFromRotator(SourceBoneTM.Rotator());
	}

	RBFParams.TargetDimensions = 3;

	// Get target array as RBF types
	TArray<FRBFTarget> RBFTargets;
	GetRBFTargets(RBFTargets);

	// Run RBF solver
	OutputWeights.Empty();
	FRBFSolver::Solve(RBFParams, RBFTargets, Input, OutputWeights);

	// Track if we have filled Output with valid pose
	bool bHaveValidPose = false;

	// Process active targets (if any)
	if (OutputWeights.Num() > 0)
	{
		// If we want to drive poses, and PoseAsset is assigned and compatible
		if (DriveOutput == EPoseDriverOutput::DrivePoses && 
			CurrentPoseAsset.IsValid() && 
			Output.AnimInstanceProxy->IsSkeletonCompatible(CurrentPoseAsset->GetSkeleton()) )
		{
			FPoseContext CurrentPose(Output);

			// Reset PoseExtractContext..
			check(PoseExtractContext.PoseCurves.Num() == PoseUIDList.Num());		
			FMemory::Memzero(PoseExtractContext.PoseCurves.GetData(), PoseExtractContext.PoseCurves.Num() * sizeof(float));
			// Then fill in weight for any driven poses
			for (const FRBFOutputWeight& Weight : OutputWeights)
			{
				const FPoseDriverTarget& PoseTarget = PoseTargets[Weight.TargetIndex];

				int32 PoseIndex = CurrentPoseAsset.Get()->GetPoseIndexByName(PoseTarget.DrivenName);
				if (PoseIndex != INDEX_NONE)
				{
					PoseExtractContext.PoseCurves[PoseIndex] = Weight.TargetWeight;
				}
			}

			if (CurrentPoseAsset.Get()->GetAnimationPose(CurrentPose.Pose, CurrentPose.Curve, PoseExtractContext))
			{
				// blend by weight
				if (CurrentPoseAsset->IsValidAdditive())
				{
					// Don't want to modify SourceBone, set additive offset to zero (not identity transform, as need zero scale)
					CurrentPose.Pose[SourceCompactIndex] = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);

					Output = SourceData;
					FAnimationRuntime::AccumulateAdditivePose(Output.Pose, CurrentPose.Pose, Output.Curve, CurrentPose.Curve, 1.f, EAdditiveAnimationType::AAT_LocalSpaceBase);
				}
				else
				{
					// Don't want to modify SourceBone, set weight to zero
					BoneBlendWeights[SourceCompactIndex.GetInt()] = 0.f;

					FAnimationRuntime::BlendTwoPosesTogetherPerBone(SourceData.Pose, CurrentPose.Pose, SourceData.Curve, CurrentPose.Curve, BoneBlendWeights, Output.Pose, Output.Curve);
				}

				bHaveValidPose = true;
			}
		}
		// Drive curves (morphs, materials etc)
		else if(DriveOutput == EPoseDriverOutput::DriveCurves)
		{
			// Start by copying input
			Output = SourceData;
			
			// Then set curves based on target weights
			for (const FRBFOutputWeight& Weight : OutputWeights)
			{
				FPoseDriverTarget& PoseTarget = PoseTargets[Weight.TargetIndex];
				if (PoseTarget.DrivenUID != SmartName::MaxUID)
				{
					Output.Curve.Set(PoseTarget.DrivenUID, Weight.TargetWeight);
				}
			}

			bHaveValidPose = true;
		}
	}


	// No valid pose, just pass through
	if(!bHaveValidPose)
	{
		Output = SourceData;
	}
}
