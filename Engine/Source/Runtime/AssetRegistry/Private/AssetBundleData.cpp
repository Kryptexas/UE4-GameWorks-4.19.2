// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AssetBundleData.h"
#include "AssetData.h"
#include "UObject/PropertyPortFlags.h"

bool FAssetBundleData::SetFromAssetData(const FAssetData& AssetData)
{
	FString TagValue;

	if (AssetData.GetTagValue(FAssetBundleData::StaticStruct()->GetFName(), TagValue))
	{
		if (FAssetBundleData::StaticStruct()->ImportText(*TagValue, this, nullptr, PPF_None, (FOutputDevice*)GWarn, AssetData.AssetName.ToString()))
		{
			FPrimaryAssetId FoundId = AssetData.GetPrimaryAssetId();

			if (FoundId.IsValid())
			{
				// Update the primary asset id if valid
				for (FAssetBundleEntry& Bundle : Bundles)
				{
					Bundle.BundleScope = FoundId;
				}
			}

			return true;
		}
	}
	return false;
}

FAssetBundleEntry* FAssetBundleData::FindEntry(const FPrimaryAssetId& SearchScope, FName SearchName)
{
	for (FAssetBundleEntry& Entry : Bundles)
	{
		if (Entry.BundleScope == SearchScope && Entry.BundleName == SearchName)
		{
			return &Entry;
		}
	}
	return nullptr;
}

void FAssetBundleData::AddBundleAsset(FName BundleName, const FStringAssetReference& AssetPath)
{
	if (!AssetPath.IsValid())
	{
		return;
	}

	FAssetBundleEntry* FoundEntry = FindEntry(FPrimaryAssetId(), BundleName);

	if (!FoundEntry)
	{
		FoundEntry = new(Bundles) FAssetBundleEntry(FPrimaryAssetId(), BundleName);
	}

	FoundEntry->BundleAssets.AddUnique(AssetPath);
}

void FAssetBundleData::AddBundleAssets(FName BundleName, const TArray<FStringAssetReference>& AssetPaths)
{
	FAssetBundleEntry* FoundEntry = FindEntry(FPrimaryAssetId(), BundleName);

	for (const FStringAssetReference& Path : AssetPaths)
	{
		if (Path.IsValid())
		{
			// Only create if required
			if (!FoundEntry)
			{
				FoundEntry = new(Bundles) FAssetBundleEntry(FPrimaryAssetId(), BundleName);
			}

			FoundEntry->BundleAssets.AddUnique(Path);
		}
	}
}

void FAssetBundleData::Reset()
{
	Bundles.Reset();
}