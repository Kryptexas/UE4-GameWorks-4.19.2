// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interface.h"
#include "SlateTextureAtlasInterface.generated.h"

/**  */
struct FSlateAtlasData
{
public:
	FSlateAtlasData(UTexture2D* InAtlasTexture, FVector2D InRegionPosition, FVector2D InRegionSize)
		: AtlasTexture(InAtlasTexture)
		, RegionPosition(InRegionPosition)
		, RegionSize(InRegionSize)
	{
	}

public:
	/** The texture pointer for the Atlas */
	UTexture2D* AtlasTexture;

	/** The region start position in pixels. */
	FVector2D RegionPosition;

	/** The region size in pixels. */
	FVector2D RegionSize;
};

/**  */
UINTERFACE(meta=( CannotImplementInterfaceInBlueprint ))
class SLATERHIRENDERER_API USlateTextureAtlasInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**  */
class SLATERHIRENDERER_API ISlateTextureAtlasInterface
{
	GENERATED_IINTERFACE_BODY()

	/** Gets the atlas data to use when rendering with slate. */
	virtual FSlateAtlasData GetSlateAtlasData() const = 0;
};
