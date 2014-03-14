// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a USubMovieSceneTrack
 */
class FSubMovieSceneTrackInstance : public IMovieSceneTrackInstance
{
public:
	FSubMovieSceneTrackInstance( USubMovieSceneTrack& InTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) OVERRIDE;
	virtual void RefreshInstance( class IMovieScenePlayer& Player ) OVERRIDE;
private:
	/** Track that is being instanced */
	TWeakObjectPtr<class USubMovieSceneTrack> SubMovieSceneTrack;
	/** Mapping of section lookups to instances.  Each section has a movie scene which must be instanced */
	TMap< TWeakObjectPtr<USubMovieSceneSection>, TSharedPtr<class FMovieSceneInstance> > SubMovieSceneInstances; 
};
