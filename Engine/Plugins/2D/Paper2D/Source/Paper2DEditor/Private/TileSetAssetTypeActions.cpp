// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "TileSetAssetTypeActions.h"
#include "TileSetEditor.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "PaperTileMapFactory.h"
#include "TileSetEditor/TileSheetPaddingFactory.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FTileSetAssetTypeActions

FTileSetAssetTypeActions::FTileSetAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
{
}

FText FTileSetAssetTypeActions::GetName() const
{
	return LOCTEXT("FTileSetAssetTypeActionsName", "Tile Set");
}

FColor FTileSetAssetTypeActions::GetTypeColor() const
{
	return FColorList::Orange;
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
	return MyAssetCategory;
}

void FTileSetAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	TArray<TWeakObjectPtr<UPaperTileSet>> TileSets = GetTypedWeakObjectPtrs<UPaperTileSet>(InObjects);

	if (TileSets.Num() == 1)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("TileSet_CreateTileMap", "Create Tile Map"),
			LOCTEXT("TileSet_CreateTileMapTooltip", "Creates a tile map using the selected tile set as a guide for tile size, etc..."),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.PaperTileSet"),
			FUIAction(FExecuteAction::CreateSP(this, &FTileSetAssetTypeActions::ExecuteCreateTileMap, TileSets[0]))
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("TileSet_ConditionTileSet", "Condition Tile Sheet Texture"),
			LOCTEXT("TileSet_ConditionTileSetTooltip", "Conditions the tile sheet texture for the selected tile set by duplicating tile edges to create a buffer zone around each tile"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.Texture2D"),
			FUIAction(FExecuteAction::CreateSP(this, &FTileSetAssetTypeActions::ExecutePadTileSetTexture, TileSets[0]))
			);
	}
}

void FTileSetAssetTypeActions::ExecuteCreateTileMap(TWeakObjectPtr<UPaperTileSet> TileSetPtr)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	const FString SuffixToIgnore(TEXT("Set"));
	const FString TileMapSuffix(TEXT("Map"));

	if (UPaperTileSet* TileSet = TileSetPtr.Get())
	{
		// Figure out what to call the new tile map
		FString EffectiveTileSetName = TileSet->GetName();
		EffectiveTileSetName.RemoveFromEnd(SuffixToIgnore);

		const FString TileSetPathName = TileSet->GetOutermost()->GetPathName();
		const FString LongPackagePath = FPackageName::GetLongPackagePath(TileSetPathName);

		const FString NewTileMapDefaultPath = LongPackagePath / EffectiveTileSetName;

		// Make sure the name is unique
		FString AssetName;
		FString PackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(NewTileMapDefaultPath, TileMapSuffix, /*out*/ PackageName, /*out*/ AssetName);
		const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

		// Create the new tile map
		UPaperTileMapFactory* TileMapFactory = NewObject<UPaperTileMapFactory>();
		TileMapFactory->InitialTileSet = TileSet;
		ContentBrowserModule.Get().CreateNewAsset(AssetName, PackagePath, UPaperTileMap::StaticClass(), TileMapFactory);
	}
}

void FTileSetAssetTypeActions::ExecutePadTileSetTexture(TWeakObjectPtr<UPaperTileSet> TileSetPtr)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	if (UPaperTileSet* TileSet = TileSetPtr.Get())
	{
		if (TileSet->TileSheet != nullptr)
		{
			const FString TileSheetSuffix(TEXT("Padded"));
			const FString TileSheetPathName = TileSet->TileSheet->GetOutermost()->GetPathName();
			const FString LongPackagePath = FPackageName::GetLongPackagePath(TileSheetPathName);

			const FString EffectiveTileSheetName = TileSet->TileSheet->GetName();
			const FString NewTileSheetDefaultPath = LongPackagePath / EffectiveTileSheetName;

			// Make sure the name is unique
			FString AssetName;
			FString PackageName;
			AssetToolsModule.Get().CreateUniqueAssetName(NewTileSheetDefaultPath, TileSheetSuffix, /*out*/ PackageName, /*out*/ AssetName);
			const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

			// Create the new tile sheet
			UTileSheetPaddingFactory* TileSheetPaddingFactory = NewObject<UTileSheetPaddingFactory>();
			TileSheetPaddingFactory->SourceTileSet = TileSet;
			ContentBrowserModule.Get().CreateNewAsset(AssetName, PackagePath, UTexture::StaticClass(), TileSheetPaddingFactory);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE