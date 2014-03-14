// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneTrack.h"
#include "SubMovieSceneTrack.generated.h"

/**
 * Handles manipulation of float properties in a movie scene
 */
UCLASS( MinimalAPI )
class USubMovieSceneTrack : public UMovieSceneTrack
{
	GENERATED_UCLASS_BODY()
public:
	/** UMovieSceneTrack interface */
	virtual UMovieSceneSection* CreateNewSection() OVERRIDE;
	virtual FName GetTrackName() const OVERRIDE;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() OVERRIDE;
	virtual TArray<UMovieSceneSection*> GetAllSections() const OVERRIDE;
	virtual void RemoveSection( UMovieSceneSection* Section ) OVERRIDE;
	virtual bool IsEmpty() const OVERRIDE;
	virtual TRange<float> GetSectionBoundaries() const OVERRIDE;
	virtual bool HasSection( UMovieSceneSection* Section ) const OVERRIDE;

	/**
	 * Adds a movie scene section
	 *
	 * @param SubMovieScene	The movie scene to add
	 * @param Time	The time to add the section at
	 */
	virtual void AddMovieSceneSection( UMovieScene* SubMovieScene, float Time );
private:
	/** All movie scene sections.  */
	UPROPERTY()
	TArray<class UMovieSceneSection*> SubMovieSceneSections;
};
