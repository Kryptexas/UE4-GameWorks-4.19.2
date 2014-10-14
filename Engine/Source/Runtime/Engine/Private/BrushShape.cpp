// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/BrushShape.h"

ABrushShape::ABrushShape(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;
	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BrushComponent->AlwaysLoadOnClient = true;
	BrushComponent->AlwaysLoadOnServer = false;

}

