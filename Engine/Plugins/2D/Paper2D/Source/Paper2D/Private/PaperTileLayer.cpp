// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "SpriteEditorOnlyTypes.h"
#include "ComponentReregisterContext.h"
#include "PhysicsEngine/BoxElem.h"
#include "PhysicsEngine/BodySetup.h"
#include "PaperTileMap.h"
#include "PaperTileLayer.h"
#include "PaperTileMapComponent.h"

// Handles the rotation and flipping of collision geometry from a tile
// 0,5,6,3 are clockwise rotations of a regular tile
// 4,7,2,1 are clockwise rotations of a horizontally flipped tile
const static FTransform TilePermutationTransforms[8] =
{
	// 000 - normal
	FTransform::Identity,

	// 001 - diagonal
	FTransform(FRotator(  90.0f, 0.0f, 0.0f), FVector::ZeroVector, -PaperAxisX.GetAbs() + PaperAxisY.GetAbs() + PaperAxisZ.GetAbs()),

	// 010 - flip Y
	FTransform(FRotator(-180.0f, 0.0f, 0.0f), FVector::ZeroVector, -PaperAxisX.GetAbs() + PaperAxisY.GetAbs() + PaperAxisZ.GetAbs()),

	// 011 - diagonal then flip Y (rotate 270 clockwise)
	FTransform(FRotator(  90.0f, 0.0f, 0.0f)),

	// 100 - flip X
	FTransform(FRotator::ZeroRotator, FVector::ZeroVector, -PaperAxisX.GetAbs() + PaperAxisY.GetAbs() + PaperAxisZ.GetAbs()),

	// 101 - diagonal then flip X (clockwise 90)
	FTransform(FRotator( -90.0f, 0.0f, 0.0f)),

	// 110 - flip X and flip Y (rotate 180 either way)
	FTransform(FRotator(-180.0f, 0.0f, 0.0f)),

	// 111 - diagonal then flip X and Y
	FTransform(FRotator(-90.0f, 0.0f, 0.0f), FVector::ZeroVector, -PaperAxisX.GetAbs() + PaperAxisY.GetAbs() + PaperAxisZ.GetAbs()),
};

//////////////////////////////////////////////////////////////////////////
// FTileMapLayerReregisterContext

/** Removes all components that use the specified tile map layer from their scenes for the lifetime of the class. */
class FTileMapLayerReregisterContext
{
public:
	/** Initialization constructor. */
	FTileMapLayerReregisterContext(UPaperTileLayer* TargetAsset)
	{
		// Look at tile map components
		for (TObjectIterator<UPaperTileMapComponent> MapIt; MapIt; ++MapIt)
		{
			if (UPaperTileMap* TestMap = (*MapIt)->TileMap)
			{
				if (TestMap->TileLayers.Contains(TargetAsset))
				{
					AddComponentToRefresh(*MapIt);
				}
			}
		}
	}

protected:
	void AddComponentToRefresh(UActorComponent* Component)
	{
		if (ComponentContexts.Num() == 0)
		{
			// wait until resources are released
			FlushRenderingCommands();
		}

		new (ComponentContexts) FComponentReregisterContext(Component);
	}

private:
	/** The recreate contexts for the individual components. */
	TIndirectArray<FComponentReregisterContext> ComponentContexts;
};

//////////////////////////////////////////////////////////////////////////
// FPaperTileLayerToBodySetupBuilder

class FPaperTileLayerToBodySetupBuilder : public FSpriteGeometryCollisionBuilderBase
{
public:
	FPaperTileLayerToBodySetupBuilder(UPaperTileMap* InTileMap, UBodySetup* InBodySetup, float InZOffset)
		: FSpriteGeometryCollisionBuilderBase(InBodySetup)
	{
		UnrealUnitsPerPixel = InTileMap->GetUnrealUnitsPerPixel();
		CollisionThickness = InTileMap->GetCollisionThickness();
		CollisionDomain = InTileMap->GetSpriteCollisionDomain();
		CurrentCellOffset = FVector2D::ZeroVector;
		ZOffsetAmount = InZOffset;
	}

