// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UDefaultSizedThumbnailRenderer::UDefaultSizedThumbnailRenderer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UDefaultSizedThumbnailRenderer::GetThumbnailSize(UObject*, float Zoom, uint32& OutWidth, uint32& OutHeight) const
{
	OutWidth = FMath::Trunc(DefaultSizeX * Zoom);
	OutHeight = FMath::Trunc(DefaultSizeY * Zoom);
}
