// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Toolkits/AssetEditorManager.h"
#include "Editor/PhAT/Public/PhATModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_PhysicsAsset::GetSupportedClass() const
{
	return UPhysicsAsset::StaticClass();
}

void FAssetTypeActions_PhysicsAsset::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto PhysicsAssets = GetTypedWeakObjectPtrs<UPhysicsAsset>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("PhysicsAsset_Edit", "Edit"),
		LOCTEXT("PhysicsAsset_EditTooltip", "Opens the selected physics asset in PhAT."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_PhysicsAsset::ExecuteEdit, PhysicsAssets ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_PhysicsAsset::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto PhysicsAsset = Cast<UPhysicsAsset>(*ObjIt);
		if (PhysicsAsset != NULL)
		{
			
			IPhATModule* PhATModule = &FModuleManager::LoadModuleChecked<IPhATModule>( "PhAT" );
			PhATModule->CreatePhAT(Mode, EditWithinLevelEditor, PhysicsAsset);
		}
	}
}

void FAssetTypeActions_PhysicsAsset::ExecuteEdit(TArray<TWeakObjectPtr<UPhysicsAsset>> Objects)
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