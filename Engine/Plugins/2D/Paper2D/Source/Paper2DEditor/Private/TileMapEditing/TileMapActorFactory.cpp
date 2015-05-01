// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "AssetData.h"
#include "TileMapActorFactory.h"
#include "PaperImporterSettings.h"

//////////////////////////////////////////////////////////////////////////
// UTileMapActorFactory

UTileMapActorFactory::UTileMapActorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayName = NSLOCTEXT("Paper2D", "TileMapFactoryDisplayName", "Paper2D Tile Map");
	NewActorClass = APaperTileMapActor::StaticClass();
}

void UTileMapActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	APaperTileMapActor* TypedActor = CastChecked<APaperTileMapActor>(NewActor);
	UPaperTileMapComponent* RenderComponent = TypedActor->GetRenderComponent();
	check(RenderComponent);

	if (UPaperTileMap* TileMap = Cast<UPaperTileMap>(Asset))
	{
		RenderComponent->UnregisterComponent();
		RenderComponent->TileMap = TileMap;
		RenderComponent->RegisterComponent();
	}
	else if (UPaperTileSet* TileSet = Cast<UPaperTileSet>(Asset))
	{
		if (RenderComponent->TileMap != nullptr)
		{
			RenderComponent->UnregisterComponent();
			RenderComponent->TileMap->TileWidth = TileSet->TileWidth;
			RenderComponent->TileMap->TileHeight = TileSet->TileHeight;
			RenderComponent->TileMap->SelectedTileSet = TileSet;
			//@TODO: Initialize the tile map material here too, based on analysis of the TileSheet texture
			RenderComponent->RegisterComponent();
		}
	}

	if (RenderComponent->OwnsTileMap())
	{
		RenderComponent->TileMap->InitializeNewEmptyTileMap();
		RenderComponent->TileMap->PixelsPerUnrealUnit = GetDefault<UPaperImporterSettings>()->GetDefaultPixelsPerUnrealUnit();
	}
}

void UTileMapActorFactory::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	if (APaperTileMapActor* TypedActor = Cast<APaperTileMapActor>(CDO))
	{
		UPaperTileMapComponent* RenderComponent = TypedActor->GetRenderComponent();
		check(RenderComponent);

		if (UPaperTileMap* TileMap = Cast<UPaperTileMap>(Asset))
		{
			RenderComponent->TileMap = TileMap;
		}
		else if (UPaperTileSet* TileSet = Cast<UPaperTileSet>(Asset))
		{
			if (RenderComponent->TileMap != nullptr)
			{
				RenderComponent->TileMap->TileWidth = TileSet->TileWidth;
				RenderComponent->TileMap->TileHeight = TileSet->TileHeight;
				RenderComponent->TileMap->SelectedTileSet = TileSet;
				//@TODO: Initialize the tile map material here too, based on analysis of the TileSheet texture
			}
		}

		if (RenderComponent->OwnsTileMap())
		{
			RenderComponent->TileMap->InitializeNewEmptyTileMap();
			RenderComponent->TileMap->PixelsPerUnrealUnit = GetDefault<UPaperImporterSettings>()->GetDefaultPixelsPerUnrealUnit();
		}
	}
}

bool UTileMapActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (AssetData.IsValid())
	{
		UClass* AssetClass = AssetData.GetClass();
		if ((AssetClass != nullptr) && (AssetClass->IsChildOf(UPaperTileMap::StaticClass()) || AssetClass->IsChildOf(UPaperTileSet::StaticClass())))
		{
			return true;
		}
		else
		{
			OutErrorMsg = NSLOCTEXT("Paper2D", "CanCreateActorFrom_NoTileMap", "No tile map was specified.");
			return false;
		}
	}
	else
	{
		return true;
	}
}
