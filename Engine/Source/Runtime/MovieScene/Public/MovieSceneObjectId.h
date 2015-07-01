// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneObjectId.generated.h"


/**
 * Identifies an object (or embedded component) bound in a movie scene.
 */
USTRUCT()
struct FMovieSceneObjectId
{
	GENERATED_USTRUCT_BODY()

	/** Unique identifier of a bound object. */
	UPROPERTY()
	FGuid ObjectGuid;

	/** Name of the bound object's component object being used (optional). */
	UPROPERTY()
	FName ComponentName;

public:

	/** Default constructor. */
	FMovieSceneObjectId() { }

	/** Creates and initializes a new instance from an object ID. */
	FMovieSceneObjectId(const FGuid& InObjectGuid)
		: ObjectGuid(InObjectGuid)
	{ }

	/** Creates and initializes a new instance from an object and component ID. */
	FMovieSceneObjectId(const FGuid& InObjectGuid, const FName& InComponentName)
		: ObjectGuid(InObjectGuid)
		, ComponentName(InComponentName)
	{ }

public:

	/**
	 * Compares two object identifiers for equality.
	 *
	 * @param X The first identifier to compare.
	 * @param Y The second identifier to compare.
	 * @return true if the identifiers are equal, false otherwise.
	 */
	friend bool operator==(const FMovieSceneObjectId& X, const FMovieSceneObjectId& Y)
	{
		return ((X.ObjectGuid == Y.ObjectGuid) && (X.ComponentName == X.ComponentName));
	}

	/**
	 * Compares two object identifiers for inequality.
	 *
	 * @param X The first identifier to compare.
	 * @param Y The second identifier to compare.
	 * @return true if the identifier are not equal, false otherwise.
	 */
	friend bool operator!=(const FMovieSceneObjectId& X, const FMovieSceneObjectId& Y)
	{
		return ((X.ObjectGuid != Y.ObjectGuid) || (X.ComponentName != X.ComponentName));
	}

	/**
	 * Calculates the hash for an object identifier.
	 *
	 * @param ObjectId The identifier to calculate the hash for.
	 * @return The hash.
	 */
	friend uint32 GetTypeHash(const FMovieSceneObjectId& ObjectId)
	{
		return GetTypeHash(ObjectId.ObjectGuid) ^ GetTypeHash(ObjectId.ComponentName);
	}

	/**
	 * Checks whether this object identifier is valid or not.
	 *
	 * @return true if valid, false otherwise.
	 */
	bool IsValid() const
	{
		return ObjectGuid.IsValid();
	}
};
