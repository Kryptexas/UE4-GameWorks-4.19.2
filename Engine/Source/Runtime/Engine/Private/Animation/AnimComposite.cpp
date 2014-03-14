// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimComposite.cpp: Composite classes that contains sequence for each section
=============================================================================*/ 

#include "EnginePrivate.h"
#include "AnimationUtils.h"
#include "AnimationRuntime.h"

UAnimComposite::UAnimComposite(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

#if WITH_EDITOR
bool UAnimComposite::GetAllAnimationSequencesReferred(TArray<UAnimSequence*>& AnimationSequences) 
{
	return AnimationTrack.GetAllAnimationSequencesReferred(AnimationSequences);
}

void UAnimComposite::ReplaceReferredAnimations(const TMap<UAnimSequence*, UAnimSequence*>& ReplacementMap)
{
	AnimationTrack.ReplaceReferredAnimations(ReplacementMap);
}
#endif