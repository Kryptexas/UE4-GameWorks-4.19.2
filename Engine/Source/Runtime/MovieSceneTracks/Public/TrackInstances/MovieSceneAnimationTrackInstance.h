// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneTrackInstance.h"


class UMovieSceneAnimationTrack;


/**
 * Instance of a UMovieSceneAnimationTrack
 */
class FMovieSceneAnimationTrackInstance
	: public IMovieSceneTrackInstance
{
public:

	FMovieSceneAnimationTrackInstance( UMovieSceneAnimationTrack& InAnimationTrack );
	virtual ~FMovieSceneAnimationTrackInstance();

	/** IMovieSceneTrackInstance interface */
	virtual void SaveState(const TArray<UObject*>& RuntimeObjects) override {}
	virtual void RestoreState(const TArray<UObject*>& RuntimeObjects) override {}
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override {}
	virtual void ClearInstance( IMovieScenePlayer& Player ) override {}

private:

	/** Track that is being instanced */
	UMovieSceneAnimationTrack* AnimationTrack;
};
