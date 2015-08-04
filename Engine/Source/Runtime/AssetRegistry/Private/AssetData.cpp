// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AssetRegistryPCH.h"

DEFINE_LOG_CATEGORY(LogAssetData);

TMap<FName, TSet<FName>> FAssetData::CookWhitelistedTagsByClass;

void FAssetData::WhitelistCookedTag(FName InClassName, FName InTagName)
{
	TSet<FName>& EditorOnlyTags = CookWhitelistedTagsByClass.FindOrAdd(InClassName);
	EditorOnlyTags.Add(InTagName);
}

const TSet<FName>* FAssetData::GetCookWhitelistedTags(FName InClassName)
{
	return CookWhitelistedTagsByClass.Find(InClassName);
}