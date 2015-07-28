// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneEventSection.generated.h"


/**
 * Structure for event section keys.
 */
USTRUCT()
struct FMovieSceneEventSectionKey
{
	GENERATED_BODY()

	/** The name of the event to be triggered. */
	UPROPERTY(EditAnywhere, Category=EventTrackKey)
	FName EventName;

	/** The time at which the event should be triggered. */
	UPROPERTY()
	float Time;
};


/**
 * Implements a section in movie scene event tracks.
 */
UCLASS( MinimalAPI )
class UMovieSceneEventSection
	: public UMovieSceneSection
{
	GENERATED_BODY()

public:

	// UMovieSceneSection interface

	virtual void DilateSection(float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles) override;
	virtual void GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const override;
	virtual void GetSnapTimes(TArray<float>& OutSnapTimes, bool bGetSectionBorders) const override;
	virtual void MoveSection(float DeltaPosition, TSet<FKeyHandle>& KeyHandles) override;

private:

	/** The section's keys. */
	UPROPERTY()
	TArray<FMovieSceneEventSectionKey> Keys;
};
