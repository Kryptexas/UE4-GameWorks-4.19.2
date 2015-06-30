// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBindingManager.h"
#include "UMGBindingManager.generated.h"


/**
 * Movie scene binding manager used by Sequencer.
 */
UCLASS(BlueprintType, MinimalAPI)
class UUMGBindingManager
	: public UObject
	, public IMovieSceneBindingManager
{
	GENERATED_UCLASS_BODY()

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
};
