// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencerObjectBindingManager.h"

class FUMGSequencerObjectBindingManager : public ISequencerObjectBindingManager
{
public:
	FUMGSequencerObjectBindingManager( UWidgetBlueprint& InWidgetBlueprint );
	
	/** ISequencerObjectBindingManager interface */
	virtual bool AllowsSpawnableObjects() const override { return false; }
	virtual FGuid FindGuidForObject( const UMovieScene& MovieScene, UObject& Object ) const override;
	virtual void SpawnOrDestroyObjectsForInstance( TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll ) {}
	virtual void DestroyAllSpawnedObjects()  {}
	virtual bool CanPossessObject( UObject& Object ) const override;
	virtual void BindPossessableObject( const FGuid& PossessableGuid, UObject& PossessedObject ) override;
	virtual void UnbindPossessableObjects( const FGuid& PossessableGuid ) override;
	virtual void GetRuntimeObjects( const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects ) const override;
	
private:
	/** Mapping of preview objects to sequencer guids */
	TMap< TWeakObjectPtr<UObject>, FGuid> PreviewObjectToGuidMap;
	
	TWeakObjectPtr<UWidgetBlueprint> WidgetBlueprint;
};
