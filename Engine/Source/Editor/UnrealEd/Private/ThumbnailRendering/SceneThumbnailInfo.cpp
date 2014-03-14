// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

USceneThumbnailInfo::USceneThumbnailInfo(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	OrbitPitch = -11.25;
	OrbitYaw = -157.5;
	OrbitZoom = 0;
}
