// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "HierarchicalLODVolume.h"
#include "Engine/CollisionProfile.h"
#include "Components/BrushComponent.h"

AHierarchicalLODVolume::AHierarchicalLODVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIncludeOverlappingActors(false)
{
	GetBrushComponent()->bGenerateOverlapEvents = false;
	bNotForClientOrServer = true;

	bColored = true;
	BrushColor.R = 255;
	BrushColor.G = 100;
	BrushColor.B = 255;
	BrushColor.A = 255;
}
