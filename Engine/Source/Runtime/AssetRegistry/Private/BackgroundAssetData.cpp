// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AssetRegistryPCH.h"

FBackgroundAssetData::FBackgroundAssetData(FString InPackageName, FString InPackagePath, FString InGroupNames, FString InAssetName, FString InAssetClass, TMap<FString, FString> InTags, TArray<int32> InChunkIDs, uint32 InPackageFlags)
	: PackageName(MoveTemp(InPackageName))
	, PackagePath(MoveTemp(InPackagePath))
	, GroupNames(MoveTemp(InGroupNames))
	, AssetName(MoveTemp(InAssetName))
	, AssetClass(MoveTemp(InAssetClass))
	, TagsAndValues(MakeSharedMapView(MoveTemp(InTags)))
	, ChunkIDs(MoveTemp(InChunkIDs))
	, PackageFlags(InPackageFlags)
{
	ObjectPath = PackageName + TEXT(".");

	if (GroupNames.Len())
	{
		ObjectPath += GroupNames + TEXT(".");
	}

	ObjectPath += AssetName;
}

FBackgroundAssetData::FBackgroundAssetData(FString InPackageName, uint32 InPackageFlags)
	: PackageName(InPackageName)
	, PackageFlags(InPackageFlags)
{
}


FAssetData FBackgroundAssetData::ToAssetData() const
{
	TMap<FName, FString> CopiedTagsAndValues;
	for (const auto& TagsAndValuesEntry : TagsAndValues)
	{
		CopiedTagsAndValues.Add(FName(*TagsAndValuesEntry.Key), TagsAndValuesEntry.Value);
	}

	return FAssetData(
		FName(*PackageName),
		FName(*PackagePath),
		FName(*GroupNames),
		FName(*AssetName),
		FName(*AssetClass),
		MoveTemp(CopiedTagsAndValues),
		ChunkIDs,
		PackageFlags
		);
}

bool FBackgroundAssetData::IsWithinSearchPath(const FString& InSearchPath) const
{
	return PackagePath.StartsWith(InSearchPath);
}

FAssetDataWrapper::FAssetDataWrapper(FAssetData InAssetData)
	: WrappedAssetData(MoveTemp(InAssetData))
	, PackagePathStr(WrappedAssetData.PackagePath.ToString())
{
}

FAssetData FAssetDataWrapper::ToAssetData() const
{
	return WrappedAssetData;
}

bool FAssetDataWrapper::IsWithinSearchPath(const FString& InSearchPath) const
{
	return PackagePathStr.StartsWith(InSearchPath);
}


FString FAssetDataWrapper::GetPackageName() const
{ 
	return WrappedAssetData.PackageName.ToString();  
}