// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSpawnTrack.generated.h"

/**
 * Handles when a spawnable should be spawned and destroyed
 */
UCLASS( MinimalAPI )
class UMovieSceneSpawnTrack
	: public UMovieSceneTrack
{
public:

	UMovieSceneSpawnTrack(const FObjectInitializer& Obj);
	GENERATED_BODY()

public:

	// UMovieSceneTrack interface
	virtual UMovieSceneSection* CreateNewSection() override;

	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() override;
	
	virtual FName GetTrackName() const override
	{
		static FName Name = "Spawned";
		return Name;
	}

	virtual bool HasSection(const UMovieSceneSection& Section ) const override
	{
		return Sections.ContainsByPredicate([&](const UMovieSceneSection* In){ return In == &Section; });
	}

	virtual void AddSection(UMovieSceneSection& Section) override
	{
		Sections.Add(&Section);
	}

	virtual void RemoveSection(UMovieSceneSection& Section) override
	{
		Sections.RemoveAll([&](const UMovieSceneSection* In){ return In == &Section; });
	}

	virtual bool IsEmpty() const override
	{
		return Sections.Num() == 0;
	}

	virtual TRange<float> GetSectionBoundaries() const override
	{
		return TRange<float>::All();
	}

	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override
	{
		return Sections;
	}

	/** Set the object identifier that this spawn track controls */
	void SetObject(const FGuid& InGuid)
	{
		ObjectGuid = InGuid;
	}

	/** Get the object identifier that this spawn track controls */
	const FGuid& GetObject() const
	{
		return ObjectGuid;
	}

	/** Evaluate whether the controlled object should currently be spawned or not */
	bool Eval(float Position, float LastPostion, bool& bOutSpawned) const;

protected:

	/** All the sections in this track */
	UPROPERTY()
	TArray<UMovieSceneSection*> Sections;

	/** The guid relating to the object we are to spawn and destroy */
	UPROPERTY()
	FGuid ObjectGuid;
};