// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
//#include "AssetToolsModule.h"
//#include "Toolkits/AssetEditorManager.h"
//#include "AssetRegistryModule.h"
#include "TileSetAssetTypeActions.h"
#include "TileSetEditor.h"


#define LOCTEXT_NAMESPACE "AssetTypeActions"


//////////////////////////////////////////////////////////////////////////
// FTileSetAssetTypeActions

FText FTileSetAssetTypeActions::GetName() const
{
	return LOCTEXT("FTileSetAssetTypeActionsName", "Tile Set");
}

FColor FTileSetAssetTypeActions::GetTypeColor() const
{
	return FColor::Cyan;
}

UClass* FTileSetAssetTypeActions::GetSupportedClass() const
{
	return UPaperTileSet::StaticClass();
}

void FTileSetAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UPaperTileSet* TileSet = Cast<UPaperTileSet>(*ObjIt))
		{
			TSharedRef<FTileSetEditor> NewTileSetEditor(new FTileSetEditor());
			NewTileSetEditor->InitTileSetEditor(Mode, EditWithinLevelEditor, TileSet);
		}
	}
}

uint32 FTileSetAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE