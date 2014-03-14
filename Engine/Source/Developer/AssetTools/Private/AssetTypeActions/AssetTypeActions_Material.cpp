// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Editor/MaterialEditor/Public/MaterialEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_Material::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Materials = GetTypedWeakObjectPtrs<UMaterial>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Material_Edit", "Edit"),
		LOCTEXT("Material_EditTooltip", "Opens the selected materials in the material editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Material::ExecuteEdit, Materials ),
			FCanExecuteAction()
			)
		);
	
	FAssetTypeActions_MaterialInterface::GetActions(InObjects, MenuBuilder);
}

void FAssetTypeActions_Material::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Material = Cast<UMaterial>(*ObjIt);
		if (Material != NULL)
		{
			IMaterialEditorModule* MaterialEditorModule = &FModuleManager::LoadModuleChecked<IMaterialEditorModule>( "MaterialEditor" );
			MaterialEditorModule->CreateMaterialEditor(Mode, EditWithinLevelEditor, Material);
		}
	}
}

void FAssetTypeActions_Material::ExecuteEdit(TArray<TWeakObjectPtr<UMaterial>> Objects)
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

#undef LOCTEXT_NAMESPACE