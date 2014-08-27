// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ComponentInstanceDataCache.h"

DEFINE_LOG_CATEGORY_STATIC(LogChildActorComponent, Warning, All);

UChildActorComponent::UChildActorComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UChildActorComponent::OnRegister()
{
	Super::OnRegister();

	if (ChildActor)
	{
		ChildActorName = ChildActor->GetFName();
	}
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
		if (Component->ChildActor)
		{
			USceneComponent* ChildRootComponent = Component->ChildActor->GetRootComponent();
			if (ChildRootComponent)
			{
				for (USceneComponent* AttachedComponent : ChildRootComponent->AttachChildren)
				{
					if (AttachedComponent)
					{
						AActor* AttachedActor = AttachedComponent->GetOwner();
						if (AttachedActor != Component->ChildActor)
						{
							FAttachedActorInfo Info;
							Info.Actor = AttachedActor;
							Info.SocketName = AttachedComponent->AttachSocketName;
							Info.RelativeTransform = AttachedComponent->GetRelativeTransform();
							AttachedActors.Add(Info);
						}
					}
				}
			}
		}
	}

	// Overridden to also check the child actor name. Without the additional check, stale cached data may be applied to the child actor instance, which can lead to a crash during GC.
	virtual bool MatchesComponent(const UActorComponent* Component) const override
	{
		return (CastChecked<UChildActorComponent>(Component)->ChildActorName == ChildActorName && FComponentInstanceDataBase::MatchesComponent(Component));
	}

	FName ChildActorName;

	struct FAttachedActorInfo
	{
		TWeakObjectPtr<AActor> Actor;
		FName SocketName;
		FTransform RelativeTransform;
	};

	TArray<FAttachedActorInfo> AttachedActors;
};

FName UChildActorComponent::GetComponentInstanceDataType() const
{
	static const FName ChildActorComponentInstanceDataName(TEXT("ChildActorInstanceData"));
	return ChildActorComponentInstanceDataName;
}

TSharedPtr<FComponentInstanceDataBase> UChildActorComponent::GetComponentInstanceData() const
{
	return (CachedInstanceData.IsValid() ? CachedInstanceData : MakeShareable(new FChildActorComponentInstanceData(this)));
}

void UChildActorComponent::ApplyComponentInstanceData(TSharedPtr<FComponentInstanceDataBase> ComponentInstanceData)
{
	check(ComponentInstanceData.IsValid());
	FChildActorComponentInstanceData* ChildActorInstanceData = StaticCastSharedPtr<FChildActorComponentInstanceData>(ComponentInstanceData).Get();
	ChildActorName = ChildActorInstanceData->ChildActorName;
	if (ChildActor)
	{
		// Only rename if it is safe to
		const FString ChildActorNameString = ChildActorName.ToString();
		if (ChildActor->Rename(*ChildActorName.ToString(), NULL, REN_Test))
		{
			ChildActor->Rename(*ChildActorName.ToString(), NULL, REN_DoNotDirty);
		}

		USceneComponent* ChildActorRoot = ChildActor->GetRootComponent();
		if (ChildActorRoot)
		{
			for (const auto& AttachInfo : ChildActorInstanceData->AttachedActors)
			{
				AActor* AttachedActor = AttachInfo.Actor.Get();
				if (AttachedActor)
				{
					USceneComponent* AttachedRootComponent = AttachedActor->GetRootComponent();
					if (AttachedRootComponent)
					{
						AttachedActor->DetachRootComponentFromParent();
						AttachedRootComponent->AttachTo(ChildActorRoot, AttachInfo.SocketName, EAttachLocation::KeepWorldPosition);
						AttachedRootComponent->SetRelativeTransform(AttachInfo.RelativeTransform);
						AttachedRootComponent->UpdateComponentToWorld();
					}
				}
			}
		}
	}
}

void UChildActorComponent::CreateChildActor()
{
	// Kill spawned actor if we have one
	DestroyChildActor();

	// This is no longer needed
	CachedInstanceData = NULL;

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
					ChildActor->FinishSpawning(ComponentToWorld);

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
			CachedInstanceData = MakeShareable(new FChildActorComponentInstanceData(this));

			UWorld* World = ChildActor->GetWorld();
			// World may be NULL during shutdown
			if(World != NULL)
			{
				UClass* ChildClass = ChildActor->GetClass();

				// We would like to make certain that our name is not going to accidentally get taken from us while we're destroyed
				// so we increment ClassUnique beyond our index to be certain of it.  This is ... a bit hacky.
				ChildClass->ClassUnique = FMath::Max(ChildClass->ClassUnique, ChildActor->GetFName().GetNumber());

				const FString ObjectBaseName = FString::Printf(TEXT("DESTROYED_%s_CHILDACTOR"), *ChildClass->GetName());
				ChildActor->Rename(*MakeUniqueObjectName(GetOuter(), ChildClass, *ObjectBaseName).ToString(), NULL, REN_DoNotDirty);
				World->DestroyActor(ChildActor);
			}
		}

		ChildActor = NULL;
	}
}
