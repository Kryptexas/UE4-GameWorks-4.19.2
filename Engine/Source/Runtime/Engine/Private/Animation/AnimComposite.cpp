// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimComposite.cpp: Composite classes that contains sequence for each section
=============================================================================*/ 

#include "EnginePrivate.h"
#include "Animation/AnimComposite.h"
#include "AnimationUtils.h"
#include "AnimationRuntime.h"
#include "AnimNotifyQueue.h"

UAnimComposite::UAnimComposite(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

#if WITH_EDITOR
bool UAnimComposite::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets)
{
	return AnimationTrack.GetAllAnimationSequencesReferred(AnimationAssets);
}

void UAnimComposite::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap)
{
	AnimationTrack.ReplaceReferredAnimations(ReplacementMap);
}
#endif

void UAnimComposite::HandleAssetPlayerTickedInternal(FAnimAssetTickContext &Context, const float PreviousTime, const float MoveDelta, const FAnimTickRecord &Instance, struct FAnimNotifyQueue& NotifyQueue) const
{
	Super::HandleAssetPlayerTickedInternal(Context, PreviousTime, MoveDelta, Instance, NotifyQueue);

	// if should generate notifies
	if (Context.ShouldGenerateNotifies())
	{
		// make sure to check animation track notifies
		TArray<const FAnimNotifyEvent*> AnimNotifies;
		const bool bMovingForward = (RateScale >= 0.f);

		if (bMovingForward)
		{
			const float CurrentTime = *Instance.TimeAccumulator;
			if (PreviousTime <= CurrentTime)
			{
				AnimationTrack.GetAnimNotifiesFromTrackPositions(PreviousTime, CurrentTime, AnimNotifies);
			}
			else
			{
				AnimationTrack.GetAnimNotifiesFromTrackPositions(PreviousTime, SequenceLength, AnimNotifies);
				AnimationTrack.GetAnimNotifiesFromTrackPositions(0.f, CurrentTime, AnimNotifies);
			}
		}
		else
		{
			const float CurrentTime = *Instance.TimeAccumulator;
			if (PreviousTime >= CurrentTime)
			{
				AnimationTrack.GetAnimNotifiesFromTrackPositions(PreviousTime, CurrentTime, AnimNotifies);
			}
			else
			{
				AnimationTrack.GetAnimNotifiesFromTrackPositions(PreviousTime, 0.f, AnimNotifies);
				AnimationTrack.GetAnimNotifiesFromTrackPositions(SequenceLength, CurrentTime, AnimNotifies);
			}
		}

		NotifyQueue.AddAnimNotifies(AnimNotifies, Instance.EffectiveBlendWeight);
	}
	ExtractRootMotionFromTrack(AnimationTrack, PreviousTime, PreviousTime + MoveDelta, Context.RootMotionMovementParams);
}

void UAnimComposite::GetAnimationPose(FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const
{
	AnimationTrack.GetAnimationPose(OutPose, OutCurve, ExtractionContext);

	FBlendedCurve CompositeCurve;
	CompositeCurve.InitFrom(OutCurve);
	EvaluateCurveData(CompositeCurve, ExtractionContext.CurrentTime);

	// combine both curve
	OutCurve.Combine(CompositeCurve);
}

EAdditiveAnimationType UAnimComposite::GetAdditiveAnimType() const
{
	int32 AdditiveType = AnimationTrack.GetTrackAdditiveType();

	if (AdditiveType != -1)
	{
		return (EAdditiveAnimationType)AdditiveType;
	}

	return AAT_None;
}

void UAnimComposite::EnableRootMotionSettingFromMontage(bool bInEnableRootMotion, const ERootMotionRootLock::Type InRootMotionRootLock)
{
	AnimationTrack.EnableRootMotionSettingFromMontage(bInEnableRootMotion, InRootMotionRootLock);
}

bool UAnimComposite::HasRootMotion() const
{
	return AnimationTrack.HasRootMotion();
}

#if WITH_EDITOR
class UAnimSequence* UAnimComposite::GetAdditiveBasePose() const
{
	// @todo : for now it just picks up the first sequence
	return AnimationTrack.GetAdditiveBasePose();
}
#endif 

void UAnimComposite::InvalidateRecursiveAsset()
{
	// unfortunately we'll have to do this all the time
	// we don't know whether or not the nested assets are modified or not
	AnimationTrack.InvalidateRecursiveAsset(this);
}

bool UAnimComposite::ContainRecursive(TArray<UAnimCompositeBase*>& CurrentAccumulatedList)
{
	// am I included already?
	if (CurrentAccumulatedList.Contains(this))
	{
		return true;
	}

	// otherwise, add myself to it
	CurrentAccumulatedList.Add(this);

	// otherwise send to animation track
	if (AnimationTrack.ContainRecursive(CurrentAccumulatedList))
	{
		return true;
	}

	return false;
}