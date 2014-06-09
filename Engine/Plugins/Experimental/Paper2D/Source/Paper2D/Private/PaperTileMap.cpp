// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodySetup2D.h"

//////////////////////////////////////////////////////////////////////////
// UPaperTileMap

UPaperTileMap::UPaperTileMap(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	MapWidth = 4;
	MapHeight = 4;
	TileWidth = 32;
	TileHeight = 32;
	PixelsPerUnit = 1.0f;
	SpriteCollisionDomain = ESpriteCollisionMode::None;

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Paper2D/DefaultSpriteMaterial"));
	Material = DefaultMaterial.Object;
}

#if WITH_EDITOR
void UPaperTileMap::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

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
	//@TODO: Layer depth, multiply by layer depth here!
	//@TODO: Tile pivot issue
	const FVector LocalPos(PaperAxisX * (TileX - 0.5f) * TileWidth + PaperAxisY * -(TileY - 0.5f) * TileHeight);
	
	return LocalPos;
}

FBoxSphereBounds UPaperTileMap::GetRenderBounds() const
{
	//@TODO: Tile pivot issue
	//@TODO: Layer thickness issue
	const float HalfThickness = 2.0f;
	const FVector TopLeft((-0.5f)*TileWidth, -HalfThickness, -(MapHeight - 0.5f) * TileHeight);
	const FVector Dimenisons(MapWidth*TileWidth, 2 * HalfThickness, MapHeight * TileHeight);

	const FBox Box(TopLeft, TopLeft + Dimenisons);
	return FBoxSphereBounds(Box);
}