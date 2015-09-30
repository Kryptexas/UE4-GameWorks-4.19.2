// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NiagaraPrivate.h"
#include "NiagaraSequence.h"


/* UNiagaraSequence structors
 *****************************************************************************/

UNiagaraSequence::UNiagaraSequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MovieScene(nullptr)
{ }


/* UMovieSceneAnimation overrides
 *****************************************************************************/

bool UNiagaraSequence::AllowsSpawnableObjects() const
{
	return false;
}


void UNiagaraSequence::BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject)
{
}


bool UNiagaraSequence::CanPossessObject(UObject& Object) const
{
	return false;
}


void UNiagaraSequence::DestroyAllSpawnedObjects()
{
}


UObject* UNiagaraSequence::FindObject(const FGuid& ObjectId) const
{
	return nullptr;
}


FGuid UNiagaraSequence::FindObjectId(UObject& Object) const
{
	return FGuid();
}


UMovieScene* UNiagaraSequence::GetMovieScene() const
{
	return MovieScene;
}


UObject* UNiagaraSequence::GetParentObject(UObject* Object) const
{
	return nullptr;
}


void UNiagaraSequence::SpawnOrDestroyObjects(bool DestroyAll)
{
}


#if WITH_EDITOR
bool UNiagaraSequence::TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const
{
	return false;
}
#endif


void UNiagaraSequence::UnbindPossessableObjects(const FGuid& ObjectId)
{
}
