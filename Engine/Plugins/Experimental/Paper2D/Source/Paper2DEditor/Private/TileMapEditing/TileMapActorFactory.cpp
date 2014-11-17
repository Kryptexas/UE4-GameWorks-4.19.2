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

bool UTileMapActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (GetDefault<UPaperRuntimeSettings>()->bEnableTileMapEditing)
	{
		return Super::CanCreateActorFrom(AssetData, OutErrorMsg);
	}
	else
	{
		return false;
	}
}
