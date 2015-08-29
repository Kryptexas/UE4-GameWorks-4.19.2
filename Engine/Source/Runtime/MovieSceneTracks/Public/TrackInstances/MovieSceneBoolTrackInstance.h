// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneTrackInstance.h"


class FTrackInstancePropertyBindings;
class UMovieSceneBoolTrack;


/**
 * Instance of a UMovieSceneBoolTrack
 */
class FMovieSceneBoolTrackInstance
	: public IMovieSceneTrackInstance
{
public:

	FMovieSceneBoolTrackInstance( UMovieSceneBoolTrack& InBoolTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void SaveState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void RestoreState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void ClearInstance( IMovieScenePlayer& Player ) override {}

private:

	/** Bool track that is being instanced */
	UMovieSceneBoolTrack* BoolTrack;

	/** Runtime property bindings */
	TSharedPtr<FTrackInstancePropertyBindings> PropertyBindings;

	/** Map from object to initial state */
	TMap<TWeakObjectPtr<UObject>, bool> InitBoolMap;
};
