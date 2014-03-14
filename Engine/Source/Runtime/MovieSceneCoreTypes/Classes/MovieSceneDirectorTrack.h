// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneDirectorTrack.generated.h"

/**
 * Handles manipulation of shot properties in a movie scene
 */
UCLASS( MinimalAPI )
class UMovieSceneDirectorTrack : public UMovieSceneTrack
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

	/** Adds a new shot at the specified time */
	virtual void AddNewShot(FGuid CameraHandle, float Time);

	/** @return The shot sections on this track */
	const TArray<UMovieSceneSection*>& GetShotSections() const { return ShotSections; }

private:
	/** List of all shot sections */
	UPROPERTY()
	TArray<UMovieSceneSection*> ShotSections;
};
