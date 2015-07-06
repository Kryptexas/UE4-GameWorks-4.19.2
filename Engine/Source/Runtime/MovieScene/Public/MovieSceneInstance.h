// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


class UMovieSceneObjectManager;
class UMovieSceneTrack;


typedef TMap<TWeakObjectPtr<UMovieSceneTrack>, TSharedPtr<class IMovieSceneTrackInstance>> FMovieSceneInstanceMap;


class FMovieSceneInstance
	: public TSharedFromThis<FMovieSceneInstance>
{
public:

	/**
	 * Creates and initializes a new instance from a movie scene.
	 *
	 * @param InMovieScene The movie scene that this instance represents.
	 */
	MOVIESCENE_API FMovieSceneInstance(UMovieScene& InMovieScene);

	/** Save state of the objects that this movie scene controls. */
	MOVIESCENE_API void SaveState();

	/** Restore state of the objects that this movie scene controls. */
	MOVIESCENE_API void RestoreState();

	/**
	 * Updates this movie scene.
	 *
	 * @param Position The local playback position.
	 * @param Player Movie scene player interface for interaction with runtime data.
	 */
	MOVIESCENE_API void Update(float Position, float LastPosition, class IMovieScenePlayer& Player);

	/**
	 * Refreshes the existing instance.
	 *
	 * Called when something significant about movie scene data changes (like adding or removing a track).
	 * Instantiates all new tracks found and removes instances for tracks that no longer exist.
	 */
	MOVIESCENE_API void RefreshInstance(class IMovieScenePlayer& Player);

	/**
	 * Get the movie scene associated with this instance.
	 *
	 * @return The movie scene object.
	 */
	UMovieScene* GetMovieScene()
	{
		return MovieScene.Get();
	}

	/** 
	 * Get the time range for the movie scene.
	 *
	 * Note: This is not necessarily how long the instance will be playing.
	 * The parent could have changed the length.
	 *
	 * @return The time range.
	 */
	TRange<float> GetMovieSceneTimeRange() const
	{
		return TimeRange;
	}

	/**
	 * Get the movie scene's object manager.
	 *
	 * @return The object manager.
	 */
	TScriptInterface<UMovieSceneObjectManager> GetObjectManager() const;

protected:

	void RefreshInstanceMap(const TArray<UMovieSceneTrack*>& Tracks, const TArray<UObject*>& RuntimeObjects, FMovieSceneInstanceMap& TrackInstances, class IMovieScenePlayer& Player);

private:

	/** A paring of a runtime object guid to a runtime data and track instances animating the runtime objects */
	struct FMovieSceneObjectBindingInstance
	{
		/** Runtime object guid for looking up runtime UObjects */
		FGuid ObjectGuid;

		/** Actual runtime objects */	
		TArray<UObject*> RuntimeObjects;

		/** Instances that animate the runtime objects */
		FMovieSceneInstanceMap TrackInstances;
	};


	/** MovieScene that is instanced */
	TWeakObjectPtr<UMovieScene> MovieScene;

	/** The shot track instance map */
	TSharedPtr<class IMovieSceneTrackInstance> ShotTrackInstance;

	/** All Master track instances */
	FMovieSceneInstanceMap MasterTrackInstances;

	/** All object binding instances */
	TMap<FGuid, FMovieSceneObjectBindingInstance> ObjectBindingInstances;

	/** Cached time range for the movie scene */
	TRange<float> TimeRange;
};
