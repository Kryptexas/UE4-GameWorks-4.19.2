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

void UPaperTileSet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	Margin = FMath::Max<int32>(Margin, 0);
	Spacing = FMath::Max<int32>(Spacing, 0);

	TileWidth = FMath::Max<int32>(TileWidth, 1);
	TileHeight = FMath::Max<int32>(TileHeight, 1);

	//@TODO: Determine when these are really needed, as they're seriously expensive!
	TComponentReregisterContext<UPaperTileMapComponent> ReregisterStaticComponents;

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
