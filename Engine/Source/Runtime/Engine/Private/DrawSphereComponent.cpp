// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Components/DrawSphereComponent.h"

UDrawSphereComponent::UDrawSphereComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BodyInstance.bEnableCollision_DEPRECATED = false;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	bHiddenInGame = true;
	bUseEditorCompositing = true;
	bGenerateOverlapEvents = false;
}
