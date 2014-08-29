// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/NavigationModifier.h"
#include "AI/Navigation/NavModifierVolume.h"

//----------------------------------------------------------------------//
// ANavModifierVolume
//----------------------------------------------------------------------//
ANavModifierVolume::ANavModifierVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void ANavModifierVolume::GetNavigationData(struct FNavigationRelevantData& Data) const
{
	if (Brush && AreaClass && AreaClass != UNavigationSystem::GetDefaultWalkableArea())
	{
		FAreaNavModifier AreaMod(BrushComponent, AreaClass);
		Data.Modifiers.Add(AreaMod);
	}
}

FBox ANavModifierVolume::GetNavigationBounds() const
{
	return GetComponentsBoundingBox();
}

#if WITH_EDITOR

void ANavModifierVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ANavModifierVolume, AreaClass))
	{
		UNavigationSystem::UpdateNavOctree(this);
	}
}

#endif