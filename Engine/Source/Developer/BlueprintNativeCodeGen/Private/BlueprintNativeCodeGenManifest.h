// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**  */
struct FBlueprintNativeCodeGenManifest
{
public: 
	struct FAssetEntry
	{
		UClass* AssetType;
		FString AssetPath;
		FString GeneratedFilePath;
	};

private:
	TArray<FAssetEntry> Manifest;
};

