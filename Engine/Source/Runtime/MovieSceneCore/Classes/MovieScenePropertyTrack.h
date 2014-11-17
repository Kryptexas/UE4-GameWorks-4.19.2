// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneTrack.h"
#include "MovieScenePropertyTrack.generated.h"


/**
 * Base class for tracks that animate an object property
 */
UCLASS( abstract )
class MOVIESCENECORE_API UMovieScenePropertyTrack : public UMovieSceneTrack
{
	GENERATED_UCLASS_BODY()

public:
	/** UMovieSceneTrack interface */
	virtual FName GetTrackName() const override { return PropertyName; }
	virtual void RemoveAllAnimationData() override;
	virtual bool HasSection( UMovieSceneSection* Section ) const override;
	virtual void RemoveSection( UMovieSceneSection* Section ) override;
	virtual bool IsEmpty() const override;
	virtual TRange<float> GetSectionBoundaries() const override;
	virtual TArray<UMovieSceneSection*> GetAllSections() const override;
	virtual bool HasShowableData() const override {return bSectionsAreShowable;}
	
	/**
	 * Sets the property name for this animatable property
	 *
	 * @param InPropertyName	The property being animated
	 */
	virtual void SetPropertyName( FName InPropertyName );

	/** @return the name of the property being animataed by this track */
	FName GetPropertyName() const { return PropertyName; }
	
	/** Sets this track as showable permanently */
	void SetAsShowable() {bSectionsAreShowable = true;}

protected:
	/**
	 * Finds a section at the current time.
	 *
	 * @param Time	The time relative to the owning movie scene where the section should be
	 *
	 * @return The found section, or NULL if it can't be found
	 */
	class UMovieSceneSection* FindOrAddSection(  float Time );
protected:

	/** Name of the property being changed */
	UPROPERTY()
	FName PropertyName;

	/** All the sections in this list */
	UPROPERTY()
	TArray<UMovieSceneSection*> Sections;
	
	/** True if this should generate a display node in Sequencer */
	UPROPERTY()
	bool bSectionsAreShowable;
};
