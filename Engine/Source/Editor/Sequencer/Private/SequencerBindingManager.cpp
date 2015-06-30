// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "MovieScene.h"
#include "MovieSceneInstance.h"
#include "SequencerBindingManager.h"

// @todo sequencer: gmp: refactor these dependencies into another module
#include "ModuleManager.h"
#include "LevelEditor.h"


/* Private types
 *****************************************************************************/

/**
 * Puppet actor info
 */
struct FSequencerActorInfo
{
	/** Unique object identifier. */
	FMovieSceneObjectId ObjectId;

	/**
	 * The actual puppet actor that we are actively managing.
	 * These actors actually live in the editor level just like any other actor,
	 * but we'll directly manage their lifetimes while in the editor.
	 */
	TWeakObjectPtr<AActor> Actor;	
};


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

void USequencerBindingManager::BindPossessableObject(const FMovieSceneObjectId& ObjectId, UObject& PossessedObject)
{
	// @todo sequencer: gmp: bind possessable object
}


bool USequencerBindingManager::CanPossessObject(UObject& Object) const
{
	return Object.IsA<AActor>();
}


void USequencerBindingManager::DestroyAllSpawnedObjects()
{
	const bool bDestroyAll = true;

	for (auto It = InstanceToActorInfos.CreateConstIterator(); It; ++It)
	{
		SpawnOrDestroyObjectsForInstance(It.Key().Pin().ToSharedRef(), bDestroyAll);
	}

	InstanceToActorInfos.Empty();
}


FMovieSceneObjectId USequencerBindingManager::FindIdForObject(const UMovieScene& MovieScene, UObject& Object) const
{
	FMovieSceneObjectId Result = FindIdForSpawnableObject(Object);

	if (Result.IsValid())
	{
		return Result;
	}

	const bool bIsGamePreviewObject = !!(Object.GetOutermost()->PackageFlags & PKG_PlayInEditor);

	if (bIsGamePreviewObject)
	{
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
			Result = FoundSpawnable->GetObjectId();
		}
	}
	else
	{
		// @todo sequencer: gmp: update bindings
/*		BindToPlayMovieSceneNode(false);

		// when editing within the level editor, make sure we're bound to the level
		// script node which contains data about possessables.
		if (PlayMovieSceneNode.IsValid())
		{
			ObjectGuid = PlayMovieSceneNode->FindGuidForObject(&Object);
		}*/
	}

	return Result;
}


void USequencerBindingManager::GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, TArray<UObject*>& OutRuntimeObjects) const
{
	UObject* Object = FindActorInfoById(MovieSceneInstance, ObjectId);

	if (Object)
	{
		// spawnable object
		OutRuntimeObjects.Reset();
		OutRuntimeObjects.Add(Object);
	}
	else
	{
		// possessable object
		// @todo sequenceR: gmp: bind object?
/*		if (!PlayMovieSceneNode.IsValid())
		{
			BindToPlayMovieSceneNode(false);
		}
	
		if (PlayMovieSceneNode.IsValid())
		{
			OutRuntimeObjects = PlayMovieSceneNode->FindBoundObjects( ObjectGuid );
		}*/
	}
}


void USequencerBindingManager::RemoveMovieSceneInstance(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance)
{
	SpawnOrDestroyObjectsForInstance(MovieSceneInstance, true);
	InstanceToActorInfos.Remove(MovieSceneInstance);
}


void USequencerBindingManager::SpawnOrDestroyObjectsForInstance(TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll)
{
	bool bAnyLevelActorsChanged = false;

	// Get the list of puppet objects for the movie scene
	TArray<TSharedRef<FSequencerActorInfo>>& ActorInfos = InstanceToActorInfos.FindOrAdd(MovieSceneInstance);
	UMovieScene* MovieScene = MovieSceneInstance->GetMovieScene();
	UWorld* ActorWorld = GetWorld();

	// remove any puppet objects that we no longer need
	if (MovieScene != nullptr)
	{
		for (auto ActorInfoIndex = 0; ActorInfoIndex < ActorInfos.Num(); ++ActorInfoIndex)
		{
			TSharedRef<FSequencerActorInfo> ActorInfo = ActorInfos[ActorInfoIndex];
			bool bShouldDestroyActor = true;

			// figure out if we still need this puppet actor
			if (!bDestroyAll)
			{
				for (auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex)
				{
					auto& Spawnable = MovieScene->GetSpawnable(SpawnableIndex);

					if (Spawnable.GetObjectId() == ActorInfo->ObjectId)
					{
						bShouldDestroyActor = false;
						break;
					}
				}
			}

			if (!bShouldDestroyActor)
			{
				// Actor is no longer valid (probably the world was destroyed)
				continue;
			}

			AActor* Actor = ActorInfo->Actor.Get();
				
			if (Actor == nullptr)
			{
				continue;
			}

			UWorld* PuppetWorld = Actor->GetWorld();

			if (ensure(PuppetWorld != nullptr))
			{
				// destroy this actor
				// Ignored unless called while game is running
				const bool bNetForce = false;

				// We don't want to dirty the level for puppet actor changes
				const bool bShouldModifyLevel = false;

				if (!bAnyLevelActorsChanged)
				{
					DeselectAllActors();
				}

				// Actor should never be selected in the editor at this point.  We took care of that up above.
				ensure(!Actor->IsSelected());

				const bool bWasDestroyed = PuppetWorld->DestroyActor(Actor, bNetForce, bShouldModifyLevel);

				if (bWasDestroyed)
				{
					bAnyLevelActorsChanged = true;
					ActorInfos.RemoveAt(ActorInfoIndex--);
				}
				else
				{
					// @todo sequencer: At least one puppet couldn't be cleaned up!
				}
			}
		}
	}

	if( !bDestroyAll && MovieScene )
	{
		for (auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex)
		{
			FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable( SpawnableIndex );

			// Must have a valid world for us to be able to do this
			if (ActorWorld == nullptr )
			{
				continue;
			}

			// Do we already have a puppet for this spawnable?
			bool bIsAlreadySpawned = false;

			for (auto PuppetIndex = 0; PuppetIndex < ActorInfos.Num(); ++PuppetIndex)
			{
				auto& ActorInfo = ActorInfos[PuppetIndex];

				if (ActorInfo->ObjectId == Spawnable.GetObjectId())
				{
					bIsAlreadySpawned = true;

					break;
				}
			}

			if (bIsAlreadySpawned)
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

				// Keep track of this actor
				TSharedRef<FSequencerActorInfo> NewActorInfo(new FSequencerActorInfo());
				{
					NewActorInfo->ObjectId = Spawnable.GetObjectId();
					NewActorInfo->Actor = NewActor;
					ActorInfos.Add(NewActorInfo);
				}
			}
			else
			{
				// Actor failed to spawn
				// @todo sequencer: What should we do when this happens to one or more actors?
			}
		}
	}
}


