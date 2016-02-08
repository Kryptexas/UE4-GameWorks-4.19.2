// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace SequenceRecorderUtils
{

/**
 * Utility function that creates an asset with the specified asset path and name.
 * If the asset cannot be created (as one already exists), we try to postfix the asset
 * name until we can successfully create the asset.
 */
template<typename AssetType>
static AssetType* MakeNewAsset(const FString& BaseAssetPath, const FString& BaseAssetName)
{
	FString AssetPath = BaseAssetPath;
	FString AssetName = BaseAssetName;

	AssetPath /= AssetName;

	UObject* Package = CreatePackage(nullptr, *AssetPath);
	UObject* Object = LoadObject<UObject>(Package, *AssetName, nullptr, LOAD_None, nullptr);

	// if object with same name exists, try a different name until we don't find one
	int32 ExtensionIndex = 0;
	while(Object)
	{
		AssetName = FString::Printf(TEXT("%s_%d"), *BaseAssetName, ExtensionIndex);
		AssetPath = BaseAssetPath / AssetName;
		Package = CreatePackage(nullptr, *AssetPath);
		Object = LoadObject<UObject>(Package, *AssetName, nullptr, LOAD_None, nullptr);

		ExtensionIndex++;
	}

	// Create the new asset in the package we just made
	return NewObject<AssetType>(Package, *AssetName, RF_Public | RF_Standalone);
}

}