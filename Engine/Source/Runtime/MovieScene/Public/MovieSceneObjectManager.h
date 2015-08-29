// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneObjectManager.generated.h"


class FMovieSceneInstance;
class UMovieScene;
class UObject;



/**
 * Interface for movie scene object managers.
 */
UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UMovieSceneObjectManager
	: public UInterface
{
	GENERATED_UINTERFACE_BODY()
};


/**
 * Interface for movie scene object managers.
 */
class IMovieSceneObjectManager
{
	GENERATED_IINTERFACE_BODY()

	/**
	 * Whether objects can be spawned at run-time.
	 *
	 * @return true if objects can be spawned by sequencer, false if only existing objects can be possessed.
	 */
	virtual bool AllowsSpawnableObjects() const PURE_VIRTUAL(IMovieSceneObjectManager::AllowsSpawnableObjects, return false;);

	/**
	 * Called when Sequencer has created an object binding for a possessable object
	 * 
	 * @param ObjectId The guid used to map to the possessable object.  Note the guid can be bound to multiple objects at once
	 * @param PossessedObject The runtime object which was possessed.
	 * @see UnbindPossessableObjects
	 */
	virtual void BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject) PURE_VIRTUAL(IMovieSceneObjectManager::BindPossessableObject,);
	
	/**
	 * @todo gmp: sequencer: add documentation.
	 */
	virtual bool CanPossessObject(UObject& Object) const PURE_VIRTUAL(IMovieSceneObjectManager::CanPossessObject, return false;);

	/**
	 * Destroy all objects that were spawned by this binding manager.
	 *
	 * @param MovieScene The movie scene that owns this manager.
	 */
	virtual void DestroyAllSpawnedObjects(UMovieScene& MovieScene) PURE_VIRTUAL(IMovieSceneObjectManager::DestroyAllSpawnedObjects,);

	/**
	 * Find the identifier for a possessed or spawned object.
	 *
	 * @param The scene that owns this binding manager.
	 * @param The object to get the identifier for.
	 * @return The object identifier, or an invalid GUID if not found.
	 */
	virtual FGuid FindObjectId(const UMovieScene& MovieScene, UObject& Object) const PURE_VIRTUAL(IMovieSceneObjectManager::FindGuidForObject, return FGuid(););

	/**
	 * Gets the possessed or spawned object for the specified identifier.
	 *
	 * @param ObjectId The unique identifier of the object.
	 * @return The object, or nullptr if not found.
	 */
	virtual UObject* FindObject(const FGuid& ObjectId) const PURE_VIRTUAL(IMovieSceneObjectManager::FindObject, return nullptr;);

	/**
	 * Called to spawn or destroy objects for a movie instance.
	 *
	 * @param MovieScene The movie scene to spawn or destroy objects for.
	 * @param DestroyAll If true, destroy all spawned objects for the instance, if false only destroy unused objects.
	 */
	virtual void SpawnOrDestroyObjects(UMovieScene* MovieScene, bool DestroyAll) PURE_VIRTUAL(IMovieSceneObjectManager::SpawnOrDestroyObjects,);

	/**
	 * 
	 * Tries to get a display name for the binding represented by the guid.
	 *
	 * @param ObjectGuid The guid for the object binding.
	 * @param DisplayName The display name for the object binding.
	 * @returns true if DisplayName has been set to a valid display name, otherwise false.
	 */
	virtual bool TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const PURE_VIRTUAL(IMovieSceneObjectManager::TryGetObjectDisplayName, return false;);

	/**
	 * Unbinds all possessable objects from the provided GUID.
	 *
	 * @param ObjectId The guid bound to possessable objects that should be removed.
	 * @see BindPossessableObject
	 */
	virtual void UnbindPossessableObjects(const FGuid& ObjectId) PURE_VIRTUAL(IMovieSceneObjectManager::UnbindPossessableObjects,);
};