bool USequencerBindingManager::TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, FText& OutDisplayName) const
{
	TArray<UObject*> RuntimeObjects;
	GetRuntimeObjects(MovieSceneInstance, ObjectId, RuntimeObjects);

	for (int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex)
	{
		AActor* Actor = Cast<AActor>(RuntimeObjects[ObjIndex]);

		if (Actor != nullptr)
		{
			OutDisplayName = FText::FromString(Actor->GetActorLabel());

			return true;
		}
	}

	return false;
}


void USequencerBindingManager::UnbindPossessableObjects(const FMovieSceneObjectId& ObjectId)
{
	// @todo sequencer: gmp: unbind object
}


/* USequencerBindingManager implementation
 *****************************************************************************/

void USequencerBindingManager::DeselectAllActors()
{
	TArray<AActor*> ActorsToDeselect;

	// find actors to deselect
	for (auto ActorInfosPair : InstanceToActorInfos)
	{
		for (const TSharedRef<FSequencerActorInfo>& ActorInfo : ActorInfosPair.Value)
		{
			AActor* Actor = ActorInfo->Actor.Get();

			if (Actor != nullptr)
			{
				ActorsToDeselect.Add(Actor);
			}
		}
	}

	if (ActorsToDeselect.Num() == 0)
	{
		return;
	}

	// deselect actors
	USelection* Selection = GEditor->GetSelectedActors();
	{
		Selection->BeginBatchSelectOperation();
		Selection->MarkBatchDirty();

		for (AActor* Actor : ActorsToDeselect)
		{
			GEditor->GetSelectedActors()->Deselect(Actor);
		}

		Selection->EndBatchSelectOperation();
	}
}


FMovieSceneObjectId USequencerBindingManager::FindIdForSpawnableObject(UObject& Object) const
{
	for (auto ActorInfosPair : InstanceToActorInfos)
	{
		for (const TSharedRef<FSequencerActorInfo>& ActorInfo : ActorInfosPair.Value)
		{
			AActor* Actor = ActorInfo->Actor.Get();

			if (Actor == &Object)
			{
				return ActorInfo->ObjectId;
			}
		}
	}

	// not found
	return FMovieSceneObjectId();
}


UObject* USequencerBindingManager::FindActorInfoById(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId) const
{
	const TArray<TSharedRef<FSequencerActorInfo>>* ActorInfos = InstanceToActorInfos.Find(MovieSceneInstance);

	if (ActorInfos == nullptr)
	{
		return nullptr;
	}

	for (TSharedRef<FSequencerActorInfo> ActorInfo : *ActorInfos)
	{
		if (ActorInfo->ObjectId == ObjectId)
		{
			return ActorInfo->Actor.Get();
		}
	}

	// not found
	return nullptr;
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

	for (auto ActorInfosPair : InstanceToActorInfos)
	{
		// is this an object that we care about?
		for (const TSharedRef<FSequencerActorInfo>& ActorInfo : ActorInfosPair.Value)
		{
			AActor* Actor = ActorInfo->Actor.Get();

			if (Actor == Object)
			{
				// @todo sequencer: propagation: Don't propagate changes that are being auto-keyed or key-adjusted!

				// Our puppet actor was modified.
				PropagateActorChanges(ActorInfo);
			}
		}
	}
}


void USequencerBindingManager::PropagateActorChanges(const TSharedRef<FSequencerActorInfo> ActorInfo)
{
	AActor* Actor = ActorInfo->Actor.Get();
	AActor* TargetActor = nullptr;

	// find the spawnable for this puppet actor
	FMovieSceneSpawnable* FoundSpawnable = nullptr;

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
