// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#include "../Classes/Lightmass/LightmassCharacterIndirectDetailVolume.h"

ALightmassCharacterIndirectDetailVolume::ALightmassCharacterIndirectDetailVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;

	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	bColored = true;
	BrushColor.R = 155;
	BrushColor.G = 185;
	BrushColor.B = 25;
	BrushColor.A = 255;

}
