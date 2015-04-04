// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpriteEditorOnlyTypes.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"

#include "PaperTileSet.generated.h"

// Information about a single tile in a tile set
USTRUCT()
struct PAPER2D_API FPaperTileMetadata
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Sprite)
	FSpriteGeometryCollection CollisionData;

public:
	FPaperTileMetadata()
	{
		CollisionData.GeometryType = ESpritePolygonMode::FullyCustom;
	}

	// Does this tile have collision information?
	bool HasCollision() const
	{
		return CollisionData.Shapes.Num() > 0;
	}
};


/**
 * A tile set is a collection of tiles pulled from a texture that can be used to fill out a tile map.
 *
 * @see UPaperTileMap, UPaperTileMapComponent
 */
UCLASS()
class PAPER2D_API UPaperTileSet : public UObject
{
	GENERATED_UCLASS_BODY()

	// The width of a single tile (in pixels)
	UPROPERTY(Category=TileSet, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 TileWidth;

	// The height of a single tile (in pixels)
	UPROPERTY(Category=TileSet, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 TileHeight;

	// The tile sheet texture associated with this tile set
	UPROPERTY(Category=TileSet, EditAnywhere)
	UTexture2D* TileSheet;

	// The amount of padding around the perimeter of the tile sheet (in pixels)
	UPROPERTY(Category=TileSet, EditAnywhere, meta=(UIMin=0, ClampMin=0))
	int32 Margin;

	// The amount of padding between tiles in the tile sheet (in pixels)
	UPROPERTY(Category=TileSet, EditAnywhere, meta=(UIMin=0, ClampMin=0))
	int32 Spacing;

	// The drawing offset for tiles from this set (in pixels)
	UPROPERTY(Category=TileSet, EditAnywhere, meta=(UIMin=0, ClampMin=0))
	FIntPoint DrawingOffset;

#if WITH_EDITORONLY_DATA
	/** The background color displayed in the tile set viewer */
	UPROPERTY(Category=TileSet, EditAnywhere)
	FLinearColor BackgroundColor;
#endif


protected:
	// Cached width of this tile set (in tiles)
	UPROPERTY()
	int32 WidthInTiles;

	// Cached height of this tile set (in tiles)
	UPROPERTY()
	int32 HeightInTiles;

	// Allocated width of the per-tile data
	UPROPERTY()
	int32 AllocatedWidth;

	// Allocated height of the per-tile data
	UPROPERTY()
	int32 AllocatedHeight;

	// Per-tile information (Experimental)
	UPROPERTY(EditAnywhere, EditFixedSize, Category=Sprite)
	TArray<FPaperTileMetadata> ExperimentalPerTileData;

protected:

	// Reallocates the ExperimentalPerTileData array to the specified size.
	// Note: This is a destructive operation!
	void DestructiveAllocateTileData(int32 NewWidth, int32 NewHeight);

	// Reallocates the per-tile data to match the current (WidthInTiles, HeightInTiles) size, copying over what it can
	void ReallocateAndCopyTileData();

public:	

	// UObject interface
#if WITH_EDITOR
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End of UObject interface

public:
	// Returns the number of tiles in this tile set
	int32 GetTileCount() const;
	
	// Returns the number of tiles per row in this tile set
	int32 GetTileCountX() const;
	
	// Returns the number of tiles per column in this tile set
	int32 GetTileCountY() const;

	// Returns editable tile metadata for the specified tile index
	FPaperTileMetadata* GetMutableTileMetadata(int32 TileIndex);

	// Returns the tile metadata for the specified tile index
	const FPaperTileMetadata* GetTileMetadata(int32 TileIndex) const;

	// Returns the texture-space coordinates of the top left corner of the specified tile index
	bool GetTileUV(int32 TileIndex, /*out*/ FVector2D& Out_TileUV) const;

	// Returns the texture-space coordinates of the top left corner of the tile at (TileXY.X, TileXY.Y)
	FIntPoint GetTileUVFromTileXY(const FIntPoint& TileXY) const;

	// Converts the texture-space coordinates into tile coordinates
	FIntPoint GetTileXYFromTextureUV(const FVector2D& TextureUV, bool bRoundUp) const;
};
