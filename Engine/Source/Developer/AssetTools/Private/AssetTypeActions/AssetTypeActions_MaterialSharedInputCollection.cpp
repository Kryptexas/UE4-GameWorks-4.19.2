// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions/AssetTypeActions_MaterialSharedInputCollection.h"
#include "MaterialEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_MaterialSharedInputCollection::GetSupportedClass() const
{
	IMaterialEditorModule& MaterialEditorModule = FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
	UClass* SupportedClass = MaterialEditorModule.MaterialLayersEnabled() ? UMaterialSharedInputCollection::StaticClass() : nullptr;
	return SupportedClass;
}

bool FAssetTypeActions_MaterialSharedInputCollection::CanFilter()
{
	IMaterialEditorModule& MaterialEditorModule = FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
	return MaterialEditorModule.MaterialLayersEnabled();
}

#undef LOCTEXT_NAMESPACE
