// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Lightmass/VolumetricLightmapDensityVolume.h"
#include "Engine/CollisionProfile.h"
#include "Components/BrushComponent.h"


AVolumetricLightmapDensityVolume::AVolumetricLightmapDensityVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetBrushComponent()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	bColored = true;
	BrushColor.R = 155;
	BrushColor.G = 185;
	BrushColor.B = 25;
	BrushColor.A = 255;

	AllowedMipLevelRange = FInt32Interval(1, 3);
}

