// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSection.h"
#include "MovieSceneSubSection.generated.h"

class UMovieSceneSequence;


/**
 * Implements a section in sub-sequence tracks.
 */
UCLASS()
class MOVIESCENETRACKS_API UMovieSceneSubSection
	: public UMovieSceneSection
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UMovieSceneSubSection();

	/**
	 * Get the sequence that is assigned to this section.
	 *
	 * @return The sequence.
	 * @see SetSequence
	 */
	UMovieSceneSequence* GetSequence() const
	{
		return SubSequence;
	}

	/**
	 * Sets the sequence played by this section.
	 *
	 * @param Sequence The sequence to play.
	 * @see GetSequence
	 */
	void SetSequence(UMovieSceneSequence* Sequence)
	{
		SubSequence = Sequence;
	}

public:

	/** Number of seconds to skip at the beginning of the sub-sequence. */
	UPROPERTY(EditAnywhere, Category="Clipping")
	float StartOffset;

	/** Playback time scaling factor. */
	UPROPERTY(EditAnywhere, Category="Timing")
	float TimeScale;

	/** Preroll time. */
	UPROPERTY(EditAnywhere, Category="Timing")
	float PrerollTime;

protected:

	/**
	 * Movie scene being played by this section.
	 *
	 * @todo Sequencer: Should this be lazy loaded?
	 */
	UPROPERTY()
	UMovieSceneSequence* SubSequence;
};
