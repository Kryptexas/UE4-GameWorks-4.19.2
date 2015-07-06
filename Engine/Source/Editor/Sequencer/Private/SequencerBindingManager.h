// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBindingManager.h"
#include "SequencerBinding.h"
#include "SequencerBindingManager.generated.h"


/**
 * Movie scene binding manager used by Sequencer.
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
	virtual void BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject) override;
	virtual bool CanPossessObject(UObject& Object) const override;
	virtual void ClearBindings() override;
	virtual void DestroyAllSpawnedObjects(UMovieScene& MovieScene) override;
	virtual FGuid FindObjectId(const UMovieScene& MovieScene, UObject& Object) const override;
	virtual UObject* FindObject(const FGuid& ObjectId) const override;
	virtual void SpawnOrDestroyObjects(UMovieScene* MovieScene, bool DestroyAll) override;
	virtual bool TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const override;
	virtual void UnbindPossessableObjects(const FGuid& ObjectId) override;

protected:

	/**
	 * Deselect all proxy actors in the Editor.
	 *
	 * @todo sequencer: gmp: remove Editor dependencies from this class
	 */
	SEQUENCER_API void DeselectAllActors();

	/**
	 * Get the world that possessed actors belong to or where spawned actors should go.
	 *
	 * @return The world object.
	 */
	SEQUENCER_API UWorld* GetWorld() const;

private:

	/** Handles changes of non-auto-keyed properties on any object. */
	void HandlePropagateObjectChanges(UObject* Object);

	/** Refresh either the reference actor or the data descriptor BP after its puppet proxy actor has been changed. */
	void PropagateActorChanges(const FGuid& ObjectId, AActor* Actor);

private:

	/** Collection of object bindings. */
	// @todo sequencer: gmp: need support for UStruct keys in TMap
	UPROPERTY()
	TMap<FString, FSequencerBinding> Bindings;
	//TMap<FGuid, FSequencerBinding> Bindings;

	/** The object change listener. */
	TWeakPtr<ISequencerObjectChangeListener> ObjectChangeListener;

	/** Collection of spawned proxy actors. */
	TMap<FGuid, TWeakObjectPtr<AActor>> SpawnedActors;
};
