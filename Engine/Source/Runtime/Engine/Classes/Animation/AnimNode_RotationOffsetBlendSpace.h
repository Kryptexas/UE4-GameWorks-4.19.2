// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_BlendSpacePlayer.h"
#include "AnimNode_RotationOffsetBlendSpace.generated.h"

//@TODO: Comment
USTRUCT()
struct ENGINE_API FAnimNode_RotationOffsetBlendSpace : public FAnimNode_BlendSpacePlayer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink BasePose;

public:	
	FAnimNode_RotationOffsetBlendSpace();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface
};
