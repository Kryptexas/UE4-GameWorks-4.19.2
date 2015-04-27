// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpriteEditorOnlyTypes.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"

#include "PaperTileSet.generated.h"

struct FPaperTileInfo;

// Information about a single tile in a tile set
USTRUCT()
struct PAPER2D_API FPaperTileMetadata
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Sprite)
	FSpriteGeometryCollection CollisionData;

	// Indexes into the Terrains array of the owning tile set, in counterclockwise order starting from top-left
	// 0xFF indicates no membership.
	UPROPERTY()
	uint8 TerrainMembership[4];

public:
	FPaperTileMetadata()
	{
		CollisionData.GeometryType = ESpritePolygonMode::FullyCustom;
		TerrainMembership[0] = 0xFF;
		TerrainMembership[1] = 0xFF;
		TerrainMembership[2] = 0xFF;
		TerrainMembership[3] = 0xFF;
	}

	// Does this tile have collision information?
	bool HasCollision() const
	{
		return CollisionData.Shapes.Num() > 0;
	}

	// Does this tile have user-specified metadata?
	bool HasMetaData() const
	{
		return false;
	}
};

// Information about a terrain type
USTRUCT()
struct PAPER2D_API FPaperTileSetTerrain
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Sprite)
	FString TerrainName;

	UPROPERTY()
	int32 CenterTileIndex;
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
	UPROPERTY(Category=TileSet, EditAnywhere)
	FIntPoint DrawingOffset;

#if WITH_EDITORONLY_DATA
	/** The background color displayed in the tile set viewer */
	UPROPERTY(Category=TileSet, EditAnywhere, meta=(HideAlphaChannel))
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

	// Terrain information
	UPROPERTY()//@TODO: TileMapTerrains: (EditAnywhere, Category=Sprite)
	TArray<FPaperTileSetTerrain> Terrains;

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

	// Adds a new terrain to this tile set (returns false if the maximum number of terrains has already been reached)
	bool AddTerrainDescription(FPaperTileSetTerrain NewTerrain);

	// Returns the number of terrains this tile set has
	int32 GetNumTerrains() const
	{
		return Terrains.Num();
	}

	FPaperTileSetTerrain GetTerrain(int32 Index) const { return Terrains.IsValidIndex(Index) ? Terrains[Index] : FPaperTileSetTerrain(); }

	// Returns the terrain type this tile is a member of, or INDEX_NONE if it is not part of a terrain
	int32 GetTerrainMembership(const FPaperTileInfo& TileInfo) const;
};
