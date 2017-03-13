// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ControlRigObjectSpawner.h"
#include "ControlRig.h"
#include "MovieSceneSpawnable.h"
#include "IMovieScenePlayer.h"
#include "Package.h"

TSharedRef<IMovieSceneObjectSpawner> FControlRigObjectSpawner::CreateObjectSpawner()
{
	return MakeShareable(new FControlRigObjectSpawner);
}

UClass* FControlRigObjectSpawner::GetSupportedTemplateType() const
{
	return UControlRig::StaticClass();
}

UObject* FControlRigObjectSpawner::SpawnObject(FMovieSceneSpawnable& Spawnable, FMovieSceneSequenceIDRef TemplateID, IMovieScenePlayer& Player)
{
	UObject* ObjectTemplate = Spawnable.GetObjectTemplate();

	if (UControlRig* ControlRig = Cast<UControlRig>(ObjectTemplate))
	{
		FName ObjectName = ControlRig->GetClass()->GetFName();
		UControlRig* SpawnedObject = NewObject<UControlRig>(Player.GetPlaybackContext(), ControlRig->GetClass(), ObjectName, RF_Transient);
		SpawnedObject->AddToRoot();
		SpawnedObject->Initialize();

		return SpawnedObject;
	}

	return nullptr;
}

void FControlRigObjectSpawner::DestroySpawnedObject(UObject& Object)
{
	if (UControlRig* ControlRig = Cast<UControlRig>(&Object))
	{
		ControlRig->RemoveFromRoot();
		ControlRig->Rename(nullptr, GetTransientPackage());
		ControlRig->MarkPendingKill();
	}
}
