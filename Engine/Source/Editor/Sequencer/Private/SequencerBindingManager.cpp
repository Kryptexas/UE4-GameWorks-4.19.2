// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "MovieScene.h"
#include "MovieSceneInstance.h"
#include "SequencerBinding.h"
#include "SequencerBindingManager.h"

// @todo sequencer: gmp: refactor these dependencies into another module
#include "ModuleManager.h"
#include "LevelEditor.h"


/* USequencerBindingManager structors
 *****************************************************************************/

USequencerBindingManager::USequencerBindingManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	//, ObjectChangeListener(InObjectChangeListener)
{
	// @todo sequencer: gmp: initialize ObjectChangeListener
	//ObjectChangeListener->GetOnPropagateObjectChanges().AddUObject(this, &USequencerBindingManager::HandlePropagateObjectChanges);
}


USequencerBindingManager::~USequencerBindingManager()
{
	if (ObjectChangeListener.IsValid())
	{
		ObjectChangeListener.Pin()->GetOnPropagateObjectChanges().RemoveAll(this);
	}
}


/* IMovieSceneBindingManager interface
 *****************************************************************************/

bool USequencerBindingManager::AllowsSpawnableObjects() const
{
	return true;
}


void USequencerBindingManager::BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject)
{
	FString ComponentName;
	UActorComponent* Component = Cast<UActorComponent>(&PossessedObject);

	if (Component != nullptr)
	{
		// @todo sequencer: gmp: need support for UStruct keys in TMap
		Bindings.Add(ObjectId.ToString(), FSequencerBinding(PossessedObject.GetOuter(), ComponentName));
		//Bindings.Add(ObjectId, FSequencerBinding(PossessedObject.GetOuter(), ComponentName));
	}
	else
	{
		// @todo sequencer: gmp: need support for UStruct keys in TMap
		Bindings.Add(ObjectId.ToString(), FSequencerBinding(&PossessedObject));
		//Bindings.Add(ObjectId, FSequencerBinding(Actor));
	}
}


bool USequencerBindingManager::CanPossessObject(UObject& Object) const
{
	return Object.IsA<AActor>();
}


void USequencerBindingManager::ClearBindings()
{
	Bindings.Empty();
}


void USequencerBindingManager::DestroyAllSpawnedObjects(UMovieScene& MovieScene)
{
	const bool DestroyAll = true;
	SpawnOrDestroyObjects(&MovieScene, DestroyAll);
	SpawnedActors.Empty();
}


FGuid USequencerBindingManager::FindObjectId(const UMovieScene& MovieScene, UObject& Object) const
{
	// search bindings (possessables)
	for (auto BindingPair : Bindings)
	{
		const FSequencerBinding& Binding = BindingPair.Value;

		if (Binding.GetBoundObject() == &Object)
		{
			FGuid Result; FGuid::Parse(BindingPair.Key, Result); return Result;
			//return BindingPair.Key;
		}
	}

	// search proxies (spawnables)
	const bool bIsGamePreviewObject = !!(Object.GetOutermost()->PackageFlags & PKG_PlayInEditor);

	if (bIsGamePreviewObject)
	{
		// @todo sequencer: gmp: i don't want this class to know the MovieScene that owns it
		//
		// someone is asking for a handle to an object from a game preview session,
		// probably because they want to capture keys during live simulation.
		//
		// Check to see if we already have a puppet that was generated from a
		// recording of this game preview object.
		//
		// @todo sequencer livecapture: We could support recalling counterpart by full name instead of weak pointer, to allow "overdubbing" of previously recorded actors, when the new actors in the current play session have the same path name
		// @todo sequencer livecapture: Ideally we could capture from editor-world actors that are "puppeteered" as well (real time)
		const FMovieSceneSpawnable* FoundSpawnable = MovieScene.FindSpawnableForCounterpart(&Object);
		
		if (FoundSpawnable != nullptr)
		{
			return FoundSpawnable->GetGuid();
		}
	}
	else
	{
		for (auto SpawnedActorPair : SpawnedActors)
		{
			if (SpawnedActorPair.Value.Get() == &Object)
			{
				return SpawnedActorPair.Key;
			}
		}
	}

	return FGuid();
}


