// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NiagaraPrivate.h"
#include "NiagaraAnimation.h"


/* UNiagaraAnimation structors
 *****************************************************************************/

UNiagaraAnimation::UNiagaraAnimation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MovieScene(nullptr)
{ }


/* IMovieSceneAnimation interface
 *****************************************************************************/

bool UNiagaraAnimation::AllowsSpawnableObjects() const
{
	return false;
}


void UNiagaraAnimation::BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject)
{
}


bool UNiagaraAnimation::CanPossessObject(UObject& Object) const
{
	return false;
}


void UNiagaraAnimation::DestroyAllSpawnedObjects(UMovieScene& SubMovieScene)
{
}


UObject* UNiagaraAnimation::FindObject(const FGuid& ObjectId) const
{
	return nullptr;
}


FGuid UNiagaraAnimation::FindObjectId(UObject& Object) const
{
	return FGuid();
}


UMovieScene* UNiagaraAnimation::GetMovieScene() const
{
	return MovieScene;
}


UObject* UNiagaraAnimation::GetParentObject(UObject* Object) const
{
	return nullptr;
}


void UNiagaraAnimation::SpawnOrDestroyObjects(UMovieScene* SubMovieScene, bool DestroyAll)
{
}


#if WITH_EDITOR
bool UNiagaraAnimation::TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const
{
	return false;
}
#endif


void UNiagaraAnimation::UnbindPossessableObjects(const FGuid& ObjectId)
{
}
