// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "AnimNode_ControlRig.generated.h"

class UControlRig;

/**
 * Animation node that allows animation ControlRig output to be used in an animation graph
 */
USTRUCT()
struct CONTROLRIG_API FAnimNode_ControlRig : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	FAnimNode_ControlRig();

	void SetControlRig(UControlRig* InControlRig);
	UControlRig* GetControlRig() const;

	// FAnimNode_Base interface
	virtual void RootInitialize(const FAnimInstanceProxy* InProxy) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	// FAnimNode_SkeletalControlBase interface
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

private:
	/** Cached ControlRig */
	TWeakObjectPtr<UControlRig> CachedControlRig;
	TArray<FName> NodeNames;
};
