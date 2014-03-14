// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SaveCachedPose.generated.h"

USTRUCT()
struct ENGINE_API FAnimNode_SaveCachedPose : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(transient)
	int16 LastInitializedContextCounter;

	UPROPERTY(transient)
	int16 LastCacheBonesContextCounter;

	UPROPERTY(transient)
	int16 LastUpdatedContextCounter;

	UPROPERTY(transient)
	int16 LastEvaluatedContextCounter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink Pose;

protected:
	FA2Pose CachedPose;
public:	
	FAnimNode_SaveCachedPose();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface
};