UObject* USequencerBindingManager::FindObject(const FGuid& ObjectId) const
{
	// search bindings (possessables)
	// @todo sequencer: gmp: need support for UStruct keys in TMap
	const FSequencerBinding* Binding = Bindings.Find(ObjectId.ToString());
	//const FSequencerBinding* Binding = Bindings.Find(ObjectId);

	if (Binding != nullptr)
	{
		return Binding->GetBoundObject();
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


void USequencerBindingManager::SpawnOrDestroyObjects(UMovieScene* MovieScene, bool DestroyAll)
{
	bool bAnyLevelActorsChanged = false;

	// remove any proxy actors that we no longer need
	if (MovieScene != nullptr)
	{
		for (auto SpawnedActorPair : SpawnedActors)
		{
			bool ShouldDestroyActor = true;

			if (!DestroyAll)
			{
				// figure out if we still need this proxy actor
				for (auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex)
				{
					auto& Spawnable = MovieScene->GetSpawnable(SpawnableIndex);

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

	if (!DestroyAll && (MovieScene != nullptr))
	{
		UWorld* ActorWorld = GetWorld();

		for (auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex)
		{
			FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable( SpawnableIndex );

			// Must have a valid world for us to be able to do this
			if (ActorWorld == nullptr )
			{
				continue;
			}

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

			AActor* NewActor = ActorWorld->SpawnActor(GeneratedClass, &SpawnLocation, &SpawnRotation, SpawnInfo);
				
			if (NewActor != nullptr)
			{
				// @todo sequencer: We're naming the actor based off of the spawnable's name.  Is that really what we want?
				FActorLabelUtilities::SetActorLabelUnique(NewActor, Spawnable.GetName());
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


bool USequencerBindingManager::TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const
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


void USequencerBindingManager::UnbindPossessableObjects(const FGuid& ObjectId)
{
	// @todo sequencer: gmp: need support for UStruct keys in TMap
	Bindings.Remove(ObjectId.ToString());
	//Bindings.Remove(ObjectId);
}


/* USequencerBindingManager implementation
 *****************************************************************************/

void USequencerBindingManager::DeselectAllActors()
{
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
	}
}


UWorld* USequencerBindingManager::GetWorld() const
{
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>("LevelEditor");

		return LevelEditorModule.GetFirstLevelEditor()->GetWorld();
	}

	return nullptr;
}


/* USequencerBindingManager internals
 *****************************************************************************/

void USequencerBindingManager::HandlePropagateObjectChanges(UObject* Object)
{
	if (Object == nullptr)
	{
		return;
	}

	// @todo sequencer major: Many other types of changes to the puppet actor may occur which should be propagated (or disallowed?):
	//		- Deleting the puppet actor (Del key) should probably delete the reference actor and remove it from the MovieScene
	//		- Attachment changes
	//		- Replace/Convert operations?

	// We only care about actor objects for this type of spawner.  Note that sometimes we can get component objects passed in, but we'll
	// rely on the caller also calling OnObjectPropertyChange for the outer Actor if a component property changes.
	if (!Object->IsA<AActor>())
	{
		return;
	}

	for (auto SpawnedActorPair : SpawnedActors)
	{
		AActor* SpawnedActor = SpawnedActorPair.Value.Get();

		if (SpawnedActor == Object)
		{
			// @todo sequencer: propagation: Don't propagate changes that are being auto-keyed or key-adjusted!
			PropagateActorChanges(SpawnedActorPair.Key, SpawnedActor);
		}
	}
}


void USequencerBindingManager::PropagateActorChanges(const FGuid& ObjectId, AActor* Actor)
{
	if (Actor == nullptr)
	{
		return;
	}

	AActor* TargetActor = nullptr;
	FMovieSceneSpawnable* FoundSpawnable = nullptr;

	// find the spawnable for this puppet actor
	// @todo sequencer: gmp: add horrible sequencer dependencies to binding manager
/*	TArray<UMovieScene*> MovieScenesBeingEdited = Sequencer.Pin()->GetMovieScenesBeingEdited();

	for (UMovieScene* MovieScene : MovieScenesBeingEdited)
	{
		FoundSpawnable = MovieScene->FindSpawnable(ActorInfo->ObjectId);
		
		if (FoundSpawnable != nullptr)
		{
			break;
		}
	}
*/	
	if (ensure((FoundSpawnable != nullptr) && (Actor != nullptr)))
	{
		UClass* ActorClass = FoundSpawnable->GetClass();

		// the proxy actor's class should always be the BP that it was spawned from
		UClass* SpawnedActorClass = Actor->GetClass();
		check(ActorClass == SpawnedActorClass);

		// we'll be copying properties into the class default object of the Blueprint's generated class
		TargetActor = CastChecked<AActor>(ActorClass->GetDefaultObject());
	}

	if ((Actor != nullptr) && (TargetActor != nullptr))
	{
		// @todo sequencer: gmp: add horrible sequencer dependencies to binding manager
//		Sequencer.Pin()->CopyActorProperties(Actor, TargetActor);
	}
}
