// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneObjectId.h"
#include "MovieSceneBindingManager.generated.h"


class FMovieSceneInstance;
class UMovieScene;
class UObject;



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
	 * Whether objects can be spawned at run-time.
	 *
	 * @return true if objects can be spawned by sequencer, false if only existing objects can be possessed.
	 */
	virtual bool AllowsSpawnableObjects() const PURE_VIRTUAL(IMovieSceneBindingManager::AllowsSpawnableObjects, return false;);

	/**
	 * Called when Sequencer has created an object binding for a possessable object
	 * 
	 * @param ObjectId The guid used to map to the possessable object.  Note the guid can be bound to multiple objects at once
	 * @param PossessedObject The runtime object which was possessed.
	 * @see UnbindPossessableObjects
	 */
	virtual void BindPossessableObject(const FMovieSceneObjectId& ObjectId, UObject& PossessedObject) PURE_VIRTUAL(IMovieSceneBindingManager::BindPossessableObject,);
	
	/**
	 * @todo gmp: sequencer: add documentation.
	 */
	virtual bool CanPossessObject( UObject& Object ) const PURE_VIRTUAL(IMovieSceneBindingManager::CanPossessObject, return false;);

	/**
	 * @todo gmp: sequencer: add documentation
	 */
	virtual void DestroyAllSpawnedObjects() PURE_VIRTUAL(IMovieSceneBindingManager::DestroyAllSpawnedObjects,);

	/**
	 * Finds an existing guid for a bound object.
	 *
	 * @param MovieScene The movie scene that may contain the binding.
	 * @param The object to get the guid for.
	 * @return The object identifier or an invalid guid if not found.
	 */
	virtual FMovieSceneObjectId FindIdForObject(const UMovieScene& MovieScene, UObject& Object) const PURE_VIRTUAL(IMovieSceneBindingManager::FindGuidForObject, return FGuid(););

	/**
	 * @todo gmp: sequencer: add documentation
	 */
	virtual void GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, TArray<UObject*>& OutRuntimeObjects) const PURE_VIRTUAL(IMovieSceneBindingManager::GetRuntimeObjects,);

	/**
	 * @todo gmp: sequencer: add documentation
	 */
	virtual void RemoveMovieSceneInstance(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance) PURE_VIRTUAL(IMovieSceneBindingManager::RemoveMovieSceneInstance,);

	/**
	 * Called to spawn or destroy objects for a movie instance.
	 *
	 * @param MovieSceneInstance The instance to spawn or destroy objects for.
	 * @param bDestroyAll If true, destroy all spawned objects for the instance, if false only destroy unused objects.
	 */
	virtual void SpawnOrDestroyObjectsForInstance( TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll ) PURE_VIRTUAL(IMovieSceneBindingManager::SpawnOrDestroyObjectsForInstance,);

	/**
	 * 
	 * Tries to get a display name for the binding represented by the guid.
	 *
	 * @param ObjectGuid The guid for the object binding.
	 * @param DisplayName The display name for the object binding.
	 * @returns true if DisplayName has been set to a valid display name, otherwise false.
	 */
	virtual bool TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, FText& OutDisplayName) const PURE_VIRTUAL(IMovieSceneBindingManager::TryGetObjectBindingDisplayName, return false;);

	/**
	 * Unbinds all possessable objects from the provided GUID.
	 *
	 * @param ObjectId The guid bound to possessable objects that should be removed.
	 * @see BindPossessableObject
	 */
	virtual void UnbindPossessableObjects(const FMovieSceneObjectId& ObjectId) PURE_VIRTUAL(IMovieSceneBindingManager::UnbindPossessableObjects,);
};
