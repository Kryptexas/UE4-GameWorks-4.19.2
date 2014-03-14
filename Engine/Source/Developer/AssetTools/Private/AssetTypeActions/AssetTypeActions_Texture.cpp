// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "Editor/UnrealEd/Public/PackageHelperFunctions.h"
#include "TextureEditor.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_Texture::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Textures = GetTypedWeakObjectPtrs<UTexture>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Texture_Reimport", "Reimport"),
		LOCTEXT("Texture_ReimportTooltip", "Reimports the selected textures from file."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Texture::ExecuteReimport, Textures ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Texture_Edit", "Edit"),
		LOCTEXT("Texture_EditTooltip", "Opens the selected textures in the texture viewer."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Texture::ExecuteEdit, Textures ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Texture_CreateMaterial", "Create Material"),
		LOCTEXT("Texture_CreateMaterialTooltip", "Creates a new material using this texture."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Texture::ExecuteCreateMaterial, Textures ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Texture_FindInExplorer", "Find Source"),
		LOCTEXT("Texture_FindInExplorerTooltip", "Opens explorer at the location of this asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Texture::ExecuteFindInExplorer, Textures ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_Texture::CanExecuteSourceCommands, Textures )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Texture_OpenInExternalEditor", "Open Source"),
		LOCTEXT("Texture_OpenInExternalEditorTooltip", "Opens the selected asset in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Texture::ExecuteOpenInExternalEditor, Textures ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_Texture::CanExecuteSourceCommands, Textures )
			)
		);

	// @todo AssetTypeActions Implement FindMaterials using the asset registry.
	/*
	if ( InObjects.Num() == 1 )
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Texture_FindMaterials", "Find Materials Using This"),
			LOCTEXT("Texture_FindMaterialsTooltip", "Finds all materials that use this material in the content browser."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetTypeActions_Texture::ExecuteFindMaterials, Textures(0) ),
				FCanExecuteAction()
				)
			);
	}
	*/
}

void FAssetTypeActions_Texture::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Texture = Cast<UTexture>(*ObjIt);
		if (Texture != NULL)
		{
			ITextureEditorModule* TextureEditorModule = &FModuleManager::LoadModuleChecked<ITextureEditorModule>("TextureEditor");
			TextureEditorModule->CreateTextureEditor(Mode, EditWithinLevelEditor, Texture);
		}
	}
}

void FAssetTypeActions_Texture::ExecuteReimport(TArray<TWeakObjectPtr<UTexture>> Objects)
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

void FAssetTypeActions_Texture::ExecuteEdit(TArray<TWeakObjectPtr<UTexture>> Objects)
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

void FAssetTypeActions_Texture::ExecuteCreateMaterial(TArray<TWeakObjectPtr<UTexture>> Objects)
{
	const FString DefaultSuffix = TEXT("_Mat");

	if ( Objects.Num() == 1 )
	{
		auto Object = Objects[0].Get();

		if ( Object )
		{
			// Determine an appropriate name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			UMaterialFactoryNew* Factory = ConstructObject<UMaterialFactoryNew>(UMaterialFactoryNew::StaticClass());
			Factory->InitialTexture = Object;

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UMaterial::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;

		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();
			if ( Object )
			{
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				UMaterialFactoryNew* Factory = ConstructObject<UMaterialFactoryNew>(UMaterialFactoryNew::StaticClass());
				Factory->InitialTexture = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UMaterial::StaticClass(), Factory);

				if ( NewAsset )
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if ( ObjectsToSync.Num() > 0 )
		{
			FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
}

void FAssetTypeActions_Texture::ExecuteFindInExplorer(TArray<TWeakObjectPtr<UTexture>> Objects)
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

void FAssetTypeActions_Texture::ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<UTexture>> Objects)
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

void FAssetTypeActions_Texture::ExecuteFindMaterials(TWeakObjectPtr<UTexture> Object)
{
	auto Texture = Object.Get();
	if ( Texture )
	{
		// @todo AssetTypeActions Implement FindMaterials using the asset registry.
	}
}

bool FAssetTypeActions_Texture::CanExecuteSourceCommands(TArray<TWeakObjectPtr<UTexture>> Objects) const
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