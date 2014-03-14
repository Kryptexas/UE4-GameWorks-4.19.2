// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

ALevelStreamingVolume::ALevelStreamingVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;

	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BrushComponent->bAlwaysCreatePhysicsState = true;

	bColored = true;
	BrushColor.R = 255;
	BrushColor.G = 165;
	BrushColor.B = 0;
	BrushColor.A = 255;

	StreamingUsage = SVB_LoadingAndVisibility;
}

