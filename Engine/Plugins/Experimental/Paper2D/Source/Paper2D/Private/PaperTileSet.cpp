// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UPaperTileSet

UPaperTileSet::UPaperTileSet(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	TileWidth = 32;
	TileHeight = 32;
}

int32 UPaperTileSet::GetTileCount() const
{
	if (TileSheet != NULL)
	{
		checkSlow((TileWidth > 0) && (TileHeight > 0));
		const int32 TextureWidth = TileSheet->GetSizeX();
		const int32 TextureHeight = TileSheet->GetSizeY();

		const int32 CellsX = TextureWidth / TileWidth;
		const int32 CellsY = TextureHeight / TileHeight;

		return CellsX * CellsY;
	}
	else
	{
		return 0;
	}
}

int32 UPaperTileSet::GetTileCountX() const
{
	if (TileSheet != NULL)
	{
		checkSlow(TileWidth > 0);
		const int32 TextureWidth = TileSheet->GetSizeX();
		const int32 CellsX = TextureWidth / TileWidth;
		return CellsX;
	}
	else
	{
		return 0;
	}
}

int32 UPaperTileSet::GetTileCountY() const
{
	if (TileSheet != NULL)
	{
		checkSlow(TileHeight > 0);
		const int32 TextureHeight = TileSheet->GetSizeY();
		const int32 CellsY = TextureHeight / TileHeight;
		return CellsY;
	}
	else
	{
		return 0;
	}
}

bool UPaperTileSet::GetTileUV(int32 TileIndex, /*out*/ FVector2D& Out_TileUV) const
{
	//@TODO: Performance: should cache this stuff
	if (TileSheet != NULL)
	{
		checkSlow((TileWidth > 0) && (TileHeight > 0));
		const int32 TextureWidth = TileSheet->GetSizeX();
		const int32 TextureHeight = TileSheet->GetSizeY();

		const int32 CellsX = TextureWidth / TileWidth;
		const int32 CellsY = TextureHeight / TileHeight;

		const int32 NumCells = CellsX * CellsY;

		if ((TileIndex < 0) || (TileIndex >= NumCells))
		{
			return false;
		}
		else
		{
			const int32 X = TileIndex % CellsX;
			const int32 Y = TileIndex / CellsX;

			Out_TileUV.X = X * TileWidth;
			Out_TileUV.Y = Y * TileHeight;
			return true;
		}
	}
	else
	{
		return false;
	}
}
