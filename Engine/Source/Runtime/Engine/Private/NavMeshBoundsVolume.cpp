// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/Navigation/NavMeshBoundsVolume.h"

ANavMeshBoundsVolume::ANavMeshBoundsVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;

	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BrushComponent->Mobility = EComponentMobility::Static;

	BrushColor = FColor(200, 200, 200, 255);

	bColored = true;
}

#if WITH_EDITOR

void ANavMeshBoundsVolume::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_NAVIGATION_GENERATOR
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	if (GIsEditor && NavSys)
	{
		if (PropertyChangedEvent.Property == NULL ||
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ABrush, BrushBuilder))
		{
			NavSys->OnNavigationBoundsUpdated(this);
		}
	}
#endif // WITH_NAVIGATION_GENERATOR
}

#endif // WITH_EDITOR

void ANavMeshBoundsVolume::PostInitializeComponents() 
{
	Super::PostInitializeComponents();

#if WITH_NAVIGATION_GENERATOR
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	if (NavSys && Role == ROLE_Authority)
	{
		NavSys->OnNavigationBoundsUpdated(this);
	}
#endif
}
