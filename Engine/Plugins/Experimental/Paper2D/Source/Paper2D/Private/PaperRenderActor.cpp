// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperRenderActor.h"

//////////////////////////////////////////////////////////////////////////
// APaperRenderActor

APaperRenderActor::APaperRenderActor(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RenderComponent = PCIP.CreateDefaultSubobject<UPaperRenderComponent>(this, TEXT("RenderComponent"));

	RootComponent = RenderComponent;
}
