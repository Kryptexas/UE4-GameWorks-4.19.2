// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperRenderActor.h"

//////////////////////////////////////////////////////////////////////////
// APaperFlipbookActor

APaperFlipbookActor::APaperFlipbookActor(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RenderComponent = PCIP.CreateDefaultSubobject<UPaperAnimatedRenderComponent>(this, TEXT("RenderComponent"));

	RootComponent = RenderComponent;
}
