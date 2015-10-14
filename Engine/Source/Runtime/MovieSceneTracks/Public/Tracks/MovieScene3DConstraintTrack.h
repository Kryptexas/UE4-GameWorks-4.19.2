// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieScene3DConstraintTrack.generated.h"

/**
 * Base class for constraint tracks - tracks that are dependent upon other objects
 */
UCLASS( MinimalAPI )
class UMovieScene3DConstraintTrack : public UMovieSceneTrack
{
	GENERATED_UCLASS_BODY()
public:
	/** UMovieSceneTrack interface */
    virtual void RemoveAllAnimationData() override;
	virtual bool HasSection( UMovieSceneSection* Section ) const override;
	virtual void AddSection( UMovieSceneSection* Section ) override;
	virtual void RemoveSection( UMovieSceneSection* Section ) override;
	virtual bool IsEmpty() const override;
	virtual TRange<float> GetSectionBoundaries() const override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;

	/**
	 * Adds a constraint
	 *
	 * @param Time				    The time relative to the owning movie scene where the section should be
	 * @param ConstraintEndTime     Set the constraint to end at this time
	 * @param ConstraintId			The id to the constraint
	 */
	virtual void AddConstraint( float Time, float ConstraintEndTime, const FGuid& ConstraintId ) {}

protected:
	/** List of all constraint sections */
	UPROPERTY()
	TArray<UMovieSceneSection*> ConstraintSections;
};
