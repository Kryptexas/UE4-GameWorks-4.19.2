// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a UMovieSceneAnimationTrack
 */
class FMovieSceneAnimationTrackInstance : public IMovieSceneTrackInstance
{
public:
	FMovieSceneAnimationTrackInstance( UMovieSceneAnimationTrack& InAnimationTrack );
	virtual ~FMovieSceneAnimationTrackInstance();

	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) override {}

private:
	/** Track that is being instanced */
	UMovieSceneAnimationTrack* AnimationTrack;
};
