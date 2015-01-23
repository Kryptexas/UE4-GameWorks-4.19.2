// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "ContentDirectoryMonitor.h"

#include "AssetRegistryModule.h"
#include "PackageTools.h"
#include "ObjectTools.h"

/** Generate a config from the specified options, to pass to FFileCache on construction */
FFileCacheConfig GenerateFileCacheConfig(const FString& InPath, const FString& InSupportedExtensions, const FString& InMountedContentPath)
{
	FString Directory = FPaths::ConvertRelativePathToFull(InPath);

	const FString& HashString = InMountedContentPath.IsEmpty() ? Directory : InMountedContentPath;
	const uint32 CRC = FCrc::MemCrc32(*HashString, HashString.Len()*sizeof(TCHAR));	
	FString CacheFilename = FPaths::ConvertRelativePathToFull(FPaths::GameIntermediateDir()) / TEXT("ReimportCache") / FString::Printf(TEXT("%u.bin"), CRC);

	FFileCacheConfig Config(MoveTemp(Directory), MoveTemp(CacheFilename));
	Config.IncludeExtensions = InSupportedExtensions;
	// We always store paths inside content folders relative to the folder
	Config.PathType = EPathType::Relative;

	return Config;
}

FContentDirectoryMonitor::FContentDirectoryMonitor(const FString& InDirectory, const FString& InSupportedExtensions, const FString& InMountedContentPath)
	: Cache(GenerateFileCacheConfig(InDirectory, InSupportedExtensions, InMountedContentPath))
	, MountedContentPath(InMountedContentPath)
	, LastSaveTime(0)
{
	
}

void FContentDirectoryMonitor::Destroy()
{
	Cache.Destroy();
}

void FContentDirectoryMonitor::Tick(const FTimeLimit& TimeLimit)
{
	Cache.Tick(TimeLimit);

	const double Now = FPlatformTime::Seconds();
	if (Now - LastSaveTime > ResaveIntervalS)
	{
		LastSaveTime = Now;
		Cache.WriteCache();
	}
}

void FContentDirectoryMonitor::StartProcessing()
{
	WorkProgress = TotalWork = 0;

	auto OutstandingChanges = Cache.GetOutstandingChanges();
	if (OutstandingChanges.Num() != 0)
	{
		for (auto& Transaction : OutstandingChanges)
		{
			switch(Transaction.Action)
			{
				case FFileChangeData::FCA_Added:		AddedFiles.Emplace(MoveTemp(Transaction));		break;
				case FFileChangeData::FCA_Modified:		ModifiedFiles.Emplace(MoveTemp(Transaction));	break;
				case FFileChangeData::FCA_Removed:		DeletedFiles.Emplace(MoveTemp(Transaction));	break;
			}
		}

		TotalWork = AddedFiles.Num() + ModifiedFiles.Num() + DeletedFiles.Num();
	}
}

void FContentDirectoryMonitor::ProcessAdditions(TArray<UPackage*>& OutPackagesToSave, const FTimeLimit& TimeLimit, const TMap<FString, TArray<UFactory*>>& InFactoriesByExtension)
{
	if (MountedContentPath.IsEmpty())
	{
		// @todo: arodham: Prompt user for import location
		return;
	}
	
	for (int32 Index = 0; Index < AddedFiles.Num(); ++Index)
	{
		auto& Addition = AddedFiles[Index];

		WorkProgress += 1;
		const FString FullFilename = Cache.GetDirectory() + Addition.Filename.Get();

		bool bImportSucceeded = false;
		bool bImportWasCancelled = false;

		FString NewAssetName = ObjectTools::SanitizeObjectName(FPaths::GetBaseFilename(FullFilename));
		FString PackagePath= PackageTools::SanitizePackageName(MountedContentPath / FPaths::GetPath(Addition.Filename.Get()) / NewAssetName);

		// We can not create assets that share the name of a map file in the same location
		if (FEditorFileUtils::IsMapPackageAsset(PackagePath))
		{
			// @todo: Error reporting
			continue;
		}

		if (FindPackage(nullptr, *PackagePath))
		{
			// @todo: Error reporting
			continue;	
		}

		UPackage* NewPackage = CreatePackage(nullptr, *PackagePath);
		if ( !ensure(NewPackage) )
		{
			// @todo: Error reporting
			continue;
		}

		// Make sure the destination package is loaded
		NewPackage->FullyLoad();

		bool bSuccess = false;
		bool bCancelled = false;

		UObject* NewAsset = nullptr;

		const FString Ext = FPaths::GetExtension(Addition.Filename.Get(), false);
		if (auto* Factories = InFactoriesByExtension.Find(Ext))
		{
			for (auto* Factory : *Factories)
			{
				NewAsset = UFactory::StaticImportObject(Factory->ResolveSupportedClass(), NewPackage, FName(*NewAssetName), RF_Public | RF_Standalone, bCancelled, *FullFilename, nullptr, Factory);
				if (NewAsset || bCancelled)
				{
					break;
				}
			}
		}
		
		if (!NewAsset || bCancelled)
		{
			// @todo: Error reporting
			continue;
		}

		FAssetRegistryModule::AssetCreated(NewAsset);
		GEditor->BroadcastObjectReimported(NewAsset);

		OutPackagesToSave.Add(NewPackage);

		// Refresh the supported class.  Some factories (e.g. FBX) only resolve their type after reading the file
		// ImportAssetType = Factory->ResolveSupportedClass();
		// @todo: analytics

		// Let the cache know that we've dealt with this change (it will be imported immediately)
		Cache.CompleteTransaction(MoveTemp(Addition));

		if (TimeLimit.Exceeded())
		{
			AddedFiles.RemoveAt(0, Index + 1);
			return;
		}
	}

	AddedFiles.Empty();
}

void FContentDirectoryMonitor::ProcessModifications(const IAssetRegistry& Registry, const FTimeLimit& TimeLimit)
{
	auto* ReimportManager = FReimportManager::Instance();

	for (int32 Index = 0; Index < ModifiedFiles.Num(); ++Index)
	{
		auto& Change = ModifiedFiles[Index];

		const FString FullFilename = Cache.GetDirectory() + Change.Filename.Get();
		for (const auto& AssetData : Utils::FindAssetsPertainingToFile(Registry, FullFilename))
		{
			UObject* Asset = AssetData.GetAsset();
			ReimportManager->Reimport(Asset, false /* Ask for new file */, false /* Show notification */);
		}

		// Let the cache know that we've dealt with this change
		Cache.CompleteTransaction(MoveTemp(Change));
		
		WorkProgress += 1;
		if (TimeLimit.Exceeded())
		{
			ModifiedFiles.RemoveAt(0, Index + 1);
			return;
		}
	}

	ModifiedFiles.Empty();
}

void FContentDirectoryMonitor::ExtractAssetsToDelete(const IAssetRegistry& Registry, TArray<FAssetData>& OutAssetsToDelete)
{
	for (auto& Deletion : DeletedFiles)
	{
		for (const auto& AssetData : Utils::FindAssetsPertainingToFile(Registry, Cache.GetDirectory() + Deletion.Filename.Get()))
		{
			OutAssetsToDelete.Add(AssetData);
		}

		// Let the cache know that we've dealt with this change (it will be imported in due course)
		Cache.CompleteTransaction(MoveTemp(Deletion));
	}

	WorkProgress += DeletedFiles.Num();
	DeletedFiles.Empty();
}