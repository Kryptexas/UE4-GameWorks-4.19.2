// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodySetup2D.h"

//////////////////////////////////////////////////////////////////////////
// UPaperTileMap

UPaperTileMap::UPaperTileMap(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MapWidth = 4;
	MapHeight = 4;
	TileWidth = 32;
	TileHeight = 32;
	PixelsPerUnit = 1.0f;
	SeparationPerTileX = 0.0f;
	SeparationPerTileY = 0.0f;
	SeparationPerLayer = 64.0f;
	SpriteCollisionDomain = ESpriteCollisionMode::None;

#if WITH_EDITORONLY_DATA
	SelectedLayerIndex = INDEX_NONE;
	LayerNameIndex = 1;
#endif

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Paper2D/DefaultSpriteMaterial"));
	Material = DefaultMaterial.Object;
}

#if WITH_EDITOR

#include "PaperTileMapRenderComponent.h"
#include "ComponentReregisterContext.h"

void UPaperTileMap::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//@TODO: Determine when these are really needed, as they're seriously expensive!
	TComponentReregisterContext<UPaperTileMapRenderComponent> ReregisterStaticComponents;

	ValidateSelectedLayerIndex();

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPaperTileMap, MapWidth)) || (PropertyName == GET_MEMBER_NAME_CHECKED(UPaperTileMap, MapHeight)))
	{
		MapWidth = FMath::Max(MapWidth, 1);
		MapHeight = FMath::Max(MapHeight, 1);

		// Resize all of the existing layers
		for (int32 LayerIndex = 0; LayerIndex < TileLayers.Num(); ++LayerIndex)
		{
			UPaperTileLayer* TileLayer = TileLayers[LayerIndex];
			TileLayer->ResizeMap(MapWidth, MapHeight);
		}
	}

	if (!IsTemplate())
	{
		UpdateBodySetup();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UPaperTileMap::PostLoad()
{
	Super::PostLoad();
	ValidateSelectedLayerIndex();
}

void UPaperTileMap::ValidateSelectedLayerIndex()
{
	if (!TileLayers.IsValidIndex(SelectedLayerIndex))
	{
		// Select the top-most visible layer
		SelectedLayerIndex = INDEX_NONE;
		for (int32 LayerIndex = 0; (LayerIndex < TileLayers.Num()) && (SelectedLayerIndex == INDEX_NONE); ++LayerIndex)
		{
			if (!TileLayers[LayerIndex]->bHiddenInEditor)
			{
				SelectedLayerIndex = LayerIndex;
			}
		}

		if ((SelectedLayerIndex == INDEX_NONE) && (TileLayers.Num() > 0))
		{
			SelectedLayerIndex = 0;
		}
	}
}

#endif


void UPaperTileMap::UpdateBodySetup()
{
	// Ensure we have the data structure for the desired collision method
	switch (SpriteCollisionDomain)
	{
	case ESpriteCollisionMode::Use3DPhysics:
		BodySetup = nullptr;
		if (BodySetup == nullptr)
		{
			BodySetup = NewObject<UBodySetup>(this);
		}
		break;
	case ESpriteCollisionMode::Use2DPhysics:
		BodySetup = nullptr;
		if (BodySetup == nullptr)
		{
			BodySetup = NewObject<UBodySetup2D>(this);
		}
		break;
	case ESpriteCollisionMode::None:
		BodySetup = nullptr;
		break;
	}

	if (SpriteCollisionDomain != ESpriteCollisionMode::None)
	{
		if (SpriteCollisionDomain == ESpriteCollisionMode::Use3DPhysics)
		{
			BodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;

			BodySetup->AggGeom.BoxElems.Empty();
			for (int32 LayerIndex = 0; LayerIndex < TileLayers.Num(); ++LayerIndex)
			{
				TileLayers[LayerIndex]->AugmentBodySetup(BodySetup);
			}
		}

		//@TODO: BOX2D: Add support for 2D physics on tile maps
	}
}

FVector UPaperTileMap::GetTilePositionInLocalSpace(int32 TileX, int32 TileY, int32 LayerIndex) const
{
	//@TODO: Tile pivot issue
	const FVector PartialX = PaperAxisX * (TileX - 0.5f) * TileWidth;
	const FVector PartialY = PaperAxisY * -(TileY - 0.5f) * TileHeight;
	const FVector PartialZ = PaperAxisZ * (LayerIndex * SeparationPerLayer);

	const FVector LocalPos(PartialX + PartialY + PartialZ);
	
	return LocalPos;
}

FBoxSphereBounds UPaperTileMap::GetRenderBounds() const
{
	//@TODO: Tile pivot issue
	const float Depth = SeparationPerLayer * (TileLayers.Num() - 1);
	const float HalfThickness = 2.0f;
	const FVector TopLeft((-0.5f)*TileWidth, -HalfThickness - Depth, -(MapHeight - 0.5f) * TileHeight);
	const FVector Dimenisons(MapWidth*TileWidth, Depth + 2 * HalfThickness, MapHeight * TileHeight);

	const FBox Box(TopLeft, TopLeft + Dimenisons);
	return FBoxSphereBounds(Box);
}