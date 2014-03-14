// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

static FName InvisibleWall_NAME(TEXT("InvisibleWall"));

ABlockingVolume::ABlockingVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = true;
	BrushComponent->SetCollisionProfileName(InvisibleWall_NAME);

	bWantsInitialize = false;
}

bool ABlockingVolume::UpdateNavigationRelevancy() 
{ 
	SetNavigationRelevancy(GetActorEnableCollision() && RootComponent->IsRegistered()); 
	return IsNavigationRelevant(); 
}

#if WITH_EDITOR

static FName InvisibleWallDynamic_NAME(TEXT("InvisibleWallDynamic"));

void ABlockingVolume::LoadedFromAnotherClass(const FName& OldClassName)
{
	Super::LoadedFromAnotherClass(OldClassName);

	if(GetLinkerUE4Version() < VER_UE4_REMOVE_DYNAMIC_VOLUME_CLASSES)
	{
		static FName DynamicBlockingVolume_NAME(TEXT("DynamicBlockingVolume"));

		if(OldClassName == DynamicBlockingVolume_NAME)
		{
			BrushComponent->Mobility = EComponentMobility::Movable;

			if(BrushComponent->GetCollisionProfileName() == InvisibleWall_NAME)
			{
				BrushComponent->SetCollisionProfileName(InvisibleWallDynamic_NAME);
			}
		}
	}
}

void ABlockingVolume::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Get 'deepest' property name we changed.
	const FName TailPropName = PropertyChangedEvent.PropertyChain.GetTail()->GetValue()->GetFName();
	static FName Mobility_NAME(TEXT("Mobility"));
	if( TailPropName == Mobility_NAME )
	{
		// If the collision profile is one of the 'default' ones for a BlockingVolume, make sure it is the correct one
		// If user has changed it to something else, don't touch it
		FName CurrentProfileName = BrushComponent->GetCollisionProfileName();
		if(	CurrentProfileName == InvisibleWall_NAME ||
			CurrentProfileName == InvisibleWallDynamic_NAME )
		{
			if(BrushComponent->Mobility == EComponentMobility::Movable)
			{
				BrushComponent->SetCollisionProfileName(InvisibleWallDynamic_NAME);
			}
			else
			{
				BrushComponent->SetCollisionProfileName(InvisibleWall_NAME);
			}
		}
	}
}

#endif