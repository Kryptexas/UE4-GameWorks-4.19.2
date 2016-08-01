// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interface.h"
#include "SlateTextureAtlasInterface.generated.h"

/**  */
struct FSlateAtlasData
{
public:
	FSlateAtlasData(UTexture* InAtlasTexture, FVector2D InStartUV, FVector2D InSizeUV)
		: AtlasTexture(InAtlasTexture)
		, StartUV(InStartUV)
		, SizeUV(InSizeUV)
	{
	}

public:
	/** The texture pointer for the Atlas */
	UTexture* AtlasTexture;

	/** The region start position in UVs */
	FVector2D StartUV;

	/** The region size in UVs. */
	FVector2D SizeUV;
};

/**  */
UINTERFACE(meta=( CannotImplementInterfaceInBlueprint ))
class ENGINE_API USlateTextureAtlasInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**  */
class ENGINE_API ISlateTextureAtlasInterface
{
	GENERATED_IINTERFACE_BODY()

	/** Gets the atlas data to use when rendering with slate. */
	virtual FSlateAtlasData GetSlateAtlasData() const = 0;
};
