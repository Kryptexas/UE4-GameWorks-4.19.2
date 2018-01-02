// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions/AssetTypeActions_MaterialFunctionInstance.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "EditorStyleSet.h"
#include "ThumbnailRendering/SceneThumbnailInfoWithPrimitive.h"
#include "AssetTools.h"
#include "MaterialEditorModule.h"

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



#undef LOCTEXT_NAMESPACE
