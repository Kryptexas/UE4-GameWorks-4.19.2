// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTerrainMaterial.generated.h"

/**
 * Paper Terrain Material
 *
 * 'Material' setup for a 2D terrain spline (stores references to sprites that will be instanced along the spline path, not actually related to UMaterialInterface).
 */

UCLASS(BlueprintType)
class PAPER2D_API UPaperTerrainMaterial : public UDataAsset
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Sprite, EditAnywhere, meta=(DisplayThumbnail = "true"))
	UPaperSprite* LeftCap;

	UPROPERTY(Category=Sprite, EditAnywhere, meta=(DisplayThumbnail = "true"))
	TArray<UPaperSprite*> BodySegments;

	UPROPERTY(Category=Sprite, EditAnywhere, meta=(DisplayThumbnail = "true"))
	UPaperSprite* RightCap;
};
