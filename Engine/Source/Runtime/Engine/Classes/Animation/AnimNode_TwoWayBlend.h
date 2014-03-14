// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InputScaleBias.h"
#include "AnimNode_TwoWayBlend.generated.h"

// This represents a baked transition
USTRUCT()
struct ENGINE_API FAnimationNode_TwoWayBlend : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(PinShownByDefault))
	mutable float Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FInputScaleBias AlphaScaleBias;

public:
	FAnimationNode_TwoWayBlend()
		: Alpha(0.0f)
	{
	}

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface
};

