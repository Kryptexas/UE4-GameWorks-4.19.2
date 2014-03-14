// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_UseCachedPose.generated.h"

USTRUCT()
struct ENGINE_API FAnimNode_UseCachedPose : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	// Note: This link is intentionally not public; it's wired up during compilation
	UPROPERTY()
	FPoseLink LinkToCachingNode;
public:	
	FAnimNode_UseCachedPose();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface
};
