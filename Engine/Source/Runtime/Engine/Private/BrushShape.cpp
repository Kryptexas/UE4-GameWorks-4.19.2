// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

ABrushShape::ABrushShape(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;
	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BrushComponent->AlwaysLoadOnClient = true;
	BrushComponent->AlwaysLoadOnServer = false;

}

