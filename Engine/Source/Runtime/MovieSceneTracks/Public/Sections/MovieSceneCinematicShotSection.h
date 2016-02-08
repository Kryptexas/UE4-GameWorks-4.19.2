// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSubSection.h"
#include "MovieSceneCinematicShotSection.generated.h"

class UMovieSceneSequence;


/**
 * Implements a cinematic shot section.
 */
UCLASS()
class MOVIESCENETRACKS_API UMovieSceneCinematicShotSection
	: public UMovieSceneSubSection
{
	GENERATED_BODY()

	/** Default constructor. */
	UMovieSceneCinematicShotSection();

public:

	/** @return The shot display name */
	FText GetShotDisplayName() const
	{
		return DisplayName;
	}

	/** Set the shot display name */
	void SetShotDisplayName(const FText& InDisplayName)
	{
		if (TryModify())
		{
			DisplayName = InDisplayName;
		}
	}

private:

	/** The Shot's display name */
	UPROPERTY()
	FText DisplayName;
};
