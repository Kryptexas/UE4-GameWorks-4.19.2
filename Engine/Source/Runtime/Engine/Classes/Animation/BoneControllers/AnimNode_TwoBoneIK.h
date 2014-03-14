// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "AnimNode_TwoBoneIK.generated.h"

/**
 * Simple 2 Bone IK Controller.
 */

USTRUCT()
struct ENGINE_API FAnimNode_TwoBoneIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()
	
	/** Name of bone to control. This is the main bone chain to modify from. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IK)
	FBoneReference IKBone;

	/** Effector Location. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=EndEffector, meta=(PinShownByDefault))
	FVector EffectorLocation;

	/** Effector Location. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=JointTarget, meta=(PinShownByDefault))
	FVector JointTargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IK)
	FVector2D StretchLimits;

	/** If EffectorLocationSpace is a bone, this is the bone to use. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=EndEffector)
	FName EffectorSpaceBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=EndEffector)
	uint32 bTakeRotationFromEffectorSpace:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=EndEffector)
	uint32 bMaintainEffectorRelRot:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IK)
	uint32 bAllowStretching:1;
	
	/** Reference frame of Effector Location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=EndEffector)
	TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace;

	/** Reference frame of Effector Location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=JointTarget)
	TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace;

	/** If EffectorLocationSpace is a bone, this is the bone to use. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=JointTarget)
	FName JointTargetSpaceBoneName;

	FAnimNode_TwoBoneIK();

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, const FBoneContainer & RequiredBones, FA2CSPose& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) OVERRIDE;
	virtual bool IsValidToEvaluate(const USkeleton * Skeleton, const FBoneContainer & RequiredBones) OVERRIDE;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer & RequiredBones) OVERRIDE;
	// End of FAnimNode_SkeletalControlBase interface
};