	void SetCellOffset(const FVector2D& NewOffset, const FTransform& NewTransform)
	{
		CurrentCellOffset = NewOffset;
		MyTransform = NewTransform;
	}

protected:
	// FSpriteGeometryCollisionBuilderBase interface
	virtual FVector2D ConvertTextureSpaceToPivotSpace(const FVector2D& Input) const override
	{
		const FVector LocalPos3D = (Input.X * PaperAxisX) - (Input.Y * PaperAxisY);
		const FVector RotatedLocalPos3D = MyTransform.TransformPosition(LocalPos3D);

		const float OutputX = CurrentCellOffset.X + FVector::DotProduct(RotatedLocalPos3D, PaperAxisX);
		const float OutputY = CurrentCellOffset.Y + FVector::DotProduct(RotatedLocalPos3D, PaperAxisY);

		return FVector2D(OutputX, OutputY);
	}

	virtual FVector2D ConvertTextureSpaceToPivotSpaceNoTranslation(const FVector2D& Input) const override
	{
		const FVector LocalPos3D = (Input.X * PaperAxisX) + (Input.Y * PaperAxisY);
		const FVector RotatedLocalPos3D = MyTransform.TransformVector(LocalPos3D);

		const float OutputX = FVector::DotProduct(RotatedLocalPos3D, PaperAxisX);
		const float OutputY = FVector::DotProduct(RotatedLocalPos3D, PaperAxisY);

		return FVector2D(OutputX, OutputY);
	}
	// End of FSpriteGeometryCollisionBuilderBase

protected:
	FTransform MyTransform;
	UPaperTileLayer* MySprite;
	FVector2D CurrentCellOffset;
};

//////////////////////////////////////////////////////////////////////////
// UPaperTileLayer

UPaperTileLayer::UPaperTileLayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LayerColor(FLinearColor::White)
{
	LayerWidth = 4;
	LayerHeight = 4;

#if WITH_EDITORONLY_DATA
	LayerOpacity = 1.0f;

	bHiddenInEditor = false;
#endif

	DestructiveAllocateMap(LayerWidth, LayerHeight);
}

void UPaperTileLayer::DestructiveAllocateMap(int32 NewWidth, int32 NewHeight)
{
	check((NewWidth > 0) && (NewHeight > 0));

	const int32 NumCells = NewWidth * NewHeight;
	AllocatedCells.Empty(NumCells);
	AllocatedCells.AddDefaulted(NumCells);

	AllocatedWidth = NewWidth;
	AllocatedHeight = NewHeight;
}

void UPaperTileLayer::ResizeMap(int32 NewWidth, int32 NewHeight)
{
	if ((LayerWidth != NewWidth) || (LayerHeight != NewHeight))
	{
		LayerWidth = NewWidth;
		LayerHeight = NewHeight;
		ReallocateAndCopyMap();
	}
}

void UPaperTileLayer::ReallocateAndCopyMap()
{
	const int32 SavedWidth = AllocatedWidth;
	const int32 SavedHeight = AllocatedHeight;
	TArray<FPaperTileInfo> SavedDesignedMap(AllocatedCells);

	DestructiveAllocateMap(LayerWidth, LayerHeight);

	const int32 CopyWidth = FMath::Min<int32>(LayerWidth, SavedWidth);
	const int32 CopyHeight = FMath::Min<int32>(LayerHeight, SavedHeight);

	for (int32 Y = 0; Y < CopyHeight; ++Y)
	{
		for (int32 X = 0; X < CopyWidth; ++X)
		{
			const int32 SrcIndex = Y*SavedWidth + X;
			const int32 DstIndex = Y*LayerWidth + X;

			AllocatedCells[DstIndex] = SavedDesignedMap[SrcIndex];
		}
	}
}

#if WITH_EDITOR

void UPaperTileLayer::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FTileMapLayerReregisterContext ReregisterExistingComponents(this);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPaperTileLayer, LayerWidth)) || (PropertyName == GET_MEMBER_NAME_CHECKED(UPaperTileLayer, LayerHeight)))
	{
		// Minimum size
		LayerWidth = FMath::Max<int32>(1, LayerWidth);
		LayerHeight = FMath::Max<int32>(1, LayerHeight);

		// Resize the map, trying to preserve existing data
		ReallocateAndCopyMap();
	}
