// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTileSet.h"

#include "PaperTileLayer.generated.h"

USTRUCT(BlueprintType)
struct FPaperTileInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UPaperTileSet* TileSet;

	UPROPERTY()
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
};

UCLASS()
class PAPER2D_API UPaperTileLayer : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FText LayerName;

	UPROPERTY()
	int32 LayerWidth;

	UPROPERTY()
	int32 LayerHeight;

	UPROPERTY()
	UPaperTileSet* TileSet;

	UPROPERTY()
	bool bHiddenInEditor;

	UPROPERTY()
	bool bHiddenInGame;

	UPROPERTY()
	bool bCollisionLayer;

protected:
	UPROPERTY()
	int32 AllocatedWidth;

	UPROPERTY()
	int32 AllocatedHeight;

	UPROPERTY()
	TArray<int32> AllocatedGrid_DEPRECATED;

	UPROPERTY()
	TArray<FPaperTileInfo> AllocatedCells;

public:
	void ConvertToTileSetPerCell();

	// UObject interface
#if WITH_EDITORONLY_DATA
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
