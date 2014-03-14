// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperSprite.h"

//////////////////////////////////////////////////////////////////////////
// UPaperTileLayer

UPaperTileLayer::UPaperTileLayer(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	LayerWidth = 4;
	LayerHeight = 4;

	DestructiveAllocateMap(LayerWidth, LayerHeight);
}

void UPaperTileLayer::DestructiveAllocateMap(int32 NewWidth, int32 NewHeight)
{
	check((NewWidth > 0) && (NewHeight > 0));

	const int32 NumCells = NewWidth * NewHeight;
	AllocatedGrid.Empty(NumCells);
	AllocatedGrid.AddZeroed(NumCells);

	AllocatedWidth = NewWidth;
	AllocatedHeight = NewHeight;
}

void UPaperTileLayer::ResizeMap(int32 NewWidth, int32 NewHeight)
{
	LayerWidth = NewWidth;
	LayerHeight = NewHeight;
	ReallocateAndCopyMap();
}

void UPaperTileLayer::ReallocateAndCopyMap()
{
	const int32 SavedWidth = AllocatedWidth;
	const int32 SavedHeight = AllocatedHeight;
	TArray<int32> SavedDesignedMap(AllocatedGrid);

	DestructiveAllocateMap(LayerWidth, LayerHeight);

	const int32 CopyWidth = FMath::Min<int32>(LayerWidth, SavedWidth);
	const int32 CopyHeight = FMath::Min<int32>(LayerHeight, SavedHeight);

	for (int32 Y = 0; Y < CopyHeight; ++Y)
	{
		for (int32 X = 0; X < CopyWidth; ++X)
		{
			const int32 SrcIndex = Y*SavedWidth + X;
			const int32 DstIndex = Y*LayerWidth + X;

			AllocatedGrid[DstIndex] = SavedDesignedMap[SrcIndex];
		}
	}

//	BakeMap();
}

#if WITH_EDITORONLY_DATA
void UPaperTileLayer::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPaperTileLayer, LayerWidth)) || (PropertyName == GET_MEMBER_NAME_CHECKED(UPaperTileLayer, LayerHeight)))
	{
		// Minimum size
		LayerWidth = FMath::Max<int32>(1, LayerWidth);
		LayerHeight = FMath::Max<int32>(1, LayerHeight);

		// Resize the map, trying to preserve existing data
		ReallocateAndCopyMap();
	}
// 	else if (PropertyName == TEXT("AllocatedGrid"))
// 	{
// 		BakeMap();
// 	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

UPaperTileMapRenderComponent* UPaperTileLayer::GetTileMap() const
{
	return CastChecked<UPaperTileMapRenderComponent>(GetOuter());
}

int32 UPaperTileLayer::GetCell(int32 X, int32 Y) const
{
	if ((X < 0) || (X >= LayerWidth) || (Y < 0) || (Y >= LayerHeight))
	{
		return INDEX_NONE;
	}
	else
	{
		return AllocatedGrid[X + (Y*LayerWidth)];
	}
}

void UPaperTileLayer::SetCell(int32 X, int32 Y, int32 NewValue)
{
	if ((X < 0) || (X >= LayerWidth) || (Y < 0) || (Y >= LayerHeight))
	{
	}
	else
	{
		AllocatedGrid[X + (Y*LayerWidth)] = NewValue;
	}
}

void UPaperTileLayer::AugmentBodySetup(UBodySetup* ShapeBodySetup)
{
	if ( bCollisionLayer )
	{
		//@TODO: Tile pivot issue
		//@TODO: Layer thickness issue
		const float TileWidth = GetTileMap()->TileWidth;
		const float TileHeight = GetTileMap()->TileHeight;
		const float TileThickness = 64.0f;

		//@TODO: When the origin of the component changes, this logic will need to be adjusted as well
		// The origin is currently the top left
		const float XOrigin = 0;
		const float YOrigin = 0;

		// Create a box element for every non-zero value in the layer
		for (int32 XValue = 0; XValue < LayerWidth; ++XValue)
		{
			for (int32 YValue = 0; YValue < LayerHeight; ++YValue)
			{
				if ( GetCell(XValue, YValue) != 0 )
				{
					FKBoxElem* NewBox = new(ShapeBodySetup->AggGeom.BoxElems) FKBoxElem(TileWidth, TileThickness, TileHeight);

					FVector BoxPosition;
					BoxPosition.X = XOrigin + XValue * TileWidth;
					BoxPosition.Y = TileThickness * 0.5f;
					BoxPosition.Z = YOrigin - YValue * TileHeight;

					NewBox->Center = BoxPosition;
				}
			}
		}
	}
}