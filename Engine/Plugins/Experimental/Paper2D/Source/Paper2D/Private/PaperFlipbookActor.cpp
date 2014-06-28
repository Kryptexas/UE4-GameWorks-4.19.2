// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperRenderActor.h"

//////////////////////////////////////////////////////////////////////////
// APaperFlipbookActor

APaperFlipbookActor::APaperFlipbookActor(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RenderComponent = PCIP.CreateDefaultSubobject<UPaperFlipbookComponent>(this, TEXT("RenderComponent"));

	RootComponent = RenderComponent;
}

#if WITH_EDITOR
bool APaperFlipbookActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	if (UPaperFlipbook* FlipbookAsset = RenderComponent->GetFlipbook())
	{
		Objects.Add(FlipbookAsset);
	}
	return true;
}
#endif
