// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneAudioTrack.generated.h"

namespace AudioTrackConstants
{
	const float ScrubDuration = 0.050f;
	const FName UniqueTrackName("MasterAudio");
}

/**
 * Handles manipulation of audio
 */
UCLASS( MinimalAPI )
class UMovieSceneAudioTrack : public UMovieSceneTrack
{
	GENERATED_UCLASS_BODY()

public:
	/** UMovieSceneTrack interface */
	virtual FName GetTrackName() const OVERRIDE;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() OVERRIDE;
	virtual void RemoveAllAnimationData() OVERRIDE;
	virtual bool HasSection( UMovieSceneSection* Section ) const OVERRIDE;
	virtual void RemoveSection( UMovieSceneSection* Section ) OVERRIDE;
	virtual bool IsEmpty() const OVERRIDE;
	virtual TRange<float> GetSectionBoundaries() const OVERRIDE;
	virtual bool SupportsMultipleRows() const OVERRIDE { return true; }
	virtual TArray<UMovieSceneSection*> GetAllSections() const OVERRIDE;

	/** Adds a new sound cue to the audio */
	virtual void AddNewSound(class USoundBase* Sound, float Time);

	/** @return The audio sections on this track */
	const TArray<UMovieSceneSection*>& GetAudioSections() const { return AudioSections; }

	/** @return true if this is a master audio track */
	bool IsAMasterTrack() const;
private:
	/** List of all master audio sections */
	UPROPERTY()
	TArray<UMovieSceneSection*> AudioSections;
};
