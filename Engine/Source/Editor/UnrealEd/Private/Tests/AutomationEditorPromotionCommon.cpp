// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AutomationEditorPromotionCommon.h"
#include "AssetRegistryModule.h"

/**
* Gets the base path for this asset
*/
FString FEditorPromotionTestUtilities::GetGamePath()
{
	return TEXT("/Game/BuildPromotionTest");
}

/**
* Creates a material from an existing texture
*
* @param InTexture - The texture to use as the diffuse for the new material
*/
UMaterial* FEditorPromotionTestUtilities::CreateMaterialFromTexture(UTexture* InTexture)
{
	// Create the factory used to generate the asset
	UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
	Factory->InitialTexture = InTexture;


	const FString AssetName = FString::Printf(TEXT("%s_Mat"), *InTexture->GetName());
	const FString PackageName = FEditorPromotionTestUtilities::GetGamePath() + TEXT("/") + AssetName;
	UPackage* AssetPackage = CreatePackage(NULL, *PackageName);
	EObjectFlags Flags = RF_Public | RF_Standalone;

	UObject* CreatedAsset = Factory->FactoryCreateNew(UMaterial::StaticClass(), AssetPackage, FName(*AssetName), Flags, NULL, GWarn);

	if (CreatedAsset)
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(CreatedAsset);

		// Mark the package dirty...
		AssetPackage->MarkPackageDirty();
	}

	return Cast<UMaterial>(CreatedAsset);
}