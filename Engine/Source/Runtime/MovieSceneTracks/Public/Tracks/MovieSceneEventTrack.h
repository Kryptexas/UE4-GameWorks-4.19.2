// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneTrack.h"
#include "MovieSceneEventTrack.generated.h"


/**
 * Implements a movie scene track that triggers discrete events during playback.
 */
UCLASS(MinimalAPI)
class UMovieSceneEventTrack
	: public UMovieSceneTrack
{
	GENERATED_BODY()

public:

	/**
	 * Adds an event to the appropriate section.
	 *
	 * This method will create a new section if no appropriate one exists.
	 *
	 * @param Time The time at which the event should be triggered.
	 * @param EventName The name of the event to be triggered.
	 */
	bool AddKeyToSection(float Time, FName EventName);

	/**
	 * Trigger the events that fall into the given time range.
	 *
	 * @param TimeRange The time range to trigger events in.
	 * @param Backwards Whether the events should be triggered in reverse order.
	 */
	void TriggerEvents(TRange<float> TimeRange, bool Backwards);

public:

	// UMovieSceneTrack interface

	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() override;
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
	virtual TRange<float> GetSectionBoundaries() const override;
	virtual FName GetTrackName() const override;
	virtual bool HasSection(UMovieSceneSection* Section) const override;
	virtual bool IsEmpty() const override;
	virtual void RemoveAllAnimationData() override;
	virtual void RemoveSection(UMovieSceneSection* Section) override;

private:

	/** If events should be fired when passed playing the sequence forwards. */
	UPROPERTY(EditAnywhere, Category=TrackEvent)
	uint32 bFireEventsWhenForwards:1;

	/** If events should be fired when passed playing the sequence backwards. */
	UPROPERTY(EditAnywhere, Category=TrackEvent)
	uint32 bFireEventsWhenBackwards:1;

	/** If checked each key's event name is the exact name of the custom event function in level script that will be called */
	UPROPERTY(EditAnywhere, Category=TrackEvent)
	uint32 bUseCustomEventName:1;

	/** The track's sections. */
	UPROPERTY()
	TArray<UMovieSceneSection*> Sections;

	/** Name of this track. */
	UPROPERTY()
	FName TrackName;
};
