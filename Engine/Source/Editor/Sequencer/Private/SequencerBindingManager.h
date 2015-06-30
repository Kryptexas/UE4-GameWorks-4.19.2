// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBindingManager.h"
#include "SequencerBindingManager.generated.h"


struct FSequencerActorInfo;


/**
 * Movie scene binding manager used by Sequencer.
 *
 * @todo squencer: gmp: probably want to refactor this to not depend on Actors/move into own module
 */
UCLASS(BlueprintType, MinimalAPI)
class USequencerBindingManager
	: public UObject
	, public IMovieSceneBindingManager
{
	GENERATED_UCLASS_BODY()

	/** Virtual destructor. */
	virtual ~USequencerBindingManager();

public:

	// IMovieSceneBindingManager interface

	virtual bool AllowsSpawnableObjects() const override;
	virtual void BindPossessableObject(const FMovieSceneObjectId& ObjectId, UObject& PossessedObject) override;
	virtual bool CanPossessObject( UObject& Object ) const override;
	virtual void DestroyAllSpawnedObjects() override;
	virtual FMovieSceneObjectId FindIdForObject(const UMovieScene& MovieScene, UObject& Object) const override;
	virtual void GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, TArray<UObject*>& OutRuntimeObjects) const override;
	virtual void RemoveMovieSceneInstance(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance) override;
	virtual void SpawnOrDestroyObjectsForInstance(TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll) override;
	virtual bool TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, FText& OutDisplayName) const override;
	virtual void UnbindPossessableObjects(const FMovieSceneObjectId& ObjectId) override;

protected:

	/**
	 * Deselect all proxy actors in the Editor.
	 *
	 * @todo sequencer: gmp: remove Editor dependencies from this class
	 */
	void DeselectAllActors();

	/**
	 * Find the unique identifier for a given proxy object.
	 *
	 * @param Object The proxy object to find the identifier for.
	 * @return The object's identifier.
	 * @see FindPuppetObjectForId
	 */
	FMovieSceneObjectId FindIdForSpawnableObject(UObject& Object) const;

	/**
	 * Find the puppet actor for a given object identifier.
	 *
	 * @param ObjectId The object identifier.
	 * @return The corresponding object, or nullptr if not found.
	 */
	UObject* FindActorInfoById(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId) const;

	/**
	 * Get the world that possessed actors belong to or where spawned actors should go.
	 *
	 * @return The world object.
	 */
	UWorld* GetWorld() const;

private:

	/** Handles changes of non-auto-keyed properties on any object. */
	void HandlePropagateObjectChanges(UObject* Object);

	/** Refresh either the reference actor or the data descriptor BP after its puppet proxy actor has been changed. */
	void PropagateActorChanges(const TSharedRef<FSequencerActorInfo> ActorInfo);

private:

	/** List of puppet objects that we've spawned and are actively managing. */
	TMap<TWeakPtr<FMovieSceneInstance>, TArray<TSharedRef<FSequencerActorInfo>>> InstanceToActorInfos;

	/** The object change listener. */
	TWeakPtr<ISequencerObjectChangeListener> ObjectChangeListener;
};
