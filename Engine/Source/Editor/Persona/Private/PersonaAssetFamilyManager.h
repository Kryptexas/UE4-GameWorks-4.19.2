// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Central registry of persona asset families */
class FPersonaAssetFamilyManager
{
public:
	/** Singleton access */
	static FPersonaAssetFamilyManager& Get();

	/** Create a skeleton tree for the requested skeleton */
	TSharedRef<class IAssetFamily> CreatePersonaAssetFamily(const UObject* InAsset);

private:
	/** Hidden constructor */
	FPersonaAssetFamilyManager() {}

private:
	/** All current asset families */
	TArray<TWeakPtr<class IAssetFamily>> AssetFamilies;
};
