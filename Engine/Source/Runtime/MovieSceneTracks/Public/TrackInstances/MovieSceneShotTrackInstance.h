// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SubMovieSceneTrackInstance.h"

class UMovieSceneShotTrack;


/**
 * Instance of a UMovieSceneShotTrack
 */
class FMovieSceneShotTrackInstance
	: public FSubMovieSceneTrackInstance
{
public:

	FMovieSceneShotTrackInstance( UMovieSceneShotTrack& InShotTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void ClearInstance( IMovieScenePlayer& Player ) override;
private:
	/** Runtime camera objects.  One for each shot.  Must be the same number of entries as sections */
	TArray< TWeakObjectPtr<UObject> > RuntimeCameraObjects;

	/** Current camera object we are looking through.  Used to determine when making a new cut */
	TWeakObjectPtr<UObject> CurrentCameraObject;
};
