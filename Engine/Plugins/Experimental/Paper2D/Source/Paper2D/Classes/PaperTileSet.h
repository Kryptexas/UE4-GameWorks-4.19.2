// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTileSet.generated.h"

UCLASS(DependsOn=UEngineTypes)
class PAPER2D_API UPaperTileSet : public UDataAsset //@TODO: Just to make it easy to spawn for now
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=TileSet, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 TileWidth;

	UPROPERTY(Category=TileSet, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 TileHeight;

	UPROPERTY(Category=TileSet, EditAnywhere)
	UTexture2D* TileSheet;

public:
	int32 GetTileCount() const;
	int32 GetTileCountX() const;
	int32 GetTileCountY() const;

	bool GetTileUV(int32 TileIndex, /*out*/ FVector2D& Out_TileUV) const;
};
