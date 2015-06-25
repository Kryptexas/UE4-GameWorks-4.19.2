// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "MovieScene.h"
#include "MovieSceneInstance.h"
#include "SequencerBindingManager.h"


/* USequencerBindingManager structors
 *****************************************************************************/

USequencerBindingManager::USequencerBindingManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ }


/* IMovieSceneBindingManager interface
 *****************************************************************************/

void USequencerBindingManager::BindPossessableObject(const FMovieSceneObjectId& PossessableGuid, UObject& PossessedObject)
{

}


bool USequencerBindingManager::CanPossessObject(UObject& Object) const
{
	return false;
}


FMovieSceneObjectId USequencerBindingManager::FindGuidForObject(const UMovieScene& MovieScene, UObject& Object) const
{
	return FMovieSceneObjectId();
}


void USequencerBindingManager::GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects) const
{

}


bool USequencerBindingManager::TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, FText& DisplayName) const
{
	return false;
}


void USequencerBindingManager::UnbindPossessableObjects(const FMovieSceneObjectId& PossessableGuid)
{

}
