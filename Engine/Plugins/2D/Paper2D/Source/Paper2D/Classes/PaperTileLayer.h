// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTileSet.h"

#include "PaperTileLayer.generated.h"

// Flags used in the packed tile index
enum class EPaperTileFlags : uint32
{
	FlipHorizontal = (1U << 31),
	FlipVertical = (1U << 30),
	FlipDiagonal = (1U << 29),

	TileIndexMask = ~(7U << 29),
};

// This is the contents of a tile map cell
USTRUCT(BlueprintType)
struct FPaperTileInfo
{
	GENERATED_USTRUCT_BODY()

	// The tile set that this tile comes from
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	UPaperTileSet* TileSet;

	// This is the index of the current tile within the tile set
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	int32 PackedTileIndex;

	FPaperTileInfo()
		: TileSet(nullptr)
		, PackedTileIndex(INDEX_NONE)
	{
	}

	inline bool operator==(const FPaperTileInfo& Other) const
	{
		return (Other.TileSet == TileSet) && (Other.PackedTileIndex == PackedTileIndex);
	}

	inline bool operator!=(const FPaperTileInfo& Other) const
	{
		return !(*this == Other);
	}

	bool IsValid() const
	{
		return (TileSet != nullptr) && (PackedTileIndex != INDEX_NONE);
	}

 	inline int32 GetFlagsAsIndex() const
 	{
		return (int32)(((uint32)PackedTileIndex) >> 29);
 	}

	inline void SetFlagsAsIndex(uint8 NewFlags)
	{
		const uint32 Base = PackedTileIndex & (int32)EPaperTileFlags::TileIndexMask;
		const uint32 WithNewFlags = Base | ((NewFlags & 0x7) << 29);
		PackedTileIndex = (int32)WithNewFlags;
	}

	inline int32 GetTileIndex() const
	{
		return PackedTileIndex & (int32)EPaperTileFlags::TileIndexMask;
	}

	inline bool HasFlag(EPaperTileFlags Flag) const
	{
		return (PackedTileIndex & (int32)Flag) != 0;
	}

	inline void ToggleFlag(EPaperTileFlags Flag)
	{
		if (IsValid())
		{
			PackedTileIndex ^= (int32)Flag;
		}
	}

	inline void SetFlagValue(EPaperTileFlags Flag, bool bValue)
	{
		if (IsValid())
		{
			if (bValue)
			{
				PackedTileIndex |= (int32)Flag;
			}
			else
			{
				PackedTileIndex &= ~(int32)Flag;
			}
		}
	}
};

// This class represents a single layer in a tile map.  All layers in the map must have the size dimensions.
UCLASS()
class PAPER2D_API UPaperTileLayer : public UObject
{
	GENERATED_UCLASS_BODY()

	// Name of the layer
	UPROPERTY()
	FText LayerName;

	// Width of the layer (in tiles)
	UPROPERTY(BlueprintReadOnly, Category=Sprite)
	int32 LayerWidth;

	// Height of the layer (in tiles)
	UPROPERTY(BlueprintReadOnly, Category=Sprite)
	int32 LayerHeight;

#if WITH_EDITORONLY_DATA
	// Opacity of the layer (editor only)
	UPROPERTY()
	float LayerOpacity;

	// Is this layer currently hidden in the editor?
	UPROPERTY()
	bool bHiddenInEditor;
#endif

	// Should this layer be hidden in the game?
	UPROPERTY(BlueprintReadOnly, Category=Sprite)
	bool bHiddenInGame;

protected:
	// The allocated width of the tile data (used to handle resizing without data loss)
	UPROPERTY()
	int32 AllocatedWidth;

	// The allocated height of the tile data (used to handle resizing without data loss)
	UPROPERTY()
	int32 AllocatedHeight;

	// The allocated tile data
	UPROPERTY()
	TArray<FPaperTileInfo> AllocatedCells;

private:
	UPROPERTY()
	UPaperTileSet* TileSet_DEPRECATED;

	UPROPERTY()
	TArray<int32> AllocatedGrid_DEPRECATED;

public:
	void ConvertToTileSetPerCell();

	// UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End of UObject interface

	class UPaperTileMap* GetTileMap() const;

	FPaperTileInfo GetCell(int32 X, int32 Y) const;
	void SetCell(int32 X, int32 Y, const FPaperTileInfo& NewValue);

	// This is a destructive operation!
	void DestructiveAllocateMap(int32 NewWidth, int32 NewHeight);
	
	// This tries to preserve contents
	void ResizeMap(int32 NewWidth, int32 NewHeight);

	// Adds collision to the specified body setup
	void AugmentBodySetup(UBodySetup* ShapeBodySetup);
protected:
	void ReallocateAndCopyMap();
};
