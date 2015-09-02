// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneEventSection.generated.h"


/**
 * Structure for event section keys.
 */
USTRUCT()
struct FMovieSceneEventSectionKey
{
	GENERATED_USTRUCT_BODY()

	/** The name of the event to be triggered. */
	UPROPERTY(EditAnywhere, Category=EventTrackKey)
	FName EventName;

	/** The time at which the event should be triggered. */
	UPROPERTY()
	float Time;

	/** Default constructor. */
	FMovieSceneEventSectionKey()
		: Time(0.0f)
	{ }

	/** Creates and initializes a new instance. */
	FMovieSceneEventSectionKey(const FName& InEventName, float InTime)
		: EventName(InEventName)
		, Time(InTime)
	{ }

	/** Operator less, used to sort the heap based on time until execution. */
	bool operator<(const FMovieSceneEventSectionKey& Other) const
	{
		return Time < Other.Time;
	}
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

	/**
	 * Add a key with the specified event.
	 *
	 * @param Time The location in time where the key should be added.
	 * @param EventName The name of the event to fire at the specified time.
	 * @param KeyParams The keying parameters.
	 */
	void AddKey(float Time, const FName& EventName, FKeyParams KeyParams);

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
