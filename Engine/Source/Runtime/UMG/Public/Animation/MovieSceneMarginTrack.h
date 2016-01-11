// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScenePropertyTrack.h"
#include "KeyParams.h"
#include "MovieSceneMarginTrack.generated.h"

struct FMarginKey
{
	FMargin Value;
	FName CurveName;
};

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
	 * @param KeyParams         The keying parameters
	 * @return True if the key was successfully added.
	 */
	UMG_API bool AddKeyToSection( float Time, const FMarginKey& MarginKey, FKeyParams KeyParams );

	/**
	 * Evaluates the track at the playback position
	 *
	 * @param Position	The current playback position
	 * @param LastPosition	The last playback position
	 * @param InOutMargin 	The margin at the playback position
	 * @return true if anything was evaluated. Note: if false is returned OutMargin remains unchanged
	 */
	bool Eval( float Position, float LastPostion, FMargin& InOutMargin ) const;

	/**
	 * Get whether the track can be keyed at a particular time.
	 *
	 * @param Time				The time relative to the owning movie scene where the section should be
	 * @param Value				The value of the key
	 * @param KeyParams         The keying parameters
	 * @return True if the key was successfully added.
	 */
	UMG_API bool CanKeyTrack( float Time, const FMarginKey& MarginKey, FKeyParams KeyParams ) const;
};
