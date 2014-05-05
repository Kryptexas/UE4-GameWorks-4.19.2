// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_Slot.generated.h"

// An animation slot node normally acts as a passthru, but a montage or PlaySlotAnimation call from
// game code can cause an animation to blend in and be played on the slot temporarily, overriding the
// Source input.
USTRUCT()
struct ENGINE_API FAnimNode_Slot : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	// The source input, passed thru to the output unless a montage or slot animation is currently playing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink Source;

	// The name of this slot, exposed to gameplay code, etc...
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FName SlotName;

protected:
	/** Weight of Source Branch. This is the weight of the input pose coming from children.
	This is different than (1.f - SourceWeight) since the Slot can play additive animations, which are overlayed on top of the source pose. */
	float SourceWeight;

	/** Weight of Slot Node. Determined by Montages weight playing on this slot */
	float SlotNodeWeight;

public:	
	FAnimNode_Slot();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	virtual void GatherDebugData(FNodeDebugData& DebugData) OVERRIDE;
	// End of FAnimNode_Base interface
};
