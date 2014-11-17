// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// APaperTileMapActor

APaperTileMapActor::APaperTileMapActor(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RenderComponent = PCIP.CreateDefaultSubobject<UPaperTileMapRenderComponent>(this, TEXT("RenderComponent"));

	RootComponent = RenderComponent;
}

#if WITH_EDITOR
bool APaperTileMapActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	if (RenderComponent->DefaultLayerTileSet != nullptr)
	{
		Objects.Add(RenderComponent->DefaultLayerTileSet);
	}
	return true;
}
#endif
