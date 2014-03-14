// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a UMovieSceneDirectorTrack
 */
class FMovieSceneDirectorTrackInstance : public IMovieSceneTrackInstance
{
public:
	FMovieSceneDirectorTrackInstance( UMovieSceneDirectorTrack& InDirectorTrack );
	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) OVERRIDE;
	virtual void RefreshInstance( class IMovieScenePlayer& Player ) OVERRIDE {}
private:
	/** Track that is being instanced */
	UMovieSceneDirectorTrack* DirectorTrack;
};
