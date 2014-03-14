// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimNode_SequencePlayer.generated.h"

#pragma once

// Sequence player node
USTRUCT()
struct ENGINE_API FAnimNode_SequencePlayer : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()
public:
	// The animation sequence asset to play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(PinHiddenByDefault))
	UAnimSequenceBase* Sequence;

	// The internal time accumulator (position within the animation asset being played)
	UPROPERTY(BlueprintReadWrite, Transient, Category=DoNotEdit)
	float InternalTimeAccumulator;

	// Should the animation continue looping when it reaches the end?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(DefaultValue="true", PinHiddenByDefault))
	mutable bool bLoopAnimation;

	// The play rate multiplier. Can be negative, which will cause the animation to play in reverse.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(DefaultValue="1.0", PinHiddenByDefault))
	mutable float PlayRate;

	// The group index, assigned at compile time based on the editoronly GroupName (or INDEX_NONE if it is not part of any group)
	UPROPERTY()
	int32 GroupIndex;

	// The role this player can assume within the group (ignored if GroupIndex is INDEX_NONE)
	UPROPERTY()
	TEnumAsByte<EAnimGroupRole::Type> GroupRole;
public:	
	FAnimNode_SequencePlayer()
		: Sequence(NULL)
		, InternalTimeAccumulator(0.0f)
		, bLoopAnimation(true)
		, PlayRate(1.0f)
		, GroupIndex(INDEX_NONE)
	{
	}

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface
};
