// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieScenePropertyTrack.h"
#include "MovieSceneBoolTrack.generated.h"

/**
 * Handles manipulation of float properties in a movie scene
 */
UCLASS( MinimalAPI )
class UMovieSceneBoolTrack : public UMovieScenePropertyTrack
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
	 * @param KeyParams         The keying parameters
	 * @return True if the key was successfully added.
	 */
	virtual bool AddKeyToSection( float Time, bool Value, FKeyParams KeyParams );

	/**
	 * Evaluates the track at the playback position
	 *
	 * @param Position	The current playback position
	 * @param LastPosition	The last plackback position
	 * @param OutBool 	The value at the playback position
	 * @return true if anything was evaluated. Note: if false is returned OutBool remains unchanged
	 */
	virtual bool Eval( float Position, float LastPostion, bool& OutBool ) const;

	/*
	 * Get whether the track can be keyed at a particular time.
	 *
	 * @param Time				The time relative to the owning movie scene where the section should be
	 * @param Value				The value of the key
	 * @param KeyParams         The keying parameters
	 * @return Whether the track can be keyed
	 */
	virtual bool CanKeyTrack( float Time, bool Value, FKeyParams KeyParams) const;
};
