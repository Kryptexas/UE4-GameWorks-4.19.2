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

void UUMGBindingManager::BindPossessableObject(const FGuid& PossessableGuid, UObject& PossessedObject)
{

}


bool UUMGBindingManager::CanPossessObject(UObject& Object) const
{
	return false;
}


FGuid UUMGBindingManager::FindGuidForObject(const UMovieScene& MovieScene, UObject& Object) const
{
	return FGuid();
}


void UUMGBindingManager::GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects) const
{

}


bool UUMGBindingManager::TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, FText& DisplayName) const
{
	return false;
}


void UUMGBindingManager::UnbindPossessableObjects(const FGuid& PossessableGuid)
{

}
