// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/NavigationModifier.h"

//----------------------------------------------------------------------//
// ANavModifierVolume
//----------------------------------------------------------------------//
ANavModifierVolume::ANavModifierVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool ANavModifierVolume::GetNavigationRelevantData(struct FNavigationRelevantData& Data) const
{
	if (Brush && AreaClass && AreaClass != UNavigationSystem::GetDefaultWalkableArea())
	{
		FAreaNavModifier AreaMod(BrushComponent, AreaClass);
		Data.Modifiers.Add(AreaMod);
	}
	
	return false;
}
