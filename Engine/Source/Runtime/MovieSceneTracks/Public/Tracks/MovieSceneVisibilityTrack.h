// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieScenePropertyTrack.h"
#include "MovieSceneVisibilityTrack.generated.h"

/**
 * Handles manipulation of visibility properties in a movie scene
 */
UCLASS( MinimalAPI )
class UMovieSceneVisibilityTrack : public UMovieSceneBoolTrack
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
	virtual bool AddKeyToSection( float Time, bool Value );
};
