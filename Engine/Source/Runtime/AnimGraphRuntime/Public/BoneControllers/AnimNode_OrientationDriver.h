// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "AnimNode_OrientationDriver.generated.h"

/** One named parameter being driven by the orientation of a bone. */
USTRUCT()
struct FOrientationDriverParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Param)
	FName ParameterName;

	UPROPERTY(EditAnywhere, Category = Param)
	float ParameterValue;
};

/** One target pose for the bone, with parameters to drive bone approaches this orientation. */
USTRUCT()
struct FOrientationDriverPose
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Pose)
	FQuat PoseQuat;

	UPROPERTY(EditAnywhere, Category = Pose)
	TArray<FOrientationDriverParam> DrivenParams;

	/** Last weight calculated for this pose. */
	float PoseWeight;

	/** Last distance calculated from this pose */
	float PoseDistance;

	FOrientationDriverPose()
	: PoseQuat(FQuat::Identity)
	{}
};

/** Set of parameters being driven */
struct FOrientationDriverParamSet
{
	/** Array of parameters that makes up a set */
	TArray<FOrientationDriverParam> Params;

	/** Increase value of a named parameter in a set. */
	void AddParam(const FOrientationDriverParam& InParam, float InScale);

	/** Add a set of params to another set */
	void AddParams(const TArray<FOrientationDriverParam>& InParams, float InScale);

	/** Scale all params in a set */
	void ScaleAllParams(float InScale);

	/** Clear all params */
	void ClearParams();
};

/** RBF based orientation driver */
USTRUCT()
struct ANIMGRAPHRUNTIME_API FAnimNode_OrientationDriver : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	/** Bone to use for driving parameters based on its orientation */
	UPROPERTY(EditAnywhere, Category = OrientationDriver)
	FBoneReference SourceBone;

	/** Set of target poses, which include parameters to drive. */
	UPROPERTY(EditAnywhere, Category = OrientationDriver)
	TArray<FOrientationDriverPose> Poses;

	/** Scaling of radial basis, applied to max distance between poses */
	UPROPERTY(EditAnywhere, Category = OrientationDriver, meta = (ClampMin = "0.01"))
	float RadialScaling;

	/** Previous set of interpolated params, kept for debugging */
	FOrientationDriverParamSet ResultParamSet;

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	FAnimNode_OrientationDriver();

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context);
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	/** Find max distance between any of the poses */
	void UpdateMaxDistanceBetweenPoses();

protected:

	/** Is MaxDistanceBetweenPoses up to date */
	bool bMaxDistanceUpToDate;

	/** Maximum angular distance between poses in the Poses array */
	float MaxDistanceBetweenPoses;

	// FAnimNode_SkeletalControlBase protected interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase protected interface
};