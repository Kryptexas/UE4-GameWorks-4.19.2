// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PackageTools.h"
#include "PackageName.h"
#include "ObjectTools.h"
#include "AssetToolsModule.h"

template <typename T>
UPackage* GetAssetPackageAndName(UObject* Parent, const FString& PreferredName, const TCHAR* BackupPrefix, FName BackupName, int32 BackupIndex, FString& OutName)
{
	FString AssetName;

	if (PreferredName.IsEmpty())
	{
		// Create asset name based on Prefix_Name_Index, e.g. "SM_Suzanne_0"
		AssetName = FString::Printf(TEXT("%s_%s_%d"), BackupPrefix, *BackupName.ToString(), BackupIndex);
	}
	else
	{
		AssetName = PreferredName;
	}

	AssetName = ObjectTools::SanitizeObjectName(AssetName);

	// set where to place the static mesh
	const FString BasePackageName = PackageTools::SanitizePackageName(FPackageName::GetLongPackagePath(Parent->GetOutermost()->GetName()) / AssetName);

	const FString ObjectPath = BasePackageName + TEXT('.') + AssetName;
	T* ExistingAsset = LoadObject<T>(nullptr, *ObjectPath, nullptr, LOAD_Quiet | LOAD_NoWarn);

	UPackage* AssetPackage;

	if (ExistingAsset)
	{
		AssetPackage = ExistingAsset->GetOutermost();
	}
	else
	{
		const FString Suffix(0, nullptr); // empty

		static FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString FinalPackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, Suffix, FinalPackageName, AssetName);

		AssetPackage = CreatePackage(nullptr, *FinalPackageName);
	}

	OutName = AssetName;
	return AssetPackage;
}
