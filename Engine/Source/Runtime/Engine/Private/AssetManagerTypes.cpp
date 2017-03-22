// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Engine/AssetManagerTypes.h"
#include "Engine/AssetManager.h"

bool FPrimaryAssetTypeInfo::FillRuntimeData()
{
	AssetBaseClassLoaded = AssetBaseClass.LoadSynchronous();

	if (!ensureMsgf(AssetBaseClassLoaded, TEXT("Failed to load Primary Asset Type class %s!"), *AssetBaseClass.ToString()))
	{
		return false;
	}

	for (const FStringAssetReference& AssetRef : SpecificAssets)
	{
		AssetScanPaths.AddUnique(AssetRef.ToString());
	}

	for (const FDirectoryPath& PathRef : Directories)
	{
		AssetScanPaths.AddUnique(PathRef.Path);
	}

	return true;
}

bool FPrimaryAssetRules::IsDefault() const
{
	return *this == FPrimaryAssetRules();
}

void FPrimaryAssetRules::OverrideRules(const FPrimaryAssetRules& OverrideRules)
{
	static FPrimaryAssetRules DefaultRules;

	if (OverrideRules.Priority != DefaultRules.Priority)
	{
		Priority = OverrideRules.Priority;
	}

	if (OverrideRules.bApplyRecursively != DefaultRules.bApplyRecursively)
	{
		bApplyRecursively = OverrideRules.bApplyRecursively;
	}

	if (OverrideRules.ChunkId != DefaultRules.ChunkId)
	{
		ChunkId = OverrideRules.ChunkId;
	}

	if (OverrideRules.CookRule != DefaultRules.CookRule)
	{
		CookRule = OverrideRules.CookRule;
	}
}

void FPrimaryAssetRules::PropagateCookRules(const FPrimaryAssetRules& ParentRules)
{
	static FPrimaryAssetRules DefaultRules;

	if (ParentRules.ChunkId != DefaultRules.ChunkId && ChunkId == DefaultRules.ChunkId)
	{
		ChunkId = ParentRules.ChunkId;
	}

	if (ParentRules.CookRule != DefaultRules.CookRule && CookRule == DefaultRules.CookRule)
	{
		CookRule = ParentRules.CookRule;
	}
}