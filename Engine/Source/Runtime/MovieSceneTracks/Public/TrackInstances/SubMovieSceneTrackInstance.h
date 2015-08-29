// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneTrackInstance.h"


class FMovieSceneInstance;
class USubMovieSceneSection;
class USubMovieSceneTrack;


/**
 * Instance of a USubMovieSceneTrack
 */
class FSubMovieSceneTrackInstance
	: public IMovieSceneTrackInstance
{
public:

	FSubMovieSceneTrackInstance( USubMovieSceneTrack& InTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void SaveState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void RestoreState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void ClearInstance( IMovieScenePlayer& Player ) override;
protected:

	/** Track that is being instanced */
	TWeakObjectPtr<USubMovieSceneTrack> SubMovieSceneTrack;

	/** Mapping of section lookups to instances.  Each section has a movie scene which must be instanced */
	TMap< TWeakObjectPtr<USubMovieSceneSection>, TSharedPtr<FMovieSceneInstance> > SubMovieSceneInstances; 
};
