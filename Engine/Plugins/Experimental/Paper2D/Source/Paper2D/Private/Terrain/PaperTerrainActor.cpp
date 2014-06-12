// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// APaperTerrainActor

APaperTerrainActor::APaperTerrainActor(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DummyRoot = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent"));
	SplineComponent = PCIP.CreateDefaultSubobject<UPaperTerrainSplineComponent>(this, TEXT("SplineComponent"));
 	RenderComponent = PCIP.CreateDefaultSubobject<UPaperTerrainComponent>(this, TEXT("RenderComponent"));
 
	SplineComponent->AttachParent = DummyRoot;
	RenderComponent->AttachParent = DummyRoot;
	RenderComponent->AssociatedSpline = SplineComponent;
	RootComponent = DummyRoot;
}
