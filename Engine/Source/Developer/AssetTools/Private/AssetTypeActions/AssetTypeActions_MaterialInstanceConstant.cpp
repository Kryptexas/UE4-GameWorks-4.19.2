// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Editor/MaterialEditor/Public/MaterialEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_MaterialInstanceConstant::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto MICs = GetTypedWeakObjectPtrs<UMaterialInstanceConstant>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MaterialInstanceConstant_Edit", "Edit"),
		LOCTEXT("MaterialInstanceConstant_EditTooltip", "Opens the selected materials in the material editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_MaterialInstanceConstant::ExecuteEdit, MICs ),
			FCanExecuteAction()
			)
		);

	FAssetTypeActions_MaterialInterface::GetActions(InObjects, MenuBuilder);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MaterialInstanceConstant_FindParent", "Find Parent"),
		LOCTEXT("MaterialInstanceConstant_FindParentTooltip", "Finds the material this instance is based on in the content browser."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_MaterialInstanceConstant::ExecuteFindParent, MICs ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_MaterialInstanceConstant::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto MIC = Cast<UMaterialInstanceConstant>(*ObjIt);
		if (MIC != NULL)
		{
			IMaterialEditorModule* MaterialEditorModule = &FModuleManager::LoadModuleChecked<IMaterialEditorModule>( "MaterialEditor" );
			MaterialEditorModule->CreateMaterialInstanceEditor(Mode, EditWithinLevelEditor, MIC);
		}
	}
}

void FAssetTypeActions_MaterialInstanceConstant::ExecuteEdit(TArray<TWeakObjectPtr<UMaterialInstanceConstant>> Objects)
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

void FAssetTypeActions_MaterialInstanceConstant::ExecuteFindParent(TArray<TWeakObjectPtr<UMaterialInstanceConstant>> Objects)
{
	TArray<UObject*> ObjectsToSyncTo;

	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			ObjectsToSyncTo.AddUnique( Object->Parent );
		}
	}

	// Sync the respective browser to the valid parents
	if ( ObjectsToSyncTo.Num() > 0 )
	{
		FAssetTools::Get().SyncBrowserToAssets(ObjectsToSyncTo);
	}
}

#undef LOCTEXT_NAMESPACE