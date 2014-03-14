// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#include "Editor/DestructibleMeshEditor/Public/DestructibleMeshEditorModule.h"
#include "Editor/DestructibleMeshEditor/Public/IDestructibleMeshEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_DestructibleMesh::GetSupportedClass() const
{
	return UDestructibleMesh::StaticClass();
}

void FAssetTypeActions_DestructibleMesh::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Meshes = GetTypedWeakObjectPtrs<UDestructibleMesh>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ObjectContext_EditUsingDestructibleMeshEditor", "Edit"),
		LOCTEXT("ObjectContext_EditUsingDestructibleMeshEditorTooltip", "Opens the selected meshes in the Destructible Mesh editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_DestructibleMesh::ExecuteEdit, Meshes ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ObjectContext_ReimportDestructibleMesh", "Reimport"),
		LOCTEXT("ObjectContext_ReimportDestructibleMeshTooltip", "Reimports the selected meshes from file."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_DestructibleMesh::ExecuteReimport, Meshes ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_DestructibleMesh::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Mesh = Cast<UDestructibleMesh>(*ObjIt);
		if (Mesh != NULL)
		{
			FDestructibleMeshEditorModule& DestructibleMeshEditorModule = FModuleManager::LoadModuleChecked<FDestructibleMeshEditorModule>( "DestructibleMeshEditor" );
			TSharedRef< IDestructibleMeshEditor > NewDestructibleMeshEditor = DestructibleMeshEditorModule.CreateDestructibleMeshEditor( EToolkitMode::Standalone, EditWithinLevelEditor, Mesh );
		}
	}
}

void FAssetTypeActions_DestructibleMesh::ExecuteEdit(TArray<TWeakObjectPtr<UDestructibleMesh>> Objects)
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

void FAssetTypeActions_DestructibleMesh::ExecuteReimport(TArray<TWeakObjectPtr<UDestructibleMesh>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FReimportManager::Instance()->Reimport(Object, true);
		}
	}
}

#undef LOCTEXT_NAMESPACE
