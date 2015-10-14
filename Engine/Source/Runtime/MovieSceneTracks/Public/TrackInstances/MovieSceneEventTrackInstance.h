// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneTrackInstance.h"


class UMovieSceneEventTrack;


/**
 * Instance of a UMovieSceneEventTrack
 */
class FMovieSceneEventTrackInstance
	: public IMovieSceneTrackInstance
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InEventTrack The event track to create an instance for.
	 */
	FMovieSceneEventTrackInstance(UMovieSceneEventTrack& InEventTrack );

	/** Virtual destructor. */
	virtual ~FMovieSceneEventTrackInstance() { }

public:

	// IMovieSceneTrackInstance interface

	virtual void ClearInstance(IMovieScenePlayer& Player) override { }
	virtual void RefreshInstance(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) override { }
	virtual void RestoreState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) override { }
	virtual void SaveState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) override { }
	virtual void Update(float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) override;

private:

	/** Track that is being instanced */
	UMovieSceneEventTrack* EventTrack;
};
