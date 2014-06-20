// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScenePropertyTrack.h"
#include "MovieSceneMarginTrack.generated.h"

/**
 * Handles manipulation of FMargins in a movie scene
 */
UCLASS( MinimalAPI )
class UMovieSceneMarginTrack : public UMovieScenePropertyTrack
{
	GENERATED_UCLASS_BODY()

public:
	/** UMovieSceneTrack interface */
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() override;

	/**
	 * Adds a key to a section.  Will create the section if it doesn't exist
	 *
	 * @param Time				The time relative to the owning movie scene where the section should be
	 * @param Value				The value of the key
	 * @return True if the key was successfully added.
	 */
	virtual bool AddKeyToSection( float Time, FMargin Value );
	
	/**
	 * Evaluates the track at the playback position
	 *
	 * @param Position	The current playback position
	 * @param LastPosition	The last plackback position
	 * @param The color at the playback position
	 * @return true if anything was evaluated. Note: if false is returned OutMargin remains unchanged
	 */
	virtual bool Eval( float Position, float LastPostion, FMargin& OutMargin ) const;
};
