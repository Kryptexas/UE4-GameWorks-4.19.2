// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetRegistryPCH.h"

FBackgroundAssetData::FBackgroundAssetData(const FString& InPackageName, const FString& InPackagePath, const FString& InGroupNames, const FString& InAssetName, const FString& InAssetClass, const TMap<FString, FString>& InTags, const TArray<int32>& InChunkIDs)
{
	PackageName = InPackageName;
	PackagePath = InPackagePath;
	GroupNames = InGroupNames;
	AssetName = InAssetName;
	AssetClass = InAssetClass;
	TagsAndValues = InTags;

	ObjectPath = PackageName + TEXT(".");

	if ( GroupNames.Len() )
	{
		ObjectPath += GroupNames + TEXT(".");
	}

	ObjectPath += AssetName;

	ChunkIDs = InChunkIDs;
}

FAssetData FBackgroundAssetData::ToAssetData() const
{
	TMap<FName, FString> CopiedTagsAndValues;
	for (TMap<FString,FString>::TConstIterator TagIt(TagsAndValues); TagIt; ++TagIt)
	{
		CopiedTagsAndValues.Add(FName(*TagIt.Key()), TagIt.Value());
	}

	return FAssetData(
		FName(*PackageName),
		FName(*PackagePath),
		FName(*GroupNames),
		FName(*AssetName),
		FName(*AssetClass),
		CopiedTagsAndValues,
		ChunkIDs
		);
}
