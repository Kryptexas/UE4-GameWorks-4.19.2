// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneParticleTrack.generated.h"


/**
 * Handles triggering of particle emitters
 */
UCLASS( MinimalAPI )
class UMovieSceneParticleTrack : public UMovieSceneTrack
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
	virtual bool SupportsMultipleRows() const OVERRIDE { return false; }
	virtual TArray<UMovieSceneSection*> GetAllSections() const OVERRIDE;
	
	virtual void AddNewParticleSystem(float KeyTime, bool bTrigger);

	virtual TArray<UMovieSceneSection*> GetAllParticleSections() const {return ParticleSections;}

private:
	/** List of all particle sections */
	UPROPERTY()
	TArray<UMovieSceneSection*> ParticleSections;
};
