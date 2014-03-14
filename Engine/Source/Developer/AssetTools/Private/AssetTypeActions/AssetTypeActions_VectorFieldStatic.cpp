// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_VectorFieldStatic::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Fields = GetTypedWeakObjectPtrs<UVectorFieldStatic>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("VectorFieldStatic_Reimport", "Reimport"),
		LOCTEXT("VectorFieldStatic_ReimportTooltip", "Reimports the selected meshes from file."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_VectorFieldStatic::ExecuteReimport, Fields ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("VectorFieldStatic_FindInExplorer", "Find Source"),
		LOCTEXT("VectorFieldStatic_FindInExplorerTooltip", "Opens explorer at the location of this asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_VectorFieldStatic::ExecuteFindInExplorer, Fields ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_VectorFieldStatic::CanExecuteSourceCommands, Fields )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("VectorFieldStatic_OpenInExternalEditor", "Open Source"),
		LOCTEXT("VectorFieldStatic_OpenInExternalEditorTooltip", "Opens the selected asset in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_VectorFieldStatic::ExecuteOpenInExternalEditor, Fields ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_VectorFieldStatic::CanExecuteSourceCommands, Fields )
			)
		);
}

void FAssetTypeActions_VectorFieldStatic::ExecuteReimport(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FReimportManager::Instance()->Reimport(Object, /*bAskForNewFileIfMissing=*/true);
		}
	}
}

void FAssetTypeActions_VectorFieldStatic::ExecuteFindInExplorer(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			const FString SourceFilePath = FReimportManager::ResolveImportFilename(Object->SourceFilePath, Object);
			if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
			{
				FPlatformProcess::ExploreFolder( *FPaths::GetPath(SourceFilePath) );
			}
		}
	}
}

void FAssetTypeActions_VectorFieldStatic::ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			const FString SourceFilePath = FReimportManager::ResolveImportFilename(Object->SourceFilePath, Object);
			if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
			{
				FPlatformProcess::LaunchFileInDefaultExternalApplication( *SourceFilePath );
			}
		}
	}
}

bool FAssetTypeActions_VectorFieldStatic::CanExecuteSourceCommands(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects) const
{
	bool bHaveSourceAsset = false;
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			const FString& SourceFilePath = FReimportManager::ResolveImportFilename(Object->SourceFilePath, Object);

			if ( SourceFilePath.Len() && IFileManager::Get().FileSize(*SourceFilePath) != INDEX_NONE )
			{
				return true;
			}
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
