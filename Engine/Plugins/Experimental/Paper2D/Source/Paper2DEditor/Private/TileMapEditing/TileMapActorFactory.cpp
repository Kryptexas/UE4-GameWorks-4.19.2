// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "AssetData.h"

//////////////////////////////////////////////////////////////////////////
// UTileMapActorFactory

UTileMapActorFactory::UTileMapActorFactory(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DisplayName = NSLOCTEXT("Paper2D", "TileMapFactoryDisplayName", "Paper2D Tile Map");
	NewActorClass = APaperTileMapActor::StaticClass();
}

void UTileMapActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	if (UPaperTileMap* TileMap = Cast<UPaperTileMap>(Asset))
	{
		GEditor->SetActorLabelUnique(NewActor, TileMap->GetName());

		APaperTileMapActor* TypedActor = CastChecked<APaperTileMapActor>(NewActor);
		UPaperTileMapRenderComponent* RenderComponent = TypedActor->RenderComponent;
		check(RenderComponent);

		RenderComponent->UnregisterComponent();
		RenderComponent->TileMap = TileMap;
		RenderComponent->RegisterComponent();
	}
}

void UTileMapActorFactory::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	if (UPaperTileMap* TileMap = Cast<UPaperTileMap>(Asset))
	{
		if (APaperTileMapActor* TypedActor = Cast<APaperTileMapActor>(CDO))
		{
			UPaperTileMapRenderComponent* RenderComponent = TypedActor->RenderComponent;
			check(RenderComponent);

			RenderComponent->TileMap = TileMap;
		}
	}
}

bool UTileMapActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (GetDefault<UPaperRuntimeSettings>()->bEnableTileMapEditing)
	{
		if (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UPaperTileMap::StaticClass()))
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
		OutErrorMsg = NSLOCTEXT("Paper2D", "CanCreateActorFrom_Disabled", "Tile map support is disabled in the Paper2D settings.");
		return false;
	}
}
