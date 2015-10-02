// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneTrackInstance.h"


class UMovieSceneSkeletalAnimationTrack;


/**
 * Instance of a UMovieSceneSkeletalAnimationTrack
 */
class FMovieSceneSkeletalAnimationTrackInstance
	: public IMovieSceneTrackInstance
{
public:

	FMovieSceneSkeletalAnimationTrackInstance( UMovieSceneSkeletalAnimationTrack& InAnimationTrack );
	virtual ~FMovieSceneSkeletalAnimationTrackInstance();

	/** IMovieSceneTrackInstance interface */
	virtual void SaveState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) override {}
	virtual void RestoreState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) override {}
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override {}
	virtual void ClearInstance( IMovieScenePlayer& Player ) override {}

private:

	/** Track that is being instanced */
	UMovieSceneSkeletalAnimationTrack* AnimationTrack;
};
