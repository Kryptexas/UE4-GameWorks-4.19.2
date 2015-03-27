// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#include "Editor/CurveTableEditor/Public/CurveTableEditorModule.h"
#include "Editor/CurveTableEditor/Public/ICurveTableEditor.h"
#include "Engine/CurveTable.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_CurveTable::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Tables = GetTypedWeakObjectPtrs<UObject>(InObjects);

	TArray<FString> ImportPaths;
	for (auto TableIter = Tables.CreateConstIterator(); TableIter; ++TableIter)
	{
		const UCurveTable* CurTable = Cast<UCurveTable>((*TableIter).Get());
		if (CurTable)
		{
			ImportPaths.Add(FReimportManager::ResolveImportFilename(CurTable->ImportPath, CurTable));
		}
	}

	TArray<FString> PotentialFileExtensions;
	PotentialFileExtensions.Add(TEXT(".xls"));
	PotentialFileExtensions.Add(TEXT(".xlsm"));
	PotentialFileExtensions.Add(TEXT(".csv"));
	PotentialFileExtensions.Add(TEXT(".json"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("CurveTable_OpenSourceData", "Open Source Data"),
		LOCTEXT("CurveTable_OpenSourceDataTooltip", "Opens the data table's source data file in an external editor. It will search using the following extensions: .xls/.xlsm/.csv/.json"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_CurveTable::ExecuteFindSourceFileInExplorer, ImportPaths, PotentialFileExtensions ),
			FCanExecuteAction::CreateSP(this, &FAssetTypeActions_CurveTable::CanExecuteFindSourceFileInExplorer, ImportPaths, PotentialFileExtensions)
			)
		);
}

void FAssetTypeActions_CurveTable::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Table = Cast<UCurveTable>(*ObjIt);
		if (Table != NULL)
		{
			FCurveTableEditorModule& CurveTableEditorModule = FModuleManager::LoadModuleChecked<FCurveTableEditorModule>( "CurveTableEditor" );
			TSharedRef< ICurveTableEditor > NewCurveTableEditor = CurveTableEditorModule.CreateCurveTableEditor( EToolkitMode::Standalone, EditWithinLevelEditor, Table );
		}
	}
}

void FAssetTypeActions_CurveTable::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto CurveTable = CastChecked<UCurveTable>(Asset);
		OutSourceFilePaths.Add(FReimportManager::ResolveImportFilename(CurveTable->ImportPath, CurveTable));
	}
}

#undef LOCTEXT_NAMESPACE