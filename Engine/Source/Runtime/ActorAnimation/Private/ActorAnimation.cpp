// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationPrivatePCH.h"
#include "ActorAnimation.h"
#include "ActorAnimationObject.h"
#include "MovieScene.h"


UActorAnimation::UActorAnimation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MovieScene(nullptr)
{
}

void UActorAnimation::Initialize()
{
	// @todo sequencer: gmp: fix me
	MovieScene = NewObject<UMovieScene>(this, NAME_None, RF_Transactional);
}

void UActorAnimation::ConvertPersistentBindingsToDefault(UObject* FixupContext)
{
	if (PossessedObjects_DEPRECATED.Num() == 0)
	{
		return;
	}

	MarkPackageDirty();
	for (auto& Pair : PossessedObjects_DEPRECATED)
	{
		UObject* Object = Pair.Value.GetObject();
		if (Object)
		{
			FGuid ObjectId;
			FGuid::Parse(Pair.Key, ObjectId);
			DefaultObjectReferences.CreateBinding(ObjectId, Object, FixupContext);
		}
	}
	PossessedObjects_DEPRECATED.Empty();
}

void UActorAnimationInstance::Initialize(UActorAnimation* InAnimation, UObject* InContext, bool bInCanRemapBindings)
{
	ActorAnimation = InAnimation;
	bCanRemapBindings = bInCanRemapBindings;

	if (!bCanRemapBindings)
	{
		RemappedObjectReferences = FActorAnimationObjectReferenceMap();
	}

	SetContext(InContext);
}

void UActorAnimationInstance::SetContext(UObject* NewContext)
{
	ResolutionContext = NewContext;
	CachedObjectBindings.Reset();
}

bool UActorAnimationInstance::AllowsSpawnableObjects() const
{
	return true;
}

void UActorAnimationInstance::BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject)
{
	UObject* Context = ResolutionContext.Get();
	if (!Context)
	{
		return;
	}

	if (!ActorAnimation)
	{
		RemappedObjectReferences.CreateBinding(ObjectId, &PossessedObject, Context);
	}
	else if (bCanRemapBindings && ActorAnimation->DefaultObjectReferences.HasBinding(ObjectId))
	{
		RemappedObjectReferences.CreateBinding(ObjectId, &PossessedObject, Context);
	}
	else
	{
		ActorAnimation->DefaultObjectReferences.CreateBinding(ObjectId, &PossessedObject, Context);
	}
}

bool UActorAnimationInstance::CanPossessObject(UObject& Object) const
{
	return Object.IsA<AActor>() || Object.IsA<UActorComponent>();
}

void UActorAnimationInstance::DestroyAllSpawnedObjects()
{
	const bool DestroyAll = true;
	SpawnOrDestroyObjects(DestroyAll);
	SpawnedActors.Empty();
}

UObject* UActorAnimationInstance::FindObject(const FGuid& ObjectId) const
{
	// If it's already cached, we can just return that
	if (auto* WeakCachedObject = CachedObjectBindings.Find(ObjectId))
	{
		if (auto* CachedObj = WeakCachedObject->Get())
		{
			return CachedObj;
		}
	}

	// Look up the object from scratch
	UObject* FoundObject = nullptr;

	// Otherwise we need to try and find it
	UObject* Context = ResolutionContext.Get();
	if (!FoundObject && Context)
	{
		if (!FoundObject && bCanRemapBindings)
		{
			// Attempt to resolve the object binding through the remapped bindings
			FoundObject = RemappedObjectReferences.ResolveBinding(ObjectId, Context);
		}
		
		if (ActorAnimation)
		{
			// Attempt to resolve the object binding through the defaults
			FoundObject = ActorAnimation->DefaultObjectReferences.ResolveBinding(ObjectId, Context);
		}
	}

	if (!FoundObject)
	{
		// Maybe it's a spawnable then?
		FoundObject = SpawnedActors.FindRef(ObjectId).Get();
	}

	if (FoundObject)
	{
		CachedObjectBindings.Add(ObjectId, FoundObject);
	}

	return FoundObject;
}

FGuid UActorAnimationInstance::FindObjectId(UObject& Object) const
{
	for (auto& Pair : CachedObjectBindings)
	{
		if (Pair.Value == &Object)
		{
			return Pair.Key;
		}
	}

	// search possessed objects
	UObject* Context = ResolutionContext.Get();
	if (Context)
	{
		if (bCanRemapBindings)
		{
			// Search instances
			FGuid ObjectId = RemappedObjectReferences.FindBindingId(&Object, Context);
			if (ObjectId.IsValid())
			{
				return ObjectId;
			}
		}

		// Search defaults
		if (ActorAnimation)
		{
			FGuid ObjectId = ActorAnimation->DefaultObjectReferences.FindBindingId(&Object, Context);
			if (ObjectId.IsValid())
			{
				return ObjectId;
			}
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

UMovieScene* UActorAnimationInstance::GetMovieScene() const
{
	return ActorAnimation ? ActorAnimation->MovieScene : nullptr;
}

UObject* UActorAnimationInstance::GetParentObject(UObject* Object) const
{
	UActorComponent* Component = Cast<UActorComponent>(Object);

	if (Component != nullptr)
	{
		return Component->GetOwner();
	}

	return nullptr;
}

void UActorAnimationInstance::SpawnOrDestroyObjects(bool DestroyAll)
{
	bool bAnyLevelActorsChanged = false;

	UMovieScene* MovieScene = GetMovieScene();
	
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
		for (auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex)
		{
			FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(SpawnableIndex);

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
bool UActorAnimationInstance::TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const
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

void UActorAnimationInstance::UnbindPossessableObjects(const FGuid& ObjectId)
{
	if (bCanRemapBindings && RemappedObjectReferences.HasBinding(ObjectId))
	{
		RemappedObjectReferences.RemoveBinding(ObjectId);
	}
	else if (ActorAnimation)
	{
		ActorAnimation->DefaultObjectReferences.RemoveBinding(ObjectId);
	}
}

void UActorAnimationInstance::DeselectAllActors()
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