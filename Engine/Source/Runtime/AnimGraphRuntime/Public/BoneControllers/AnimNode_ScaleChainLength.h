// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneIndices.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_ScaleChainLength.generated.h"

class USkeletalMeshComponent;

/**
 *	
 */
USTRUCT(BlueprintInternalUseOnly)
struct ANIMGRAPHRUNTIME_API FAnimNode_ScaleChainLength : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Links)
	FPoseLink InputPose;

	/** Default chain length, as animated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ScaleChainLength, meta = (PinHiddenByDefault))
	float DefaultChainLength;

	UPROPERTY(EditAnywhere, Category = ScaleChainLength)
	FBoneReference ChainStartBone;

	UPROPERTY(EditAnywhere, Category = ScaleChainLength)
	FBoneReference ChainEndBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ScaleChainLength, meta = (PinShownByDefault))
	FVector TargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	mutable float Alpha;

	UPROPERTY(EditAnywhere, Category = Settings)
	FInputScaleBias AlphaScaleBias;

	UPROPERTY(Transient)
	bool bBoneIndicesCached;

	TArray<FCompactPoseBoneIndex> ChainBoneIndices;

public:
	FAnimNode_ScaleChainLength();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) override;
	virtual void Update(const FAnimationUpdateContext& Context) override;
	virtual void CacheBones(const FAnimationCacheBonesContext& Context) override;
	virtual void Evaluate(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
};
