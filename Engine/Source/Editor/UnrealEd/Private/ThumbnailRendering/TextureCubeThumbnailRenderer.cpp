// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UTextureCubeThumbnailRenderer::UTextureCubeThumbnailRenderer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UTextureCubeThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const
{
	OutWidth = 0;
	OutHeight = 0;

	UTextureCube* CubeMap = Cast<UTextureCube>(Object);
	if (CubeMap != nullptr)
	{
		// Let the base class get the size of a face
		Super::GetThumbnailSize(CubeMap, Zoom, OutWidth, OutHeight);
	}
}

void UTextureCubeThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas)
{
	UTextureCube* CubeMap = Cast<UTextureCube>(Object);
	if (CubeMap != nullptr)
	{
		Super::Draw(CubeMap, X, Y, Width, Height, nullptr,	Canvas);
	}
}
