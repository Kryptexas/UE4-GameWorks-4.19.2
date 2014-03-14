// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#include "Editor/DataTableEditor/Public/DataTableEditorModule.h"
#include "Editor/DataTableEditor/Public/IDataTableEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_DataTable::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Tables = GetTypedWeakObjectPtrs<UObject>(InObjects);

	TArray<FString> ImportPaths;
	for (auto TableIter = Tables.CreateConstIterator(); TableIter; ++TableIter)
	{
		const UDataTable* CurTable = Cast<UDataTable>((*TableIter).Get());
		if (CurTable)
		{
			ImportPaths.Add(FReimportManager::ResolveImportFilename(CurTable->ImportPath, CurTable));
		}
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTable_Edit", "Edit"),
		LOCTEXT("DataTable_EditTooltip", "Opens the selected tables in the table editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_DataTable::ExecuteEdit, Tables ),
			FCanExecuteAction()
			)
		);


	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTable_Reimport", "Reimport"),
		LOCTEXT("DataTable_ReimportTooltip", "Reimports the selected Data Table from file."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_DataTable::ExecuteReimport, Tables ),
			FCanExecuteAction() 
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTable_FindSource", "Find Source"),
		LOCTEXT("DataTable_FindSourceTooltip", "Opens explorer at the location of this asset's source data."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_DataTable::ExecuteFindInExplorer, ImportPaths ),
			FCanExecuteAction::CreateSP(this, &FAssetTypeActions_DataTable::CanExecuteFindInExplorerSourceCommand, ImportPaths )
			)
		);

	static const TArray<FString> EmptyArray;
	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTable_OpenSourceCSV", "Open Source (.csv)"),
		LOCTEXT("DataTable_OpenSourceCSVTooltip", "Opens the selected asset's source CSV data in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_DataTable::ExecuteOpenInExternalEditor, ImportPaths, EmptyArray ),
			FCanExecuteAction::CreateSP(this, &FAssetTypeActions_DataTable::CanExecuteLaunchExternalSourceCommands, ImportPaths, EmptyArray )
			)
		);

	TArray<FString> XLSExtensions;
	XLSExtensions.Add(TEXT(".xls"));
	XLSExtensions.Add(TEXT(".xlsm"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTable_OpenSourceXLS", "Open Source (.xls/.xlsm)"),
		LOCTEXT("DataTable_OpenSourceXLSTooltip", "Opens the selected asset's source XLS/XLSM data in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_DataTable::ExecuteOpenInExternalEditor, ImportPaths, XLSExtensions ),
			FCanExecuteAction::CreateSP(this, &FAssetTypeActions_DataTable::CanExecuteLaunchExternalSourceCommands, ImportPaths, XLSExtensions )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DataTable_JSON", "JSON"),
		LOCTEXT("DataTable_JSONTooltip", "Creates a JSON version of the data table in the lock."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_DataTable::ExecuteJSON, Tables ),
		FCanExecuteAction()
		)
		);
}

void FAssetTypeActions_DataTable::ExecuteJSON(TArray< TWeakObjectPtr<UObject> > Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Table = Cast<UDataTable>((*ObjIt).Get());
		if (Table != NULL)
		{
			UE_LOG(LogDataTable, Log,  TEXT("JSON DataTable : %s"), *Table->GetTableAsJSON());
		}
	}
}

void FAssetTypeActions_DataTable::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Table = Cast<UDataTable>(*ObjIt);
		if (Table != NULL)
		{
			FDataTableEditorModule& DataTableEditorModule = FModuleManager::LoadModuleChecked<FDataTableEditorModule>( "DataTableEditor" );
			TSharedRef< IDataTableEditor > NewDataTableEditor = DataTableEditorModule.CreateDataTableEditor( EToolkitMode::Standalone, EditWithinLevelEditor, Table );
		}
	}
}

#undef LOCTEXT_NAMESPACE