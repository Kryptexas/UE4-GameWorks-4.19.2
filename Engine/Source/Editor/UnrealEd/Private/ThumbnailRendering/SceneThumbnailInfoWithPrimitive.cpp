// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

USceneThumbnailInfoWithPrimitive::USceneThumbnailInfoWithPrimitive(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimitiveType = TPT_Sphere;
	OrbitPitch = -35;
	OrbitYaw = -180.f;
	OrbitZoom = -400.f;
}

