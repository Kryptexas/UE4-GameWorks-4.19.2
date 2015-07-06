// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieScene3DPathTrack.generated.h"

/**
 * Handles manipulation of path tracks in a movie scene
 */
UCLASS( MinimalAPI )
class UMovieScene3DPathTrack : public UMovieSceneTrack
{
	GENERATED_UCLASS_BODY()
public:
	/** UMovieSceneTrack interface */
	virtual FName GetTrackName() const override;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() override;
	virtual void RemoveAllAnimationData() override;
	virtual bool HasSection( UMovieSceneSection* Section ) const override;
	virtual void RemoveSection( UMovieSceneSection* Section ) override;
	virtual bool IsEmpty() const override;
	virtual TRange<float> GetSectionBoundaries() const override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;

	/**
	 * Adds a path
	 *
	 * @param Time				The time relative to the owning movie scene where the section should be
	 * @param PathEndTime       Set the path to end at this time
	 * @param PathId			The id to the spline
	 */
	virtual void AddPath( float Time, float PathEndTime, const FGuid& PathId );

private:
	/** List of all path sections */
	UPROPERTY()
	TArray<UMovieSceneSection*> PathSections;
};
