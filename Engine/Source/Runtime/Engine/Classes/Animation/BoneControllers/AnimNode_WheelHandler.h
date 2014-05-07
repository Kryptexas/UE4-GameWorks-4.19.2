// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "AnimNode_WheelHandler.generated.h"

struct FWheelSimulator
{
	int32					WheelIndex;
	FBoneReference			BoneReference;
	FRotator				RotOffset;
	FVector					LocOffset;
};

/**
 *	Simple controller that replaces or adds to the translation/rotation of a single bone.
 */
USTRUCT()
struct ENGINE_API FAnimNode_WheelHandler : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Current Asset being played **/
	UPROPERTY(transient)
	class UWheeledVehicleMovementComponent* VehicleSimComponent;

	TArray<FWheelSimulator>			WheelSimulators;

	FAnimNode_WheelHandler();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) OVERRIDE;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, const FBoneContainer & RequiredBones, FA2CSPose& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) OVERRIDE;
	virtual bool IsValidToEvaluate(const USkeleton * Skeleton, const FBoneContainer & RequiredBones) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer & RequiredBones) OVERRIDE;
	// End of FAnimNode_SkeletalControlBase interface
};
