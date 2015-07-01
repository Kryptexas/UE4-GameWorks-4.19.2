// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieSceneTrackInstance.h"

class UMovieScene3DPathTrack;

/**
 * Instance of a UMovieScene3DPathTrack
 */
class FMovieScene3DPathTrackInstance
	: public IMovieSceneTrackInstance
{
public:
	FMovieScene3DPathTrackInstance( UMovieScene3DPathTrack& InPathTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void SaveState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void RestoreState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player ) override {}
	virtual void ClearInstance( IMovieScenePlayer& Player ) override {}
	virtual float EvalOrder() { return 1.f; }

private:
	/** Track that is being instanced */
	UMovieScene3DPathTrack* PathTrack;

	/** Map from object to initial state */
	TMap< TWeakObjectPtr<UObject>, FTransform > InitTransformMap;
};
