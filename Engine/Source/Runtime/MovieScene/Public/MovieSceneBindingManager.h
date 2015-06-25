// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBindingManager.generated.h"


class FMovieSceneInstance;
class UMovieScene;
class UObject;


UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UMovieSceneBindingManager
	: public UInterface
{
	GENERATED_UINTERFACE_BODY()
};


class IMovieSceneBindingManager
{
	GENERATED_IINTERFACE_BODY()

	/**
	 * Called when Sequencer has created an object binding for a possessable object
	 * 
	 * @param PossessableGuid The guid used to map to the possessable object.  Note the guid can be bound to multiple objects at once
	 * @param PossessedObject The runtime object which was possessed.
	 */
	virtual void BindPossessableObject(const FGuid& PossessableGuid, UObject& PossessedObject) PURE_VIRTUAL(IMovieSceneBindingManager::BindPossessableObject,);
	
	/**
	 * @todo sequencer: add documentation
	 */
	virtual bool CanPossessObject( UObject& Object ) const PURE_VIRTUAL(IMovieSceneBindingManager::CanPossessObject, return false;);

	/**
	 * Finds an existing guid for a bound object
	 *
	 * @param MovieScene The movie scene that may contain the binding
	 * @param The object to get the guid for
	 * @return The object guid or an invalid guid if not found
	 */
	virtual FGuid FindGuidForObject(const UMovieScene& MovieScene, UObject& Object) const PURE_VIRTUAL(IMovieSceneBindingManager::FindGuidForObject, return FGuid(););

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
	virtual void UnbindPossessableObjects(const FGuid& PossessableGuid) PURE_VIRTUAL(IMovieSceneBindingManager::UnbindPossessableObjects,);
};
