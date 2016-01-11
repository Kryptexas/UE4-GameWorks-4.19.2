// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieScene3DConstraintTrack.h"
#include "MovieScene3DPathTrack.generated.h"

/**
 * Handles manipulation of path tracks in a movie scene
 */
UCLASS( MinimalAPI )
class UMovieScene3DPathTrack : public UMovieScene3DConstraintTrack
{
	GENERATED_UCLASS_BODY()
public:
	/** UMovieSceneTrack interface */
	virtual FName GetTrackName() const override;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() override;

	/**
	 * Adds a constraint
	 *
	 * @param Time				    The time relative to the owning movie scene where the section should be
	 * @param ConstraintEndTime     Set the constraint to end at this time
	 * @param ConstraintId			The id to the constraint
	 */
	virtual void AddConstraint( float Time, float ConstraintEndTime, const FGuid& ConstraintId ) override;
};
