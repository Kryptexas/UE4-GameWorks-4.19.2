// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneAnimation.h"
#include "ActorAnimationObject.h"
#include "ActorAnimation.generated.h"


/**
 * Movie scene animation for Actors.
 */
UCLASS(BlueprintType, MinimalAPI)
class UActorAnimation
	: public UObject
	, public IMovieSceneAnimation
{
	GENERATED_UCLASS_BODY()

	/** Initialize this actor animation. */
	ACTORANIMATION_API void Initialize();

public:

	// IMovieSceneAnimation interface

	virtual bool AllowsSpawnableObjects() const override;
	virtual void BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject) override;
	virtual bool CanPossessObject(UObject& Object) const override;
	virtual void DestroyAllSpawnedObjects(UMovieScene& SubMovieScene) override;
	virtual UObject* FindObject(const FGuid& ObjectId) const override;
	virtual FGuid FindObjectId(UObject& Object) const override;
	virtual UMovieScene* GetMovieScene() const override;
	virtual UObject* GetParentObject(UObject* Object) const override;
	virtual void SpawnOrDestroyObjects(UMovieScene* SubMovieScene, bool DestroyAll) override;
	virtual void UnbindPossessableObjects(const FGuid& ObjectId) override;

#if WITH_EDITOR
	virtual bool TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const override;
#endif

protected:

	/**
	 * Deselect all proxy actors in the Editor.
	 *
	 * @todo sequencer: remove Editor dependencies from this class
	 */
	ACTORANIMATION_API void DeselectAllActors();

private:

	/** Pointer to the movie scene that controls this animation. */
	UPROPERTY()
	UMovieScene* MovieScene;

	/** Collection of possessed objects. */
	// @todo sequencer: gmp: need support for UStruct keys in TMap
	UPROPERTY()
	TMap<FString, FActorAnimationObject> PossessedObjects;
	//TMap<FGuid, FSequencerBinding> Bindings;

	/** Collection of spawned proxy actors. */
	TMap<FGuid, TWeakObjectPtr<AActor>> SpawnedActors;
};
