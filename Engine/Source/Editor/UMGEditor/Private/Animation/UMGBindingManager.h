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

	virtual void BindPossessableObject(const FMovieSceneObjectId& PossessableGuid, UObject& PossessedObject) override;
	virtual bool CanPossessObject( UObject& Object ) const override;
	virtual FMovieSceneObjectId FindGuidForObject(const UMovieScene& MovieScene, UObject& Object) const override;
	virtual void GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects) const override;
	virtual bool TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, FText& DisplayName) const override;
	virtual void UnbindPossessableObjects(const FMovieSceneObjectId& PossessableGuid) override;
};
