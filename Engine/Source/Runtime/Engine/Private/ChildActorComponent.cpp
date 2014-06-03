// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ComponentInstanceDataCache.h"

DEFINE_LOG_CATEGORY_STATIC(LogChildActorComponent, Warning, All);

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

class FChildActorComponentInstanceData : public FComponentInstanceDataBase
{
public:
	FChildActorComponentInstanceData(const UChildActorComponent* Component)
		: FComponentInstanceDataBase(Component)
		, ChildActorName(Component->ChildActorName)
	{
	}

	FName ChildActorName;
};

FName UChildActorComponent::GetComponentInstanceDataType() const
{
	static const FName ChildActorComponentInstanceDataName(TEXT("ChildActorInstanceData"));
	return ChildActorComponentInstanceDataName;
}

TSharedPtr<FComponentInstanceDataBase> UChildActorComponent::GetComponentInstanceData() const
{
	TSharedPtr<FChildActorComponentInstanceData> InstanceData;
	if (!ChildActorName.IsNone())
	{
		// Allocate new struct for holding light map data
		 InstanceData = MakeShareable(new FChildActorComponentInstanceData(this));
	}

	return InstanceData;
}

void UChildActorComponent::ApplyComponentInstanceData(TSharedPtr<FComponentInstanceDataBase> ComponentInstanceData)
{
	check(ComponentInstanceData.IsValid());
	ChildActorName = StaticCastSharedPtr<FChildActorComponentInstanceData>(ComponentInstanceData)->ChildActorName;
	if (ChildActor)
	{
		ChildActor->Rename(*ChildActorName.ToString(), NULL, REN_DoNotDirty);
	}
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
			// Before we spawn let's try and prevent cyclic disaster
			bool bSpawn = true;
			AActor* Actor = GetOwner();
			while (Actor && bSpawn)
			{
				if (Actor->GetClass() == ChildActorClass)
				{
					bSpawn = false;
					UE_LOG(LogChildActorComponent, Error, TEXT("Found cycle in child actor component '%s'.  Not spawning Actor of class '%s' to break."), *GetPathName(), *ChildActorClass->GetName());
				}
				Actor = Actor->ParentComponentActor.Get();
			}

			if (bSpawn)
			{
			FActorSpawnParameters Params;
			Params.bNoCollisionFail = true;
			Params.bDeferConstruction = true; // We defer construction so that we set ParentComponentActor prior to component registration so they appear selected
			Params.bAllowDuringConstructionScript = true;
			Params.OverrideLevel = GetOwner()->GetLevel();
			Params.Name = ChildActorName;
			Params.ObjectFlags &= ~RF_Transactional;

			// Spawn actor of desired class
			FVector Location = GetComponentLocation();
			FRotator Rotation = GetComponentRotation();
			ChildActor = World->SpawnActor(ChildActorClass, &Location, &Rotation, Params);

			// If spawn was successful, 
			if(ChildActor != NULL)
			{
				ChildActorName = ChildActor->GetFName();

				// Remember which actor spawned it (for selection in editor etc)
				ChildActor->ParentComponentActor = GetOwner();

				// Parts that we deferred from SpawnActor
				ChildActor->ExecuteConstruction(ComponentToWorld, NULL);
				ChildActor->PostActorConstruction();

				// attach new actor to this component
				ChildActor->AttachRootComponentTo(this, NAME_None, EAttachLocation::SnapToTarget);
			}
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
				ChildActor->Rename(*MakeUniqueObjectName(GetOuter(), ChildActor->GetClass(), TEXT("DESTROYED_CHILDACTOR")).ToString(), NULL, REN_DoNotDirty);
				World->DestroyActor(ChildActor);
			}
		}

		ChildActor = NULL;
	}
}
