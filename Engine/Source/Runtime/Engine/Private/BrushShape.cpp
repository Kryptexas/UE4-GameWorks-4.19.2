// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/BrushShape.h"

ABrushShape::ABrushShape(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	GetBrushComponent()->BodyInstance.bEnableCollision_DEPRECATED = false;
	GetBrushComponent()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	GetBrushComponent()->AlwaysLoadOnClient = true;
	GetBrushComponent()->AlwaysLoadOnServer = false;

}

