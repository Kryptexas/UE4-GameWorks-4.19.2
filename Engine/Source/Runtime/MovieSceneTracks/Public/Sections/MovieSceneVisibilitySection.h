// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBoolSection.h"
#include "MovieSceneVisibilitySection.generated.h"

/**
 * A single visibility section.
 *
 * The property that's being tracked by this section is bHiddenInGame. 
 * This custom bool track stores the inverse keys to display visibility (A green section bar means visible).
 *
 */
UCLASS( MinimalAPI )
class UMovieSceneVisibilitySection : public UMovieSceneBoolSection
{
	GENERATED_UCLASS_BODY()
public:

	/** 
	 * Adds a key to the section
	 *
	 * @param Time	The location in time where the key should be added
	 * @param Value	The value of the key
	 * @param KeyParams The keying parameters
	 */
	void AddKey( float Time, bool Value, FKeyParams KeyParams );

	/** 
	 * Determines if a new key would be new data, or just a duplicate of existing data
	 *
	 * @param Time	The location in time where the key would be added
	 * @param Value	The value of the new key
	 * @param KeyParams The keying parameters
	 * @return True if the new key would be new data, false if duplicate
	 */
	bool NewKeyIsNewData(float Time, bool Value, FKeyParams KeyParams) const;
};
