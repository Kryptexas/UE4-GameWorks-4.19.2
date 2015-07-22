// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationPrivatePCH.h"
#include "ActorAnimation.h"
#include "ActorAnimationObject.h"
#include "MovieScene.h"


/* UActorAnimation structors
 *****************************************************************************/

UActorAnimation::UActorAnimation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MovieScene(nullptr)
{ }


/* UActorAnimation interface
 *****************************************************************************/

void UActorAnimation::Initialize()
{
	// @todo sequencer: gmp: fix me
	MovieScene = NewObject<UMovieScene>(this);
}


/* IMovieSceneAnimation interface
 *****************************************************************************/

bool UActorAnimation::AllowsSpawnableObjects() const
{
	return true;
}


void UActorAnimation::BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject)
{
	FString ComponentName;
	UActorComponent* Component = Cast<UActorComponent>(&PossessedObject);

	if (Component != nullptr)
	{
		// @todo sequencer: gmp: need support for UStruct keys in TMap
		PossessedObjects.Add(ObjectId.ToString(), FActorAnimationObject(PossessedObject.GetOuter(), ComponentName));
		//PossessedObjects.Add(ObjectId, FSequencerPossessedObject(PossessedObject.GetOuter(), ComponentName));
	}
	else
	{
		// @todo sequencer: gmp: need support for UStruct keys in TMap
		PossessedObjects.Add(ObjectId.ToString(), FActorAnimationObject(&PossessedObject));
		//PossessedObjects.Add(ObjectId, FSequencerPossessedObject(Actor));
	}
}


bool UActorAnimation::CanPossessObject(UObject& Object) const
{
	return Object.IsA<AActor>();
}


void UActorAnimation::DestroyAllSpawnedObjects(UMovieScene& SubMovieScene)
{
	const bool DestroyAll = true;
	SpawnOrDestroyObjects(&SubMovieScene, DestroyAll);
	SpawnedActors.Empty();
}


UObject* UActorAnimation::FindObject(const FGuid& ObjectId) const
{
	// search bindings (possessables)
	// @todo sequencer: gmp: need support for UStruct keys in TMap
	const FActorAnimationObject* PossessedObject = PossessedObjects.Find(ObjectId.ToString());
	//const FSequencerPossessedObject* PossessedObject = PossessedObjects.Find(ObjectId);

	if (PossessedObject != nullptr)
	{
		return PossessedObject->GetObject();
	}

	// search proxies (spawnables)
	TWeakObjectPtr<AActor> Actor = SpawnedActors.FindRef(ObjectId);

	if (Actor.IsValid())
	{
		return Actor.Get();
	}

	// not found
	return nullptr;
}


FGuid UActorAnimation::FindObjectId(UObject& Object) const
{
	// search possessed objects
	for (auto PossessedObjectPair : PossessedObjects)
	{
		const FActorAnimationObject& PossessedObject = PossessedObjectPair.Value;

		if (PossessedObject.GetObject() == &Object)
		{
			// @todo sequencer: gmp: need support for UStruct keys in TMap
			FGuid Result; FGuid::Parse(PossessedObjectPair.Key, Result); return Result;
			//return PossessedObjectsPair.Key;
		}
	}

	// search spawned objects
	for (auto SpawnedActorPair : SpawnedActors)
	{
		if (SpawnedActorPair.Value.Get() == &Object)
		{
			return SpawnedActorPair.Key;
		}
	}

	// not found
	return FGuid();
}


UMovieScene* UActorAnimation::GetMovieScene() const
{
	return MovieScene;
}


UObject* UActorAnimation::GetParentObject(UObject* Object) const
{
	UActorComponent* Component = Cast<UActorComponent>(Object);

	if (Component != nullptr)
	{
		return Component->GetOwner();
	}

	return nullptr;
}


