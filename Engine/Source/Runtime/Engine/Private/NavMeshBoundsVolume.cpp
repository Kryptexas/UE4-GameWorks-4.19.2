// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

ANavMeshBoundsVolume::ANavMeshBoundsVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;

	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BrushComponent->Mobility = EComponentMobility::Static;

	BrushColor = FColor(200, 200, 200, 255);

	bColored = true;

	bWantsInitialize = true;
}

#if WITH_EDITOR

void ANavMeshBoundsVolume::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_NAVIGATION_GENERATOR
	if (PropertyChangedEvent.Property == NULL && GIsEditor == true && GetWorld() != NULL)
	{
		check(GetWorld()->GetNavigationSystem() != NULL);
		GetWorld()->GetNavigationSystem()->OnNavigationBoundsUpdated(this);
	}
#endif // WITH_NAVIGATION_GENERATOR
}

#endif // WITH_EDITOR

void ANavMeshBoundsVolume::PostInitializeComponents() 
{
	Super::PostInitializeComponents();

#if WITH_NAVIGATION_GENERATOR
	if (GetWorld()->GetNavigationSystem() != NULL && Role == ROLE_Authority)
	{
		GetWorld()->GetNavigationSystem()->OnNavigationBoundsUpdated(this);
	}
#endif
}
