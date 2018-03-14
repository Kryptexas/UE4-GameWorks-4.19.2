// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions/AssetTypeActions_MaterialFunctionInstance.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "EditorStyleSet.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"
#include "AssetTools.h"
#include "MaterialEditorModule.h"
#include "Factories/MaterialFunctionInstanceFactory.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_MaterialFunctionInstance::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto MFIs = GetTypedWeakObjectPtrs<UMaterialFunctionInstance>(InObjects);

	FAssetTypeActions_MaterialFunction::GetActions(InObjects, MenuBuilder);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MaterialInstanceConstant_FindParent", "Find Parent"),
		LOCTEXT("MaterialInstanceConstant_FindParentTooltip", "Finds the function this instance is based on in the content browser."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions.GenericFind"),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_MaterialFunctionInstance::ExecuteFindParent, MFIs ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_MaterialFunctionInstance::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (auto MFI = Cast<UMaterialFunctionInstance>(*ObjIt))
		{
			IMaterialEditorModule* MaterialEditorModule = &FModuleManager::LoadModuleChecked<IMaterialEditorModule>( "MaterialEditor" );
			MaterialEditorModule->CreateMaterialInstanceEditor(Mode, EditWithinLevelEditor, MFI);
		}
	}
}

UThumbnailInfo* FAssetTypeActions_MaterialFunctionInstance::GetThumbnailInfo(UObject* Asset) const
{
	UMaterialFunctionInstance* MaterialFunc = CastChecked<UMaterialFunctionInstance>(Asset);
	UThumbnailInfo* ThumbnailInfo = MaterialFunc->ThumbnailInfo;
	if (ThumbnailInfo == NULL)
	{
		ThumbnailInfo = NewObject<USceneThumbnailInfoWithPrimitive>(MaterialFunc, NAME_None, RF_Transactional);
		MaterialFunc->ThumbnailInfo = ThumbnailInfo;
	}

	return ThumbnailInfo;
}

void FAssetTypeActions_MaterialFunctionInstance::ExecuteFindParent(TArray<TWeakObjectPtr<UMaterialFunctionInstance>> Objects)
{
	TArray<UObject*> ObjectsToSyncTo;

	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (auto Object = (*ObjIt).Get())
		{
			ObjectsToSyncTo.AddUnique(Object->Parent);
		}
	}

	// Sync the respective browser to the valid parents
	if (ObjectsToSyncTo.Num() > 0)
	{
		FAssetTools::Get().SyncBrowserToAssets(ObjectsToSyncTo);
	}
}

UClass* FAssetTypeActions_MaterialFunctionLayerInstance::GetSupportedClass() const
{
	IMaterialEditorModule& MaterialEditorModule = FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
	UClass* SupportedClass = MaterialEditorModule.MaterialLayersEnabled() ? UMaterialFunctionMaterialLayerInstance::StaticClass() : nullptr;
	return SupportedClass;
}

bool FAssetTypeActions_MaterialFunctionLayerInstance::CanFilter()
{
	IMaterialEditorModule& MaterialEditorModule = FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
	return MaterialEditorModule.MaterialLayersEnabled();
}

void FAssetTypeActions_MaterialFunctionLayerInstance::ExecuteNewMFI(TArray<TWeakObjectPtr<UMaterialFunctionInterface>> Objects)
{
	const FString DefaultSuffix = TEXT("_Inst");

	if (Objects.Num() == 1)
	{
		auto Object = Objects[0].Get();

		if (Object)
		{
			// Create an appropriate and unique name 
			FString Name;
			FString PackageName;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

			UMaterialFunctionMaterialLayerInstanceFactory* Factory = NewObject<UMaterialFunctionMaterialLayerInstanceFactory>();
			Factory->InitialParent = Object;

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionMaterialLayerInstance::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;
		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();
			if (Object)
			{
				// Determine an appropriate name
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				UMaterialFunctionMaterialLayerInstanceFactory* Factory = NewObject<UMaterialFunctionMaterialLayerInstanceFactory>();
				Factory->InitialParent = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionMaterialLayerInstance::StaticClass(), Factory);

				if (NewAsset)
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if (ObjectsToSync.Num() > 0)
		{
			FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
}

UClass* FAssetTypeActions_MaterialFunctionLayerBlendInstance::GetSupportedClass() const
{
	IMaterialEditorModule& MaterialEditorModule = FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
	UClass* SupportedClass = MaterialEditorModule.MaterialLayersEnabled() ? UMaterialFunctionMaterialLayerBlendInstance::StaticClass() : nullptr;
	return SupportedClass;
}

bool FAssetTypeActions_MaterialFunctionLayerBlendInstance::CanFilter()
{
	IMaterialEditorModule& MaterialEditorModule = FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
	return MaterialEditorModule.MaterialLayersEnabled();
}


void FAssetTypeActions_MaterialFunctionLayerBlendInstance::ExecuteNewMFI(TArray<TWeakObjectPtr<UMaterialFunctionInterface>> Objects)
{
	const FString DefaultSuffix = TEXT("_Inst");

	if (Objects.Num() == 1)
	{
		auto Object = Objects[0].Get();

		if (Object)
		{
			// Create an appropriate and unique name 
			FString Name;
			FString PackageName;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

			UMaterialFunctionMaterialLayerBlendInstanceFactory* Factory = NewObject<UMaterialFunctionMaterialLayerBlendInstanceFactory>();
			Factory->InitialParent = Object;

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionMaterialLayerBlendInstance::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;
		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();
			if (Object)
			{
				// Determine an appropriate name
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				UMaterialFunctionMaterialLayerBlendInstanceFactory* Factory = NewObject<UMaterialFunctionMaterialLayerBlendInstanceFactory>();
				Factory->InitialParent = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialFunctionMaterialLayerBlendInstance::StaticClass(), Factory);

				if (NewAsset)
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if (ObjectsToSync.Num() > 0)
		{
			FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
}

#undef LOCTEXT_NAMESPACE