void UActorAnimation::SpawnOrDestroyObjects(UMovieScene* SubMovieScene, bool DestroyAll)
{
	bool bAnyLevelActorsChanged = false;

	// remove any proxy actors that we no longer need
	if (SubMovieScene != nullptr)
	{
		for (auto SpawnedActorPair : SpawnedActors)
		{
			bool ShouldDestroyActor = true;

			if (!DestroyAll)
			{
				// figure out if we still need this proxy actor
				for (auto SpawnableIndex = 0; SpawnableIndex < SubMovieScene->GetSpawnableCount(); ++SpawnableIndex)
				{
					auto& Spawnable = SubMovieScene->GetSpawnable(SpawnableIndex);

					if (Spawnable.GetGuid() == SpawnedActorPair.Key)
					{
						ShouldDestroyActor = false;
						break;
					}
				}
			}

			if (!ShouldDestroyActor)
			{
				// Actor is no longer valid (probably the world was destroyed)
				continue;
			}

			AActor* Actor = SpawnedActorPair.Value.Get();
				
			if (Actor == nullptr)
			{
				continue;
			}

			// destroy Actor
			UWorld* World = Actor->GetWorld();

			if (ensure(World != nullptr))
			{
				if (!bAnyLevelActorsChanged)
				{
					DeselectAllActors();
				}

				// Actor should never be selected in the Editor at this point (we took care of that up above)
				ensure(!Actor->IsSelected());

				const bool bNetForce = false;
				const bool bShouldModifyLevel = false;
				const bool bWasDestroyed = World->DestroyActor(Actor, bNetForce, bShouldModifyLevel);

				if (bWasDestroyed)
				{
					bAnyLevelActorsChanged = true;
					SpawnedActors.Remove(SpawnedActorPair.Key);
				}
				else
				{
					// @todo sequencer: At least one proxy actor couldn't be cleaned up!
				}
			}
		}
	}

	if (!DestroyAll && (SubMovieScene != nullptr))
	{
		for (auto SpawnableIndex = 0; SpawnableIndex < SubMovieScene->GetSpawnableCount(); ++SpawnableIndex)
		{
			FMovieSceneSpawnable& Spawnable = SubMovieScene->GetSpawnable(SpawnableIndex);

			// Do we already have a proxy for this spawnable?
			if (SpawnedActors.Contains(Spawnable.GetGuid()))
			{
				continue;
			}

			UClass* GeneratedClass = Spawnable.GetClass();

			if ((GeneratedClass == nullptr) || !GeneratedClass->IsChildOf(AActor::StaticClass()))
			{
				continue;
			}

			AActor* ActorCDO = CastChecked<AActor>(GeneratedClass->ClassDefaultObject);

			const FVector SpawnLocation = ActorCDO->GetRootComponent()->RelativeLocation;
			const FRotator SpawnRotation = ActorCDO->GetRootComponent()->RelativeRotation;

			// @todo sequencer: We should probably spawn these in a specific sub-level!
			// World->CurrentLevel = ???;

			const FName ActorName = NAME_None;

			// Override the object flags so that RF_Transactional is not set.  Puppet actors are never transactional
			// @todo sequencer: These actors need to avoid any transaction history.  However, RF_Transactional can currently be set on objects on the fly!
			const EObjectFlags ObjectFlags = RF_Transient;		// NOTE: We are omitting RF_Transactional intentionally

			// @todo sequencer livecapture: Consider using SetPlayInEditorWorld() and RestoreEditorWorld() here instead
						
			// @todo sequencer actors: We need to make sure puppet objects aren't copied into PIE/SIE sessions!  They should be omitted from that duplication!

			// Spawn the puppet actor
			FActorSpawnParameters SpawnInfo;
			{
				SpawnInfo.Name = ActorName;
				SpawnInfo.ObjectFlags = ObjectFlags;
			}

			AActor* NewActor = GWorld->SpawnActor(GeneratedClass, &SpawnLocation, &SpawnRotation, SpawnInfo);
				
			if (NewActor != nullptr)
			{
				// @todo sequencer: We're naming the actor based off of the spawnable's name.  Is that really what we want?
				// @todo sequencer: gmp: fix me
				//FActorLabelUtilities::SetActorLabelUnique(NewActor, Spawnable.GetName());
				SpawnedActors.Add(Spawnable.GetGuid(), NewActor);
			}
			else
			{
				// Actor failed to spawn
				// @todo sequencer: What should we do when this happens to one or more actors?
			}
		}
	}
}


#if WITH_EDITOR
bool UActorAnimation::TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const
{
	UObject* Object = FindObject(ObjectId);

	if (Object == nullptr)
	{
		return false;
	}

	AActor* Actor = Cast<AActor>(Object);

	if (Actor == nullptr)
	{
		return false;
	}

	OutDisplayName = FText::FromString(Actor->GetActorLabel());

	return true;
}
#endif


void UActorAnimation::UnbindPossessableObjects(const FGuid& ObjectId)
{
	// @todo sequencer: gmp: need support for UStruct keys in TMap
	PossessedObjects.Remove(ObjectId.ToString());
	//PossessedObjects.Remove(ObjectId);
}


/* UActorAnimation implementation
 *****************************************************************************/

void UActorAnimation::DeselectAllActors()
{
	// @todo sequencer: gmp: fix me
	/*
	USelection* Selection = GEditor->GetSelectedActors();
	{
		Selection->BeginBatchSelectOperation();
		Selection->MarkBatchDirty();

		for (auto SpawnedActorPair : SpawnedActors)
		{
			AActor* SpawnedActor = SpawnedActorPair.Value.Get();

			if (SpawnedActor != nullptr)
			{
				GEditor->GetSelectedActors()->Deselect(SpawnedActor);
			}
		}

		Selection->EndBatchSelectOperation();
	}*/
}
