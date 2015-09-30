// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSection.h"
#include "MovieSceneSkeletalAnimationSection.generated.h"

/**
 * Audio section, for use in the master audio, or by attached audio objects
 */
UCLASS( MinimalAPI )
class UMovieSceneSkeletalAnimationSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:
	/** Sets the animation sequence for this section */
	void SetAnimSequence(class UAnimSequence* InAnimSequence) {AnimSequence = InAnimSequence;}
	
	/** Gets the animation sequence for this section */
	class UAnimSequence* GetAnimSequence() {return AnimSequence;}
	
	/** Sets the time that the animation is supposed to be played at */
	void SetAnimationStartTime(float InAnimationStartTime) {AnimationStartTime = InAnimationStartTime;}
	
	/** Gets the (absolute) time that the animation is supposed to be played at */
	float GetAnimationStartTime() const {return AnimationStartTime;}
	
	/** Gets the animation duration, modified by dilation */
	float GetAnimationDuration() const {return FMath::IsNearlyZero(AnimationDilationFactor) ? 0.f : AnimSequence->SequenceLength / AnimationDilationFactor;}

	/** Gets the animation sequence length, not modified by dilation */
	float GetAnimationSequenceLength() const {return AnimSequence->SequenceLength;}

	/**
	 * @return The dilation multiplier of the animation
	 */
	float GetAnimationDilationFactor() const {return AnimationDilationFactor;}

	/** MovieSceneSection interface */
	virtual void MoveSection( float DeltaPosition, TSet<FKeyHandle>& KeyHandles ) override;
	virtual void DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles  ) override;
	virtual void GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const override;
	virtual void GetSnapTimes(TArray<float>& OutSnapTimes, bool bGetSectionBorders) const override;

private:
	/** The animation sequence this section has */
	UPROPERTY(EditAnywhere, Category="Animation")
	class UAnimSequence* AnimSequence;

	/** The absolute time that the animation starts playing at */
	UPROPERTY(EditAnywhere, Category="Animation")
	float AnimationStartTime;
	
	/** The amount which this animation is time dilated by */
	UPROPERTY(EditAnywhere, Category="Animation")
	float AnimationDilationFactor;
};
