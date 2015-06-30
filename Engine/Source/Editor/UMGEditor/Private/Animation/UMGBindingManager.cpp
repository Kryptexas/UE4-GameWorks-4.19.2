// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "MovieScene.h"
#include "MovieSceneInstance.h"
#include "UMGBindingManager.h"


/* UUMGBindingManager structors
 *****************************************************************************/

UUMGBindingManager::UUMGBindingManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ }


/* IMovieSceneBindingManager interface
 *****************************************************************************/

bool UUMGBindingManager::AllowsSpawnableObjects() const
{
	return false;
}


void UUMGBindingManager::BindPossessableObject(const FMovieSceneObjectId& ObjectId, UObject& PossessedObject)
{
	// @todo gmp: sequencer: bind possessable object
}


bool UUMGBindingManager::CanPossessObject(UObject& Object) const
{
	return false;
}


void UUMGBindingManager::DestroyAllSpawnedObjects()
{

}


FMovieSceneObjectId UUMGBindingManager::FindIdForObject(const UMovieScene& MovieScene, UObject& Object) const
{
	return FMovieSceneObjectId();
}


void UUMGBindingManager::GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, TArray<UObject*>& OutRuntimeObjects) const
{

}


void UUMGBindingManager::RemoveMovieSceneInstance(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance)
{

}


void UUMGBindingManager::SpawnOrDestroyObjectsForInstance(TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll)
{

}


bool UUMGBindingManager::TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, FText& OutDisplayName) const
{
	return false;
}


void UUMGBindingManager::UnbindPossessableObjects(const FMovieSceneObjectId& ObjectId)
{
	// @todo gmp: sequencer: bind possessable object
}
