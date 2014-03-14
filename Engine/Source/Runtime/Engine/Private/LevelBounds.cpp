// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

ALevelBounds::ALevelBounds(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	TSubobjectPtr<UBoxComponent> BoxComponent = PCIP.CreateDefaultSubobject<UBoxComponent>(this, TEXT("BoxComponent0"));
	RootComponent = BoxComponent;
	RootComponent->Mobility = EComponentMobility::Static;
	RootComponent->RelativeScale3D = FVector(2048.0f);

	bAutoUpdateBounds = true;

	BoxComponent->bDrawOnlyIfSelected = true;
	BoxComponent->bUseAttachParentBound = false;
	BoxComponent->bUseEditorCompositing = true;
	BoxComponent->BodyInstance.bEnableCollision_DEPRECATED = false;
	BoxComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BoxComponent->InitBoxExtent(FVector(0.5f, 0.5f, 0.5f));
	
	bWantsInitialize = false;
	bCanBeDamaged = false;
	
#if WITH_EDITOR
	bLevelBoundsDirty = true;
	bSubscribedToEvents = false;
#endif
}

void ALevelBounds::PostLoad()
{
	Super::PostLoad();
	GetLevel()->LevelBoundsActor = this;
}

FBox ALevelBounds::GetComponentsBoundingBox(bool bNonColliding) const
{
	FVector BoundsCenter = RootComponent->GetComponentLocation();
	FVector BoundsExtent = RootComponent->ComponentToWorld.GetScale3D() * 0.5f;
	return FBox(BoundsCenter - BoundsExtent, 
				BoundsCenter + BoundsExtent);
}

FBox ALevelBounds::CalculateLevelBounds(ULevel* InLevel)
{
	FBox LevelBounds = FBox(0);
	
	if (InLevel)
	{
		// Iterate over all level actors
		for (int32 ActorIndex = 0; ActorIndex < InLevel->Actors.Num() ; ++ActorIndex)
		{
			AActor* Actor = InLevel->Actors[ActorIndex];
			if (Actor && Actor->IsLevelBoundsRelevant())
			{
				// Sum up components bounding boxes
				FBox ActorBox = Actor->GetComponentsBoundingBox(true);
				if (ActorBox.IsValid)
				{
					LevelBounds+= ActorBox;
				}
			}
		}
	}

	return LevelBounds;
}

#if WITH_EDITOR
void ALevelBounds::PostEditUndo()
{
	Super::PostEditUndo();
	bLevelBoundsDirty = true;
}

void ALevelBounds::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	bLevelBoundsDirty = true;
}

void ALevelBounds::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty( PropertyChangedEvent );

	if (GIsEditor && !GetWorld()->IsPlayInEditor())
	{
		if (bAutoUpdateBounds)
		{
			UpdateLevelBounds();
		}
	}
}

void ALevelBounds::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	
	GetLevel()->LevelBoundsActor = this;
	SubscribeToUpdateEvents();
}

void ALevelBounds::PostUnregisterAllComponents()
{
	UnsubscribeFromUpdateEvents();
	Super::PostUnregisterAllComponents();
}

void ALevelBounds::UpdateLevelBounds()
{
	FBox LevelBounds = CalculateLevelBounds(GetLevel());
	if (LevelBounds.IsValid)
	{
		FVector LevelCenter = LevelBounds.GetCenter();
		FVector LevelSize = LevelBounds.GetSize();
		
		SetActorTransform(FTransform(FQuat::Identity, LevelCenter, LevelSize));
	}
	
	BroadcastLevelBoundsUpdated();
}

void ALevelBounds::OnLevelBoundsDirtied()
{
	bLevelBoundsDirty = true;
}

void ALevelBounds::OnLevelActorMoved(AActor* InActor)
{
	if (InActor->GetOuter() == GetOuter())
	{
		if (InActor == this)
		{
			BroadcastLevelBoundsUpdated();
		}
		else
		{
			OnLevelBoundsDirtied();
		}
	}
}
	
void ALevelBounds::OnLevelActorAddedRemoved(AActor* InActor)
{
	if (InActor->GetOuter() == GetOuter())
	{
		OnLevelBoundsDirtied();
	}
}

void ALevelBounds::OnTimerTick()
{
	if (bLevelBoundsDirty && bAutoUpdateBounds)
	{
		UpdateLevelBounds();
		bLevelBoundsDirty = false;
	}
}

void ALevelBounds::BroadcastLevelBoundsUpdated()
{
	ULevel* Level = GetLevel();
	if (Level && 
		Level->LevelBoundsActor.Get() == this)
	{
		Level->BroadcastLevelBoundsActorUpdated();
	}
}

void ALevelBounds::SubscribeToUpdateEvents()
{
	if (bSubscribedToEvents == false)
	{
		GetWorldTimerManager().SetTimer(this, &ALevelBounds::OnTimerTick, 1, true);
		GEngine->OnActorMoved().AddUObject(this, &ALevelBounds::OnLevelActorMoved);
		GEngine->OnLevelActorDeleted().AddUObject(this, &ALevelBounds::OnLevelActorAddedRemoved);
		GEngine->OnLevelActorAdded().AddUObject(this, &ALevelBounds::OnLevelActorAddedRemoved);
		
		bSubscribedToEvents = true;
	}
}

void ALevelBounds::UnsubscribeFromUpdateEvents()
{
	if (bSubscribedToEvents == true)
	{
		if (GetWorld())
		{
			GetWorldTimerManager().ClearTimer(this, &ALevelBounds::OnTimerTick);
		}
		GEngine->OnActorMoved().RemoveUObject(this, &ALevelBounds::OnLevelActorMoved);
		GEngine->OnLevelActorDeleted().RemoveUObject(this, &ALevelBounds::OnLevelActorAddedRemoved);
		GEngine->OnLevelActorAdded().RemoveUObject(this, &ALevelBounds::OnLevelActorAddedRemoved);

		bSubscribedToEvents = false;
	}
}


#endif // WITH_EDITOR


