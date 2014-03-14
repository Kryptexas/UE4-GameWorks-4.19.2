// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *	Abstract base class for a skeletal controller.
 *	A SkelControl is a module that can modify the position or orientation of a set of bones in a skeletal mesh in some programmatic way.
 */

#pragma once

#include "../AnimNodeBase.h"
#include "../InputScaleBias.h"
#include "AnimNode_SkeletalControlBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSkeletalControl, Warning, All);

USTRUCT()
struct ENGINE_API FAnimNode_SkeletalControlBase : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	// Input link
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FComponentSpacePoseLink ComponentPose;

	// Current strength of the skeletal control
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(PinShownByDefault))
	mutable float Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FInputScaleBias AlphaScaleBias;

public:

	FAnimNode_SkeletalControlBase()
		: Alpha(1.0f)
	{
	}

public:
	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context)  OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void EvaluateComponentSpace(FComponentSpacePoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface
	
protected:
	// Interface for derived skeletal controls to implement

	// Evaluate the new component-space transforms for the affected bones.
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, const FBoneContainer & RequiredBones, FA2CSPose& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) {}
	// return true if it is valid to Evaluate
	virtual bool IsValidToEvaluate(const USkeleton * Skeleton, const FBoneContainer & RequiredBones) { return false; }
	// initialize any bone references you have
	virtual void InitializeBoneReferences(const FBoneContainer & RequiredBones){};
};
