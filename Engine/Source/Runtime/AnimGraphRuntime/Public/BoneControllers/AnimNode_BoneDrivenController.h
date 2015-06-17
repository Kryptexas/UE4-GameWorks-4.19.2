// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "AnimNode_BoneDrivenController.generated.h"

// Evaluation of the bone transforms relies on the size and ordering of this
// enum, if this needs to change make sure EvaluateBoneTransforms is updated.
UENUM()
namespace EComponentType
{
	enum Type
	{
		None = 0,
		TranslationX,
		TranslationY,
		TranslationZ,
		RotationX,
		RotationY,
		RotationZ,
		Scale
	};
}

/**
 * This is the runtime version of a bone driven controller, which maps part of the state from one bone to another (e.g., 2 * source.x -> target.z)
 */
USTRUCT()
struct ANIMGRAPHRUNTIME_API FAnimNode_BoneDrivenController : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	// Bone to use as controller input
	UPROPERTY(EditAnywhere, Category="Source (driver)")
	FBoneReference SourceBone;

	// Transform component to use as input
	UPROPERTY(EditAnywhere, Category="Source (driver)")
	TEnumAsByte<EComponentType::Type> SourceComponent;

	/** Curve used to map from the source attribute to the driven attributes if present (otherwise the Multiplier will be used) */
	UPROPERTY(EditAnywhere, Category=Mapping)
	UCurveFloat* DrivingCurve;

	// Multiplier to apply to the input value
	UPROPERTY(EditAnywhere, Category=Mapping)
	float Multiplier;

	// Whether or not to use the range limits on the multiplied value
	UPROPERTY(EditAnywhere, Category=Mapping)
	bool bUseRange;

	// Minimum limit of the output value
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(EditCondition=bUseRange))
	float RangeMin;

	// Maximum limit of the output value
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(EditCondition=bUseRange))
	float RangeMax;

	// Bone to drive using controller input
	UPROPERTY(EditAnywhere, Category="Destination (driven)")
	FBoneReference TargetBone;

	// Transform component to write to
	UPROPERTY(EditAnywhere, Category="Destination (driven)")
	TEnumAsByte<EComponentType::Type> TargetComponent;

public:
	FAnimNode_BoneDrivenController();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

protected:
	
	// FAnimNode_SkeletalControlBase protected interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase protected interface
};