// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSequence.h"
#include "LevelSequenceObject.h"
#include "LevelSequenceObjectReference.h"
#include "LevelSequence.generated.h"


/**
 * Movie scene animation for Actors.
 */
UCLASS(BlueprintType)
class LEVELSEQUENCE_API ULevelSequence
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Collection of possessed objects. */
	UPROPERTY()
	FLevelSequenceObjectReferenceMap DefaultObjectReferences;

	/** Pointer to the movie scene that controls this animation. */
	UPROPERTY()
	UMovieScene* MovieScene;

public:

	/** Initialize this level sequence. */
	void Initialize();

	/** Convert old-style lazy object ptrs to new-style references using the specified context */
	void ConvertPersistentBindingsToDefault(UObject* FixupContext);

private:

	/** Deprecated property housing old possessed object bindings */
	UPROPERTY()
	TMap<FString, FLevelSequenceObject> PossessedObjects_DEPRECATED;
};


/**
 * An instance of a ULevelSequence that supports resolution and remapping of object bindings.
 */
UCLASS()
class LEVELSEQUENCE_API ULevelSequenceInstance
	: public UMovieSceneSequence
{
public:

	GENERATED_BODY()
	
	/**
	 * Initialize this instance. Safe to be called multiple times. Does not reset object re-mappings unless bInCanRemapBindings is false.
	 *
	 * @param InSequence The level sequence to instantiate.
	 * @param InContext The context to use for object binding resolution.
	 * @param bInCanRemapBindings Whether this instance is permitted to remap object bindings or not.
	 */
	void Initialize(ULevelSequence* InSequence, UObject* InContext, bool bInCanRemapBindings);
	
	/**
	 * Change this instance's context.
	 *
	 * @param NewContext The new context to use for object binding resolution.
	 */
	void SetContext(UObject* NewContext);

	/**
	 * Get the level sequence of which this is an instance
	 *
	 * @return this instance's parent level sequence
	 */
	ULevelSequence* GetLevelSequence()
	{
		return LevelSequence;
	}

public:

	// UMovieSceneSequence interface

	virtual bool AllowsSpawnableObjects() const override;
	virtual void BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject) override;
	virtual bool CanPossessObject(UObject& Object) const override;
	virtual void DestroyAllSpawnedObjects() override;
	virtual UObject* FindObject(const FGuid& ObjectId) const override;
	virtual FGuid FindObjectId(UObject& Object) const override;
	virtual UMovieScene* GetMovieScene() const override;
	virtual UObject* GetParentObject(UObject* Object) const override;
	virtual void SpawnOrDestroyObjects(bool DestroyAll) override;
	virtual void UnbindPossessableObjects(const FGuid& ObjectId) override;

#if WITH_EDITOR
	virtual bool TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const override;
	virtual FText GetDisplayName() const override;
#endif

private:

	/** The level sequence we are instancing. */
	UPROPERTY(transient)
	ULevelSequence* LevelSequence;

	/** Flag to specify whether this instance can remap bindings or not. */
	UPROPERTY()
	bool bCanRemapBindings;

	/** Collection of re-mappings for possessed objects references. */
	UPROPERTY()
	FLevelSequenceObjectReferenceMap RemappedObjectReferences;

private:

	/** A context to use for resolving object bindings */
	TWeakObjectPtr<UObject> ResolutionContext;

	/** A transient map of cached object bindings */
	mutable TMap<FGuid, TWeakObjectPtr<UObject>> CachedObjectBindings;

	/** Collection of spawned proxy actors. */
	TMap<FGuid, TWeakObjectPtr<AActor>> SpawnedActors;
};
