// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Engine/PrimaryAssetLabel.h"
#include "Engine/DataAsset.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "Engine/AssetManager.h"
#include "AssetRegistryModule.h"

const FName UPrimaryAssetLabel::DirectoryBundle = FName("Directory");

UPrimaryAssetLabel::UPrimaryAssetLabel()
{
	bLabelAssetsInMyDirectory = false;

	// By default have low priority and don't recurse
	Rules.bApplyRecursively = false;
	Rules.Priority = 0;
}

#if WITH_EDITORONLY_DATA
void UPrimaryAssetLabel::UpdateAssetBundleData()
{
	Super::UpdateAssetBundleData();

	if (!UAssetManager::IsValid())
	{
		return;
	}

	UAssetManager& Manager = UAssetManager::Get();
	IAssetRegistry& AssetRegistry = Manager.GetAssetRegistry();

	if (bLabelAssetsInMyDirectory)
	{
		FName PackagePath = FName(*FPackageName::GetLongPackagePath(GetOutermost()->GetName()));

		TArray<FAssetData> DirectoryAssets;
		AssetRegistry.GetAssetsByPath(PackagePath, DirectoryAssets, true);

		TArray<FStringAssetReference> NewPaths;

		for (const FAssetData& AssetData : DirectoryAssets)
		{
			FStringAssetReference AssetRef = Manager.GetAssetPathForData(AssetData);

			if (!AssetRef.IsNull())
			{
				NewPaths.Add(AssetRef);
			}
		}

		// Fast set, destroys NewPaths
		AssetBundleData.SetBundleAssets(DirectoryBundle, MoveTemp(NewPaths));
	}
	
	// Update rules
	FPrimaryAssetId PrimaryAssetId = GetPrimaryAssetId();
	Manager.SetPrimaryAssetRules(PrimaryAssetId, Rules);
}
#endif
