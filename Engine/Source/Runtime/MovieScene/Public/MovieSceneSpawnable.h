// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSpawnable.generated.h"


/**
 * MovieSceneSpawnable describes an object that can be spawned for this MovieScene
 */
USTRUCT()
struct FMovieSceneSpawnable 
{
	GENERATED_USTRUCT_BODY()

public:

	/** FMovieSceneSpawnable default constructor */
	FMovieSceneSpawnable() { }

	/** FMovieSceneSpawnable initialization constructor */
	FMovieSceneSpawnable(const FString& InitName, UClass* InitClass, UObject* InitCounterpartGamePreviewObject)
		: Guid(FGuid::NewGuid())
		, Name(InitName)
		, GeneratedClass(InitClass)
		, CounterpartGamePreviewObject(InitCounterpartGamePreviewObject)
	{ }

public:

	/**
	 * Get the Blueprint associated with the spawnable object.
	 *
	 * @return Blueprint class
	 * @see GetCounterpartGamePreviewObject, GetGuid, GetName
	 */
	UClass* GetClass()
	{
		return GeneratedClass;
	}

	/**
	 * Get the game preview counterpart object for the spawnable object, if it has one.
	 *
	 * @return Counterpart object, or nullptr if it doesn't have one.
	 * @see GetClass, GetGuid, GetName
	 */
	const FWeakObjectPtr& GetCounterpartGamePreviewObject() const
	{
		return CounterpartGamePreviewObject;
	}

	/**
	 * Get the unique identifier of the spawnable object.
	 *
	 * @return Object GUID.
	 * @see GetClass, GetCounterpartGamePreviewObject, GetName
	 */
	const FGuid& GetGuid() const
	{
		return Guid;
	}

	/**
	 * Get the name of the spawnable object.
	 *
	 * @return Object name.
	 * @see GetClass, GetCounterpartGamePreviewObject, GetGuid
	 */
	const FString& GetName() const
	{
		return Name;
	}

private:

	/** Unique identifier of the spawnable object. */
	// @todo sequencer: Guids need to be handled carefully when the asset is duplicated (or loaded after being copied on disk).
	//					Sometimes we'll need to generate fresh Guids.
	UPROPERTY()
	FGuid Guid;

	/** Name label */
	// @todo sequencer: Should be editor-only probably
	UPROPERTY()
	FString Name;

	/** Data-only blueprint-generated-class for this object */
	// @todo sequencer: Could be weak object ptr here, IF blueprints that are inners are housekept properly without references
	UPROPERTY()
	UClass* GeneratedClass;

	/** Optional transient weak pointer to the game preview object this spawnable was created to capture data for.  This is
	    used in the editor when capturing keyframe data from a live simulation */
	// @todo sequencer data: Should be editor only
	FWeakObjectPtr CounterpartGamePreviewObject;
};
