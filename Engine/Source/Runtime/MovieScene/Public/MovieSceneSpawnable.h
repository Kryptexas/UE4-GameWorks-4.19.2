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
	FMovieSceneSpawnable(const FString& InitName, UClass* InitClass)
		: Guid(FGuid::NewGuid())
		, Name(InitName)
		, GeneratedClass(InitClass)
	{ }

public:

	/**
	 * Get the Blueprint associated with the spawnable object.
	 *
	 * @return Blueprint class
	 * @see GetGuid, GetName
	 */
	UClass* GetClass()
	{
		return GeneratedClass;
	}

	/**
	 * Get the unique identifier of the spawnable object.
	 *
	 * @return Object GUID.
	 * @see GetClass, GetName
	 */
	const FGuid& GetGuid() const
	{
		return Guid;
	}

	/**
	 * Get the name of the spawnable object.
	 *
	 * @return Object name.
	 * @see GetClass, GetGuid
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
};
