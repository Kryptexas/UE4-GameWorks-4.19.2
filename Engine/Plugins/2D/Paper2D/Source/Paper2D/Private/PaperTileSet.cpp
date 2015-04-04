// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UPaperTileSet

UPaperTileSet::UPaperTileSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TileWidth = 32;
	TileHeight = 32;

#if WITH_EDITORONLY_DATA
	BackgroundColor = FColor(0, 0, 127);
#endif
}

int32 UPaperTileSet::GetTileCount() const
{
	if (TileSheet != nullptr)
	{
		checkSlow((TileWidth > 0) && (TileHeight > 0));
		const int32 TextureWidth = TileSheet->GetSizeX();
		const int32 TextureHeight = TileSheet->GetSizeY();

		const int32 CellsX = (TextureWidth - (Margin * 2) + Spacing) / (TileWidth + Spacing);
		const int32 CellsY = (TextureHeight - (Margin * 2) + Spacing) / (TileHeight + Spacing);

		return CellsX * CellsY;
	}
	else
	{
		return 0;
	}
}

int32 UPaperTileSet::GetTileCountX() const
{
	if (TileSheet != nullptr)
	{
		checkSlow(TileWidth > 0);
		const int32 TextureWidth = TileSheet->GetSizeX();
		const int32 CellsX = (TextureWidth - (Margin * 2) + Spacing) / (TileWidth + Spacing);
		return CellsX;
	}
	else
	{
		return 0;
	}
}

int32 UPaperTileSet::GetTileCountY() const
{
	if (TileSheet != nullptr)
	{
		checkSlow(TileHeight > 0);
		const int32 TextureHeight = TileSheet->GetSizeY();
		const int32 CellsY = (TextureHeight - (Margin * 2) + Spacing) / (TileHeight + Spacing);
		return CellsY;
	}
	else
	{
		return 0;
	}
}

FPaperTileMetadata* UPaperTileSet::GetMutableTileMetadata(int32 TileIndex)
{
	if (ExperimentalPerTileData.IsValidIndex(TileIndex))
	{
		return &(ExperimentalPerTileData[TileIndex]);
	}
	else
	{
		return nullptr;
	}
}

const FPaperTileMetadata* UPaperTileSet::GetTileMetadata(int32 TileIndex) const
{
	if (ExperimentalPerTileData.IsValidIndex(TileIndex))
	{
		return &(ExperimentalPerTileData[TileIndex]);
	}
	else
	{
		return nullptr;
	}
}

bool UPaperTileSet::GetTileUV(int32 TileIndex, /*out*/ FVector2D& Out_TileUV) const
{
	const int32 NumCells = GetTileCount();

	if ((TileIndex < 0) || (TileIndex >= NumCells))
	{
		return false;
	}
	else
	{
		const int32 CellsX = GetTileCountX();

		const FIntPoint XY(TileIndex % CellsX, TileIndex / CellsX);

		Out_TileUV = GetTileUVFromTileXY(XY);
		return true;
	}
}

FIntPoint UPaperTileSet::GetTileUVFromTileXY(const FIntPoint& TileXY) const
{
	return FIntPoint(TileXY.X * (TileWidth + Spacing) + Margin, TileXY.Y * (TileHeight + Spacing) + Margin);
}

FIntPoint UPaperTileSet::GetTileXYFromTextureUV(const FVector2D& TextureUV, bool bRoundUp) const
{
	const float DividendX = TextureUV.X - Margin;
	const float DividendY = TextureUV.Y - Margin;
	const float DivisorX = TileWidth + Spacing;
	const float DivisorY = TileHeight + Spacing;
	const int32 X = bRoundUp ? FMath::DivideAndRoundUp<int32>(DividendX, DivisorX) : FMath::DivideAndRoundDown<int32>(DividendX, DivisorX);
	const int32 Y = bRoundUp ? FMath::DivideAndRoundUp<int32>(DividendY, DivisorY) : FMath::DivideAndRoundDown<int32>(DividendY, DivisorY);
	return FIntPoint(X, Y);
}

#if WITH_EDITOR

#include "PaperTileMapComponent.h"
#include "ComponentReregisterContext.h"

void UPaperTileSet::PostLoad()
{
	if (TileSheet != nullptr)
	{
		TileSheet->ConditionalPostLoad();

		WidthInTiles = GetTileCountX();
		HeightInTiles = GetTileCountY();
		ReallocateAndCopyTileData();
	}

	Super::PostLoad();
}

void UPaperTileSet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	Margin = FMath::Max<int32>(Margin, 0);
	Spacing = FMath::Max<int32>(Spacing, 0);

	TileWidth = FMath::Max<int32>(TileWidth, 1);
	TileHeight = FMath::Max<int32>(TileHeight, 1);

	WidthInTiles = GetTileCountX();
	HeightInTiles = GetTileCountY();
	ReallocateAndCopyTileData();

	// Rebuild any tile map components that may have been relying on us
	if ((PropertyChangedEvent.ChangeType & EPropertyChangeType::Interactive) == 0)
	{
		//@TODO: Currently tile maps have no fast list of referenced tile sets, so we just rebuild all of them
		TComponentReregisterContext<UPaperTileMapComponent> ReregisterAllTileMaps;
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UPaperTileSet::DestructiveAllocateTileData(int32 NewWidth, int32 NewHeight)
{
	check((NewWidth > 0) && (NewHeight > 0));

	const int32 NumCells = NewWidth * NewHeight;
	ExperimentalPerTileData.Empty(NumCells);
	for (int32 Index = 0; Index < NumCells; ++Index)
	{
		ExperimentalPerTileData.Add(FPaperTileMetadata());
	}

	AllocatedWidth = NewWidth;
	AllocatedHeight = NewHeight;
}

void UPaperTileSet::ReallocateAndCopyTileData()
{
	if ((AllocatedWidth == WidthInTiles) && (AllocatedHeight == HeightInTiles))
	{
		return;
	}

	const int32 SavedWidth = AllocatedWidth;
	const int32 SavedHeight = AllocatedHeight;
	TArray<FPaperTileMetadata> SavedDesignedMap(ExperimentalPerTileData);

	DestructiveAllocateTileData(WidthInTiles, HeightInTiles);

	const int32 CopyWidth = FMath::Min<int32>(WidthInTiles, SavedWidth);
	const int32 CopyHeight = FMath::Min<int32>(HeightInTiles, SavedHeight);

	for (int32 Y = 0; Y < CopyHeight; ++Y)
	{
		for (int32 X = 0; X < CopyWidth; ++X)
		{
			const int32 SrcIndex = Y*SavedWidth + X;
			const int32 DstIndex = Y*WidthInTiles + X;

			ExperimentalPerTileData[DstIndex] = SavedDesignedMap[SrcIndex];
		}
	}

}
