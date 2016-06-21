// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
#include "BoneControllers/AnimNode_OrientationDriver.h"

#include "AnimInstanceProxy.h"

//////////////////////////////////////////////////////////////////////////

void FOrientationDriverParamSet::AddParam(const FOrientationDriverParam& InParam, float InScale)
{
	bool bFoundParam = false;

	// Look to see if this param already exists in this set
	for (FOrientationDriverParam& Param : Params)
	{
		if (Param.ParameterName == InParam.ParameterName)
		{
			Param.ParameterValue += (InParam.ParameterValue * InScale);
			bFoundParam = true;
		}
	}

	// If not found, add new entry and set to supplied value
	if (!bFoundParam)
	{
		int32 NewParamIndex = Params.AddZeroed();
		FOrientationDriverParam& Param = Params[NewParamIndex];
		Param.ParameterName = InParam.ParameterName;
		Param.ParameterValue = (InParam.ParameterValue * InScale);
	}
}

void FOrientationDriverParamSet::AddParams(const TArray<FOrientationDriverParam>& InParams, float InScale)
{
	for (const FOrientationDriverParam& InParam : InParams)
	{
		AddParam(InParam, InScale);
	}
}

void FOrientationDriverParamSet::ScaleAllParams(float InScale)
{
	for (FOrientationDriverParam& Param : Params)
	{
		Param.ParameterValue *= InScale;
	}
}

void FOrientationDriverParamSet::ClearParams()
{
	Params.Empty();
}


//////////////////////////////////////////////////////////////////////////

FAnimNode_OrientationDriver::FAnimNode_OrientationDriver()
{
	RadialScaling = 0.25f;
}

void FAnimNode_OrientationDriver::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_OrientationDriver::EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutCSBoneTransforms)
{

}



void FAnimNode_OrientationDriver::UpdateMaxDistanceBetweenPoses()
{
	// Find largest distance between poses
	MaxDistanceBetweenPoses = 0.f;

	for (int32 I = 1; I < Poses.Num(); I++)
	{
		FQuat QuatI = Poses[I].PoseQuat;
		for (int32 J = 0; J < I; J++)
		{
			FQuat QuatJ = Poses[J].PoseQuat;

			float Dist = QuatI.AngularDistance(QuatJ);

			MaxDistanceBetweenPoses = FMath::Max(Dist, MaxDistanceBetweenPoses);
		}
	}

	bMaxDistanceUpToDate = true;
}

void FAnimNode_OrientationDriver::EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context)
{
	// Do nothing if no poses
	if (Poses.Num() == 0)
	{
		return;
	}

	// Ensure max distance up to date
	if (!bMaxDistanceUpToDate)
	{
		UpdateMaxDistanceBetweenPoses();
	}

	const FBoneContainer& BoneContainer = Context.Pose.GetPose().GetBoneContainer();

	// Final set of driven params, from all interpolators
	ResultParamSet.ClearParams();

	// Get the current rotation of the source bone
	const FTransform SourceCurrentBoneTransform = Context.Pose.GetLocalSpaceTransform(SourceBone.GetCompactPoseIndex(BoneContainer));
	const FQuat BoneQuat = SourceCurrentBoneTransform.GetRotation();

	// Array of 'weights' of current orientation to each pose
	float TotalWeight = 0.f;

	float GaussVarianceSqr = FMath::Square(RadialScaling * MaxDistanceBetweenPoses);

	// Iterate over each pose, adding its contribution
	for (FOrientationDriverPose& Pose : Poses)
	{
		// Find distance
		Pose.PoseDistance = BoneQuat.AngularDistance(Pose.PoseQuat);

		// Evaluate radial basis function to find weight
		Pose.PoseWeight = FMath::Exp(-(Pose.PoseDistance * Pose.PoseDistance) / GaussVarianceSqr);

		// Add weight to total
		TotalWeight += Pose.PoseWeight;

		// Add params for this pose, weighted by pose weight
		ResultParamSet.AddParams(Pose.DrivenParams, Pose.PoseWeight);
	}

	// Only normalize and apply if we got some kind of weight
	if (TotalWeight > KINDA_SMALL_NUMBER)
	{
		// Normalize params by rescaling by sum of weights
		const float WeightScale = 1.f / TotalWeight;
		ResultParamSet.ScaleAllParams(WeightScale);

		// Also normalize each pose weight
		for (FOrientationDriverPose& Pose : Poses)
		{
			Pose.PoseWeight *= WeightScale;
		}

		//	Morph target and Material parameter curves
		USkeleton* Skeleton = Context.AnimInstanceProxy->GetSkeleton();

		for (FOrientationDriverParam& Param : ResultParamSet.Params)
		{
			FSmartNameMapping::UID NameUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, Param.ParameterName);
			if (NameUID != FSmartNameMapping::MaxUID)
			{
				Context.Curve.Set(NameUID, Param.ParameterValue, EAnimCurveFlags::ACF_DrivesMorphTarget | EAnimCurveFlags::ACF_DrivesMaterial | EAnimCurveFlags::ACF_DrivesPose);
			}
		}
	}
}

bool FAnimNode_OrientationDriver::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// return true if at least one bone ref is valid
	return SourceBone.IsValid(RequiredBones);
}

void FAnimNode_OrientationDriver::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	// Init bone ref
	SourceBone.Initialize(RequiredBones);
}