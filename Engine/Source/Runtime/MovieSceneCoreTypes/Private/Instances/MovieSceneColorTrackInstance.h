// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a UMovieSceneColorTrack
 */
class FMovieSceneColorTrackInstance : public IMovieSceneTrackInstance
{
public:
	FMovieSceneColorTrackInstance( UMovieSceneColorTrack& InColorTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void SaveState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void RestoreState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) override;
private:
	/** The track being instanced */
	UMovieSceneColorTrack* ColorTrack;
	/** Runtime property bindings */
	TSharedPtr<class FTrackInstancePropertyBindings> PropertyBindings;
	/** Map from object to initial state */
	TMap< TWeakObjectPtr<UObject>, FSlateColor > InitSlateColorMap;
	TMap< TWeakObjectPtr<UObject>, FLinearColor > InitLinearColorMap;
};
