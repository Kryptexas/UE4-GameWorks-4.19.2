// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneTrackInstance.h"


class UMovieSceneShotTrack;


/**
 * Instance of a UMovieSceneShotTrack
 */
class FMovieSceneShotTrackInstance
	: public IMovieSceneTrackInstance
{
public:

	FMovieSceneShotTrackInstance( UMovieSceneShotTrack& InDirectorTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player  ) override {}

private:

	/** Track that is being instanced */
	UMovieSceneShotTrack* DirectorTrack;
};
