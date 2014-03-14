// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNodeBase.h"
#include "AnimNodeSpaceConversions.generated.h"

USTRUCT()
struct ENGINE_API FAnimNode_ConvertComponentToLocalSpace : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FComponentSpacePoseLink ComponentPose;

public:
	FAnimNode_ConvertComponentToLocalSpace();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface

};

USTRUCT()
struct ENGINE_API FAnimNode_ConvertLocalToComponentSpace : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink LocalPose;

public:
	FAnimNode_ConvertLocalToComponentSpace();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void EvaluateComponentSpace(FComponentSpacePoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface
};
