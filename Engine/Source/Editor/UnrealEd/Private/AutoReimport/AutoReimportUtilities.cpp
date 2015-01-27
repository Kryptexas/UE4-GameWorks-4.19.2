// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "AutoReimportUtilities.h"
#include "AssetRegistryModule.h"

namespace Utils
{
	/** Find a list of assets that were last imported from the specified filename. */
	TArray<FAssetData> FindAssetsPertainingToFile(const IAssetRegistry& Registry, const FString& AbsoluteFilename)
	{
		TArray<FAssetData> Assets, Result;
		const FString LeafName = FPaths::GetCleanFilename(AbsoluteFilename);
		const FName TagName = UObject::SourceFileTagName();

		FARFilter Filter;
		Filter.SourceFilenames.Add(*LeafName);
		Filter.bIncludeOnlyOnDiskAssets = true;
		Registry.GetAssets(Filter, Assets);

		for (const auto& Asset : Assets)
		{
			for (const auto& Pair : Asset.TagsAndValues)
			{
				// We don't compare numbers on FNames for this check because the tag "ReimportPath" may exist multiple times
				const bool bCompareNumber = false;
				if (Pair.Key.IsEqual(TagName, ENameCase::IgnoreCase, bCompareNumber))
				{
					// Either relative to the asset itself, or relative to the base path, or absolute.
					// Would ideally use FReimportManager::ResolveImportFilename here, but we don't want to ask the file system whether the file exists.
					if (AbsoluteFilename == FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(Asset.PackagePath.ToString()) / Pair.Value) ||
						AbsoluteFilename == FPaths::ConvertRelativePathToFull(Pair.Value))
					{
						Result.Add(Asset);
						break;
					}
				}
			}
		}

		return Result;
	}
}