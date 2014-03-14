// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#include "Editor/StaticMeshEditor/Public/StaticMeshEditorModule.h"

#include "Editor/DestructibleMeshEditor/Public/DestructibleMeshEditorModule.h"
#include "Editor/DestructibleMeshEditor/Public/IDestructibleMeshEditor.h"

#include "ApexDestructibleAssetImport.h"

#include "FbxMeshUtils.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_StaticMesh::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Meshes = GetTypedWeakObjectPtrs<UStaticMesh>(InObjects);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_Edit", "Edit"),
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_EditTooltip", "Opens the selected meshes in the static mesh editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_StaticMesh::ExecuteEdit, Meshes ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_Reimport", "Reimport"),
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_ReimportTooltip", "Reimports the selected meshes from file."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_StaticMesh::ExecuteReimport, Meshes ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddSubMenu(
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_ImportLOD", "ImportLOD"),
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_ImportLODtooltip", "Imports meshes into the LODs"),
		FNewMenuDelegate::CreateSP( this, &FAssetTypeActions_StaticMesh::GetImportLODMenu, Meshes )
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_FindInExplorer", "Find Source"),
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_FindInExplorerTooltip", "Opens explorer at the location of this asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_StaticMesh::ExecuteFindInExplorer, Meshes ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_StaticMesh::CanExecuteSourceCommands, Meshes )
			)
		);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_OpenInExternalEditor", "Open Source"),
		NSLOCTEXT("AssetTypeActions_StaticMesh", "StaticMesh_OpenInExternalEditorTooltip", "Opens the selected asset in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_StaticMesh::ExecuteOpenInExternalEditor, Meshes ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_StaticMesh::CanExecuteSourceCommands, Meshes )
			)
		);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("AssetTypeActions_StaticMesh", "ObjectContext_CreateDestructibleMesh", "Create Destructible Mesh"),
		NSLOCTEXT("AssetTypeActions_StaticMesh", "ObjectContext_CreateDestructibleMeshTooltip", "Creates a DestructibleMesh from the StaticMesh and opens it in the DestructibleMesh editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_StaticMesh::ExecuteCreateDestructibleMesh, Meshes ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_StaticMesh::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Mesh = Cast<UStaticMesh>(*ObjIt);
		if (Mesh != NULL)
		{
			IStaticMeshEditorModule* StaticMeshEditorModule = &FModuleManager::LoadModuleChecked<IStaticMeshEditorModule>( "StaticMeshEditor" );
			StaticMeshEditorModule->CreateStaticMeshEditor(Mode, EditWithinLevelEditor, Mesh);
		}
	}
}

UThumbnailInfo* FAssetTypeActions_StaticMesh::GetThumbnailInfo(UObject* Asset) const
{
	UStaticMesh* StaticMesh = CastChecked<UStaticMesh>(Asset);
	UThumbnailInfo* ThumbnailInfo = StaticMesh->ThumbnailInfo;
	if ( ThumbnailInfo == NULL )
	{
		ThumbnailInfo = ConstructObject<USceneThumbnailInfo>(USceneThumbnailInfo::StaticClass(), StaticMesh);
		StaticMesh->ThumbnailInfo = ThumbnailInfo;
	}

	return ThumbnailInfo;
}

void FAssetTypeActions_StaticMesh::ExecuteEdit(TArray<TWeakObjectPtr<UStaticMesh>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}

void FAssetTypeActions_StaticMesh::ExecuteReimport(TArray<TWeakObjectPtr<UStaticMesh>> Objects)
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

void FAssetTypeActions_StaticMesh::GetImportLODMenu(class FMenuBuilder& MenuBuilder,TArray<TWeakObjectPtr<UStaticMesh>> Objects)
{
	check(Objects.Num() > 0);
	auto First = Objects[0];
	UStaticMesh* StaticMesh = First.Get();

	for(int32 LOD = 1;LOD<=First->GetNumLODs();++LOD)
	{
		FText LODText = FText::AsNumber( LOD );
		FText Description = FText::Format( NSLOCTEXT("AssetTypeActions_StaticMesh", "Reimport LOD (number)", "Reimport LOD {0}"), LODText );
		FText ToolTip = NSLOCTEXT("AssetTypeActions_StaticMesh", "ReimportTip", "Reimport over existing LOD");
		if(LOD == First->GetNumLODs())
		{
			Description = FText::Format( NSLOCTEXT("AssetTypeActions_StaticMesh", "Import LOD (number)", "Import LOD {0}"), LODText );
			ToolTip = NSLOCTEXT("AssetTypeActions_StaticMesh", "NewImportTip", "Import new LOD");
		}

		MenuBuilder.AddMenuEntry( Description, ToolTip, FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic( &FbxMeshUtils::ImportMeshLODDialog, Cast<UObject>(StaticMesh), LOD) )) ;
	}
}

void FAssetTypeActions_StaticMesh::ExecuteFindInExplorer(TArray<TWeakObjectPtr<UStaticMesh>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object && Object->AssetImportData )
		{
			const FString SourceFilePath = FReimportManager::ResolveImportFilename(Object->AssetImportData->SourceFilePath, Object);
			if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
			{
				FPlatformProcess::ExploreFolder( *FPaths::GetPath(SourceFilePath) );
			}
		}
	}
}

void FAssetTypeActions_StaticMesh::ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<UStaticMesh>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object && Object->AssetImportData )
		{
			const FString SourceFilePath = FReimportManager::ResolveImportFilename(Object->AssetImportData->SourceFilePath, Object);
			if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
			{
				FPlatformProcess::LaunchFileInDefaultExternalApplication( *SourceFilePath );
			}
		}
	}
}

void FAssetTypeActions_StaticMesh::ExecuteCreateDestructibleMesh(TArray<TWeakObjectPtr<UStaticMesh>> Objects)
{
	TArray< UObject* > Assets;
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FText ErrorMsg;
			FDestructibleMeshEditorModule& DestructibleMeshEditorModule = FModuleManager::LoadModuleChecked<FDestructibleMeshEditorModule>( "DestructibleMeshEditor" );
			UDestructibleMesh* DestructibleMesh = DestructibleMeshEditorModule.CreateDestructibleMeshFromStaticMesh(Object->GetOuter(), Object, NAME_None, Object->GetFlags(), ErrorMsg);
			if ( DestructibleMesh )
			{
				FAssetEditorManager::Get().OpenEditorForAsset(DestructibleMesh);
				Assets.Add(DestructibleMesh);
			}
			else if ( !ErrorMsg.IsEmpty() )
			{
				FNotificationInfo ErrorNotification( ErrorMsg );
				FSlateNotificationManager::Get().AddNotification(ErrorNotification);
			}
		}
	}
	if ( Assets.Num() > 0 )
	{
		FAssetTools::Get().SyncBrowserToAssets(Assets);
	}
}

bool FAssetTypeActions_StaticMesh::CanExecuteSourceCommands(TArray<TWeakObjectPtr<UStaticMesh>> Objects) const
{
	bool bHaveSourceAsset = false;
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object && Object->AssetImportData )
		{
			const FString& SourceFilePath = FReimportManager::ResolveImportFilename(Object->AssetImportData->SourceFilePath, Object);

			if ( SourceFilePath.Len() && IFileManager::Get().FileSize(*SourceFilePath) != INDEX_NONE )
			{
				return true;
			}
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
