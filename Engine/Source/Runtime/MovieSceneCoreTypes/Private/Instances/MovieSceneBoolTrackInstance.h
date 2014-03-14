// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a UMovieSceneBoolTrack
 */
class FMovieSceneBoolTrackInstance : public IMovieSceneTrackInstance
{
public:
	FMovieSceneBoolTrackInstance( UMovieSceneBoolTrack& InBoolTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) OVERRIDE;
	virtual void RefreshInstance( class IMovieScenePlayer& Player ) OVERRIDE {}
private:
	/** Bool track that is being instanced */
	UMovieSceneBoolTrack* BoolTrack;
};
