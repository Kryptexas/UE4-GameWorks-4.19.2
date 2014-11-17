// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "PaperSprite.h"
#include "PaperTileMap.generated.h"

UCLASS()
class PAPER2D_API UPaperTileMap : public UDataAsset //@TODO: Just to make it easy to spawn for now
{
	GENERATED_UCLASS_BODY()

	// Width of map (in tiles)
	UPROPERTY(Category=Setup, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 MapWidth;

	// Height of map (in tiles)
	UPROPERTY(Category=Setup, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 MapHeight;

	// Width of one tile (in pixels)
	UPROPERTY(Category=Setup, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 TileWidth;

	// Height of one tile (in pixels)
	UPROPERTY(Category=Setup, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 TileHeight;

	// Pixels per Unreal Unit (pixels per cm)
	UPROPERTY(Category = Setup, EditAnywhere)
	float PixelsPerUnit;

	// Default tile set to use for new layers
	UPROPERTY(Category=Setup, EditAnywhere)
	UPaperTileSet* DefaultLayerTileSet;

	// Test material
	UPROPERTY(Category=Setup, EditAnywhere)
	UMaterialInterface* Material;

	// The list of layers
	UPROPERTY()
	TArray<UPaperTileLayer*> TileLayers;

	// Collision domain (no collision, 2D, or 3D)
	UPROPERTY(Category = Collision, EditAnywhere)
	TEnumAsByte<ESpriteCollisionMode::Type> SpriteCollisionDomain;

	// Baked physics data.
	UPROPERTY()
	class UBodySetup* BodySetup;

public:
	// UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End of UObject interface


	FVector GetTilePositionInLocalSpace(int32 TileX, int32 TileY, int32 LayerIndex = 0) const;

	FBoxSphereBounds GetRenderBounds() const;
protected:
	virtual void UpdateBodySetup();
};
