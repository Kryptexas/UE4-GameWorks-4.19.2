// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSubTrackInstance.h"

class UMovieSceneShotTrack;


/**
 * Instance of a UMovieSceneShotTrack
 */
class FMovieSceneShotTrackInstance
	: public IMovieSceneTrackInstance
{
public:

	FMovieSceneShotTrackInstance(UMovieSceneShotTrack& InShotTrack);

public:

	// IMovieSceneTrackInstance interface

	virtual void ClearInstance(IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance) override;
	virtual void RefreshInstance(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance) override;
	virtual void RestoreState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance) override { }
	virtual void SaveState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance) override { }
	virtual void Update(float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance) override;

private:
	
	/** Runtime camera objects.  One for each shot.  Must be the same number of entries as sections */
	TArray< TWeakObjectPtr<UObject> > RuntimeCameraObjects;

	/** Current camera object we are looking through.  Used to determine when making a new cut */
	TWeakObjectPtr<UObject> CurrentCameraObject;

	/** Track that is being instanced */
	TWeakObjectPtr<UMovieSceneShotTrack> ShotTrack;
};
