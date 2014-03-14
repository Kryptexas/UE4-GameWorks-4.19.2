// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a UMovieSceneColorTrack
 */
class FMovieSceneColorTrackInstance : public IMovieSceneTrackInstance
{
public:
	FMovieSceneColorTrackInstance( UMovieSceneColorTrack& InColorTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) OVERRIDE;
	virtual void RefreshInstance( class IMovieScenePlayer& Player ) OVERRIDE {}
private:
	/** The track being instanced */
	UMovieSceneColorTrack* ColorTrack;
};
