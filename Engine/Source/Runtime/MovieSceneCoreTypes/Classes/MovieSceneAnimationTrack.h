// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneAnimationTrack.generated.h"


/**
 * Handles animation of skeletal mesh actors
 */
UCLASS( MinimalAPI )
class UMovieSceneAnimationTrack : public UMovieSceneTrack
{
	GENERATED_UCLASS_BODY()

public:
	/** UMovieSceneTrack interface */
	virtual FName GetTrackName() const OVERRIDE;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() OVERRIDE;
	virtual void RemoveAllAnimationData() OVERRIDE;
	virtual bool HasSection( UMovieSceneSection* Section ) const OVERRIDE;
	virtual void RemoveSection( UMovieSceneSection* Section ) OVERRIDE;
	virtual bool IsEmpty() const OVERRIDE;
	virtual TRange<float> GetSectionBoundaries() const OVERRIDE;
	virtual bool SupportsMultipleRows() const OVERRIDE { return true; }
	virtual TArray<UMovieSceneSection*> GetAllSections() const OVERRIDE;

	/** Adds a new animation to this track */
	virtual void AddNewAnimation(float KeyTime, class UAnimSequence* AnimSequence);

	/** Gets the animation section at a certain time, or NULL if there is none */
	class UMovieSceneSection* GetAnimSectionAtTime(float Time);

private:
	/** List of all animation sections */
	UPROPERTY()
	TArray<UMovieSceneSection*> AnimationSections;
};
