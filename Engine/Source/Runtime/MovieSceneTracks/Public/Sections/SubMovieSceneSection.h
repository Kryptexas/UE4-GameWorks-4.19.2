// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SubMovieSceneSection.generated.h"

class UMovieSceneSequence;

/**
 * A container for a movie scene within a movie scene
 */
UCLASS()
class MOVIESCENETRACKS_API USubMovieSceneSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:
	/**
	 * Sets the movie scene played by this section
	 *
	 * @param InMovieScene	The movie scene to play
	 */
	void SetMovieSceneAnimation( UMovieSceneSequence* InAnimation );

	/** @return The movie scene played by this section */
	UMovieSceneSequence* GetMovieSceneAnimation() const;
private:
	/** Movie scene being played by this section */
	/** @todo Sequencer: Should this be lazy loaded? */
	UPROPERTY()
	UMovieSceneSequence* Sequence;
};
