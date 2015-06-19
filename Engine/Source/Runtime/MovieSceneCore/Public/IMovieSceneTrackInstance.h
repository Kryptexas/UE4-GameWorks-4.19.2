// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Base class for an animated type instance in a Movie Scene
 */
class IMovieSceneTrackInstance
{
public:
	virtual ~IMovieSceneTrackInstance() {}

	/**
	* Save state of objects that this instance will be editing.
	*/
	virtual void SaveState ( const TArray<UObject*>& RuntimeObjects ) {}

	/**
	* Restore state of objects that this instance edited.
	*/
	virtual void RestoreState ( const TArray<UObject*>& RuntimeObjects ) {}

	/**
	 * Main update function for track instances.  Called in game and in editor when we update a moviescene.
	 *
	 * @param Position			The current position of the moviescene that is playing
	 * @param LastPosition		The previous playback position
	 * @param RuntimeObjects	Runtime objects bound to this instance (if any)
	 * @param Player			The playback interface.  Contains state and some other functionality for runtime playback
	 */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) = 0;
	

	/**
	 * Refreshes the current instance
	 */
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) = 0;
};