// 	else if (PropertyName == TEXT("AllocatedCells"))
// 	{
// 		BakeMap();
// 	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

UPaperTileMap* UPaperTileLayer::GetTileMap() const
{
	return CastChecked<UPaperTileMap>(GetOuter());
}

int32 UPaperTileLayer::GetLayerIndex() const
{
	return GetTileMap()->TileLayers.Find(const_cast<UPaperTileLayer*>(this));
}

FPaperTileInfo UPaperTileLayer::GetCell(int32 X, int32 Y) const
{
	if ((X < 0) || (X >= LayerWidth) || (Y < 0) || (Y >= LayerHeight))
	{
		return FPaperTileInfo();
	}
	else
	{
		return AllocatedCells[X + (Y*LayerWidth)];
	}
}

void UPaperTileLayer::SetCell(int32 X, int32 Y, const FPaperTileInfo& NewValue)
{
	if ((X < 0) || (X >= LayerWidth) || (Y < 0) || (Y >= LayerHeight))
	{
	}
	else
	{
		AllocatedCells[X + (Y*LayerWidth)] = NewValue;
	}
}

void UPaperTileLayer::AugmentBodySetup(UBodySetup* ShapeBodySetup)
{
	UPaperTileMap* TileMap = GetTileMap();
	const float TileWidth = TileMap->TileWidth;
	const float TileHeight = TileMap->TileHeight;

	//@TODO: Determine if we want collision to be attached to the layer or always relative to the zero layer (probably need a per-layer config value / option, as you may even want to inset layer 0's collision so it's flush with the surface (imagine a top-down game))
	const float ZOffset = 0.0f;

	// Generate collision for all cells that contain a tile with collision metadata
	FPaperTileLayerToBodySetupBuilder CollisionBuilder(GetTileMap(), ShapeBodySetup, ZOffset);

	for (int32 CellY = 0; CellY < LayerHeight; ++CellY)
	{
		for (int32 CellX = 0; CellX < LayerWidth; ++CellX)
		{
			const FPaperTileInfo CellInfo = GetCell(CellX, CellY);

			if (CellInfo.IsValid())
			{
				if (const FPaperTileMetadata* CellMetadata = CellInfo.TileSet->GetTileMetadata(CellInfo.GetTileIndex()))
				{
					const int32 Flags = CellInfo.GetFlagsAsIndex();

					const FTransform& LocalTransform = TilePermutationTransforms[Flags];
					const FVector2D CellOffset(TileWidth * CellX, TileHeight * -CellY);
					CollisionBuilder.SetCellOffset(CellOffset, LocalTransform);

					CollisionBuilder.ProcessGeometry(CellMetadata->CollisionData);
				}
			}
		}
	}
}

FLinearColor UPaperTileLayer::GetLayerColor() const
{
	return LayerColor;
}

void UPaperTileLayer::SetLayerColor(FLinearColor NewColor)
{
	LayerColor = NewColor;
}

void UPaperTileLayer::ConvertToTileSetPerCell()
{
	AllocatedCells.Empty(AllocatedGrid_DEPRECATED.Num());

	const int32 NumCells = AllocatedWidth * AllocatedHeight;
	for (int32 Index = 0; Index < NumCells; ++Index)
	{
		FPaperTileInfo* Info = new (AllocatedCells) FPaperTileInfo();
		Info->TileSet = TileSet_DEPRECATED;
		Info->PackedTileIndex = AllocatedGrid_DEPRECATED[Index];
	}
}

bool UPaperTileLayer::UsesTileSet(UPaperTileSet* TileSet) const
{
	for (const FPaperTileInfo& TileInfo : AllocatedCells)
	{
		if (TileInfo.TileSet == TileSet)
		{
			if (TileInfo.IsValid())
			{
				return true;
			}
		}
	}

	return false;
}
