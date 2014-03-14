// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#include "Editor/CurveTableEditor/Public/CurveTableEditorModule.h"
#include "Editor/CurveTableEditor/Public/ICurveTableEditor.h"

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

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CurveTable_Edit", "Edit"),
		LOCTEXT("CurveTable_EditTooltip", "Opens the selected tables in the table editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_CurveTable::ExecuteEdit, Tables ),
			FCanExecuteAction()
			)
		);


	MenuBuilder.AddMenuEntry(
		LOCTEXT("CurveTable_Reimport", "Reimport"),
		LOCTEXT("CurveTable_ReimportTooltip", "Reimports the selected Curve Table from file."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_CurveTable::ExecuteReimport, Tables ),
		FCanExecuteAction() 
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CurveTable_FindSource", "Find Source"),
		LOCTEXT("CurveTable_FindSourceTooltip", "Opens explorer at the location of this asset's source data."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_CurveTable::ExecuteFindInExplorer, ImportPaths ),
			FCanExecuteAction::CreateSP(this, &FAssetTypeActions_CurveTable::CanExecuteFindInExplorerSourceCommand, ImportPaths )
			)
		);

	static const TArray<FString> EmptyArray;
	MenuBuilder.AddMenuEntry(
		LOCTEXT("CurveTable_OpenSourceCSV", "Open Source (.csv)"),
		LOCTEXT("CurveTable_OpenSourceCSVTooltip", "Opens the selected asset's source CSV data in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_CurveTable::ExecuteOpenInExternalEditor, ImportPaths, EmptyArray ),
			FCanExecuteAction::CreateSP(this, &FAssetTypeActions_CurveTable::CanExecuteLaunchExternalSourceCommands, ImportPaths, EmptyArray )
			)
		);

	TArray<FString> XLSExtensions;
	XLSExtensions.Add(TEXT(".xls"));
	XLSExtensions.Add(TEXT(".xlsm"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("CurveTable_OpenSourceXLS", "Open Source (.xls/.xlsm)"),
		LOCTEXT("CurveTable_OpenSourceXLSTooltip", "Opens the selected asset's source XLS/XLSM data in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_CurveTable::ExecuteOpenInExternalEditor, ImportPaths, XLSExtensions ),
			FCanExecuteAction::CreateSP(this, &FAssetTypeActions_CurveTable::CanExecuteLaunchExternalSourceCommands, ImportPaths, XLSExtensions )
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

#undef LOCTEXT_NAMESPACE