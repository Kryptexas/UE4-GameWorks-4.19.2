// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UChildActorComponent::UChildActorComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UChildActorComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	CreateChildActor();
}

void UChildActorComponent::OnComponentDestroyed()
{
	Super::OnComponentDestroyed();

	DestroyChildActor();
}

void UChildActorComponent::CreateChildActor()
{
	// Kill spawned actor if we have one
	DestroyChildActor();

	// If we have a class to spawn.
	if(ChildActorClass != NULL)
	{
		UWorld* World = GetWorld();
		if(World != NULL)
		{
			FActorSpawnParameters Params;
			Params.bNoCollisionFail = true;
			Params.bDeferConstruction = true; // We defer construction so that we set ParentComponentActor prior to component registration so they appear selected
			Params.bAllowDuringConstructionScript = true;

			// Spawn actor of desired class
			FVector Location = GetComponentLocation();
			FRotator Rotation = GetComponentRotation();
			ChildActor = World->SpawnActor(ChildActorClass, &Location, &Rotation, Params);

			// If spawn was successful, 
			if(ChildActor != NULL)
			{
#if WITH_EDITOR
				// Remember which actor spawned it (for selection in editor etc)
				ChildActor->ParentComponentActor = GetOwner();
#endif

				// attach new actor to this component
				ChildActor->AttachRootComponentTo(this, NAME_None, EAttachLocation::SnapToTarget);

				// Parts that we deferred from SpawnActor
				ChildActor->OnConstruction(ComponentToWorld);
				ChildActor->PostActorConstruction();
			}
		}
	}
}

void UChildActorComponent::DestroyChildActor()
{
	// If we own an Actor, kill it now
	if(ChildActor != NULL)
	{
		// if still alive, destroy, otherwise just clear the pointer
		if(!ChildActor->IsPendingKill())
		{
			UWorld* World = ChildActor->GetWorld();
			// World may be NULL during shutdown
			if(World != NULL)
			{
				World->DestroyActor(ChildActor);
			}
		}

		ChildActor = NULL;
	}
}
