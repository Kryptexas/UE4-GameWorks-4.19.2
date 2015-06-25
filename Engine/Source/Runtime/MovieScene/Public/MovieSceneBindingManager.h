// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBindingManager.generated.h"


class FMovieSceneInstance;
class UMovieScene;
class UObject;


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
	friend bool operator==( const FMovieSceneObjectId& X, const FMovieSceneObjectId& Y )
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
	friend bool operator!=( const FMovieSceneObjectId& X, const FMovieSceneObjectId& Y )
	{
		return ((X.ObjectGuid != Y.ObjectGuid) || (X.ComponentName != X.ComponentName));
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


/**
 * Interface for movie scene object binding managers.
 */
UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UMovieSceneBindingManager
	: public UInterface
{
	GENERATED_UINTERFACE_BODY()
};


/**
 * Interface for movie scene object binding managers.
 */
class IMovieSceneBindingManager
{
	GENERATED_IINTERFACE_BODY()

	/**
	 * Called when Sequencer has created an object binding for a possessable object
	 * 
	 * @param PossessableGuid The guid used to map to the possessable object.  Note the guid can be bound to multiple objects at once
	 * @param PossessedObject The runtime object which was possessed.
	 */
	virtual void BindPossessableObject(const FMovieSceneObjectId& PossessableGuid, UObject& PossessedObject) PURE_VIRTUAL(IMovieSceneBindingManager::BindPossessableObject,);
	
	/**
	 * @todo sequencer: add documentation
	 */
	virtual bool CanPossessObject( UObject& Object ) const PURE_VIRTUAL(IMovieSceneBindingManager::CanPossessObject, return false;);

	/**
	 * Finds an existing guid for a bound object
	 *
	 * @param MovieScene The movie scene that may contain the binding
	 * @param The object to get the guid for
	 * @return The object identifier or an invalid guid if not found
	 */
	virtual FMovieSceneObjectId FindGuidForObject(const UMovieScene& MovieScene, UObject& Object) const PURE_VIRTUAL(IMovieSceneBindingManager::FindGuidForObject, return FGuid(););

	/**
	 * @todo sequencer: add documentation
	 */
	virtual void GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects) const PURE_VIRTUAL(IMovieSceneBindingManager::GetRuntimeObjects,);

	/**
	 * 
	 * Tries to get a display name for the binding represented by the guid.
	 *
	 * @param ObjectGuid The guid for the object binding.
	 * @param DisplayName The display name for the object binding.
	 * @returns true if DisplayName has been set to a valid display name, otherwise false.
	 */
	virtual bool TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, FText& DisplayName) const PURE_VIRTUAL(IMovieSceneBindingManager::TryGetObjectBindingDisplayName, return false;);

	/**
	 * Unbinds all possessable objects from the provided guid
	 *
	 * @param PossessableGuid The guid bound to possessable objects that should be removed
	 */
	virtual void UnbindPossessableObjects(const FMovieSceneObjectId& PossessableGuid) PURE_VIRTUAL(IMovieSceneBindingManager::UnbindPossessableObjects,);
};
