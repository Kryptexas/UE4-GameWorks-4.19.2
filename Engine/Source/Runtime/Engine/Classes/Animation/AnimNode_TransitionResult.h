// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_TransitionResult.generated.h"

// Root node of a state machine transition graph
USTRUCT()
struct ENGINE_API FAnimNode_TransitionResult : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Result, meta=(AlwaysAsPin))
	bool bCanEnterTransition;

public:	
	FAnimNode_TransitionResult();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface
};
