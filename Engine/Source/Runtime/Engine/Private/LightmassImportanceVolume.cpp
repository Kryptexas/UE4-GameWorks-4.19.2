// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#include "../Classes/Lightmass/LightmassImportanceVolume.h"

ALightmassImportanceVolume::ALightmassImportanceVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetBrushComponent()->BodyInstance.bEnableCollision_DEPRECATED = false;

	GetBrushComponent()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	bColored = true;
	BrushColor.R = 255;
	BrushColor.G = 255;
	BrushColor.B = 25;
	BrushColor.A = 255;

}

