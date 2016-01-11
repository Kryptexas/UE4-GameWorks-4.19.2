// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


class IMovieScenePlayer;


/**
 * A track instance holds the live objects for a track.  
 */
class IMovieSceneTrackInstance
{
public:

	/** Virtual destructor. */
	virtual ~IMovieSceneTrackInstance() { }

	/**
	 * Save state of objects that this instance will be editing.
	 * @todo Sequencer: This is likely editor only
	 */
	virtual void SaveState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) = 0;

	/**
	 * Restore state of objects that this instance edited.
	 * @todo Sequencer: This is likely editor only
	 */
	virtual void RestoreState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) = 0;

	/**
	 * Main update function for track instances.  Called in game and in editor when we update a moviescene.
	 *
	 * @param Position			The current position of the moviescene that is playing
	 * @param LastPosition		The previous playback position
	 * @param RuntimeObjects	Runtime objects bound to this instance (if any)
	 * @param Player			The playback interface.  Contains state and some other functionality for runtime playback
	 */
	virtual void Update(float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) = 0;
	
	/**
	 * Refreshes the current instance
	 */
	virtual void RefreshInstance(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player) = 0;

	/**
	 * Removes all instance data from this track instance
	 *
	 * Called before an instance is removed
	 */
	virtual void ClearInstance( IMovieScenePlayer& Player ) = 0;

	/**
	* Evaluation order
	*/
	virtual float EvalOrder() { return 0.f; }
};
