// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a UMovieSceneVectorTrack
 */
class FMovieSceneVectorTrackInstance : public IMovieSceneTrackInstance
{
public:
	FMovieSceneVectorTrackInstance( UMovieSceneVectorTrack& InVectorTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) OVERRIDE;
	virtual void RefreshInstance( IMovieScenePlayer& Player ) OVERRIDE {}
private:
	/** Track that is being instanced */
	UMovieSceneVectorTrack* VectorTrack;
};
