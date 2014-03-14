// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Abstract base class of animation made of multiple sequences.
 *
 */

#pragma once
#include "AnimComposite.generated.h"

UCLASS(config=Engine, hidecategories=UObject, MinimalAPI, BlueprintType)
class UAnimComposite : public UAnimCompositeBase
{
	GENERATED_UCLASS_BODY()

public:
	/** Serializable data that stores section/anim pairing **/
	UPROPERTY()
	struct FAnimTrack AnimationTrack;

#if WITH_EDITOR
	virtual bool GetAllAnimationSequencesReferred(TArray<UAnimSequence*>& AnimationSequences) OVERRIDE;
	virtual void ReplaceReferredAnimations(const TMap<UAnimSequence*, UAnimSequence*>& ReplacementMap) OVERRIDE;
#endif
};

