// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "PackageHelperFunctions.h"
#include "DerivedDataCacheInterface.h"
#include "ISourceControlModule.h"
#include "GlobalShader.h"
#include "TargetPlatform.h"
#include "IConsoleManager.h"
#include "Developer/PackageDependencyInfo/Public/PackageDependencyInfo.h"
#include "AssetRegistryModule.h"
#include "UnrealEdMessages.h"
#include "GameDelegates.h"
#include "ChunkManifestGenerator.h"

DEFINE_LOG_CATEGORY_STATIC(LogChunkManifestGenerator, Log, All);

FChunkManifestGenerator::FChunkManifestGenerator(const TArray<ITargetPlatform*>& InPlatforms)
	: AssetRegistry(FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get())
	, Platforms(InPlatforms)
	, DefaultEngineChunkID(INDEX_NONE)
	, DefaultGameChunkID(INDEX_NONE)
	, bGenerateChunks(false)
{
}

FChunkManifestGenerator::~FChunkManifestGenerator()
{
	for (auto Manifest : ChunkManifests)
	{
		delete Manifest;
	}
	ChunkManifests.Empty();
}

void FChunkManifestGenerator::OnAssetLoaded(UObject* Asset)
{
	if (Asset != NULL)
	{
		UPackage* AssetPackage = CastChecked<UPackage>(Asset->GetOutermost());
		if (!AssetsLoadedWithLastPackage.Contains(AssetPackage->GetFName()))
		{
			AssetsLoadedWithLastPackage.Add(AssetPackage->GetFName());
		}
	}
}

void FChunkManifestGenerator::AddPackageToManifest(const FString& PackageName, int32 ChunkId)
{
	if (ChunkId >= ChunkManifests.Num())
	{
		for (int32 Index = ChunkManifests.Num(); Index <= ChunkId; ++Index)
		{
			ChunkManifests.Add(new FChunkManifest());
		}
	}
	if (!ChunkManifests[ChunkId]->Contains(PackageName))
	{
		ChunkManifests[ChunkId]->Add(PackageName);
	}
}

void FChunkManifestGenerator::FindPackageInManifests(const FString& PackageName, TArray<int32>& OutChunkIDs) const
{
	for (int32 Index = 0; Index < ChunkManifests.Num(); ++Index)
	{
		if (ChunkManifests[Index]->Contains(PackageName))
		{
			OutChunkIDs.Add(Index);
		}
	}
}

bool FChunkManifestGenerator::CleanTempPackagingDirectory(const FString& Platform) const
{
	FString TmpPackagingDir = GetTempPackagingDirectoryForPlatform(Platform);
	if (IFileManager::Get().DirectoryExists(*TmpPackagingDir))
	{
		if (!IFileManager::Get().DeleteDirectory(*TmpPackagingDir, false, true))
		{
			UE_LOG(LogChunkManifestGenerator, Error, TEXT("Failed to delete directory: %s"), *TmpPackagingDir);
			return false;
		}
	}
	return true;
}

bool FChunkManifestGenerator::GenerateStreamingInstallManifest(const FString& Platform)
{
	FString GameNameLower = FString(GGameName).ToLower();

	// empty out the current paklist directory
	FString TmpPackagingDir = GetTempPackagingDirectoryForPlatform(Platform);
	if (!IFileManager::Get().MakeDirectory(*TmpPackagingDir, true))
	{
		UE_LOG(LogChunkManifestGenerator, Error, TEXT("Failed to create directory: %s"), *TmpPackagingDir);
		return false;
	}
	
	// open a file for writing the list of pak file lists that we've generated
	FString PakChunkListFilename = TmpPackagingDir / TEXT("pakchunklist.txt");
	TAutoPtr<FArchive> PakChunkListFile(IFileManager::Get().CreateFileWriter(*PakChunkListFilename));

	if (!PakChunkListFile.IsValid())
	{
		UE_LOG(LogChunkManifestGenerator, Error, TEXT("Failed to open output pakchunklist file %s"), *PakChunkListFilename);
		return false;
	}

	// generate per-chunk pak list files
	for (int32 Index = 0; Index < ChunkManifests.Num(); ++Index)
	{			
		auto& Manifest = *ChunkManifests[Index];
		if (Manifest.Num())
		{
			FString PakListFilename = FString::Printf(TEXT("%s/pakchunk%d.txt"), *TmpPackagingDir, Index);
			TAutoPtr<FArchive> PakListFile(IFileManager::Get().CreateFileWriter(*PakListFilename));

			if (!PakListFile.IsValid())
			{
				UE_LOG(LogChunkManifestGenerator, Error, TEXT("Failed to open output paklist file %s"), *PakListFilename);
				return false;
			}

			for (auto& Filename : Manifest)
			{
				FString PakListLine = FPaths::ConvertRelativePathToFull(Filename.Replace(TEXT("[Platform]"), *Platform));
				PakListLine.ReplaceInline(TEXT("/"), TEXT("\\"));
				PakListLine += TEXT("\r\n");
				PakListFile->Serialize(TCHAR_TO_ANSI(*PakListLine), PakListLine.Len());
			}

			PakListFile->Close();

			// add this pakfilelist to our master list of pakfilelists
			FString PakChunkListLine = FString::Printf(TEXT("pakchunk%d.txt\r\n"), Index);
			PakChunkListFile->Serialize(TCHAR_TO_ANSI(*PakChunkListLine), PakChunkListLine.Len());
		}
	}

	PakChunkListFile->Close();

	return true;
}

void FChunkManifestGenerator::Initialize(bool InGenerateChunks)
{
	bGenerateChunks = InGenerateChunks;

	// Calculate the largest chunk id used by the registry to get the indices for the default chunks
	AssetRegistry.GetAllAssets(AssetRegistryData);
	int32 LargestChunkID = -1;

	for (int32 Index = 0; Index < AssetRegistryData.Num(); ++Index)
	{
		auto& AssetData = AssetRegistryData[Index];
		auto& RegistryChunkIDs = RegistryChunkIDsMap.FindOrAdd(AssetData.PackageName);
		for (auto ChunkIt = AssetData.ChunkIDs.CreateConstIterator(); ChunkIt; ++ChunkIt)
		{
			int32 ChunkID = *ChunkIt;
			if (ChunkID < 0)
			{
				UE_LOG(LogChunkManifestGenerator, Warning, TEXT("Out of range ChunkID: %d"), ChunkID);
				ChunkID = 0;
			}
			else if (ChunkID > LargestChunkID)
			{
				LargestChunkID = ChunkID;
			}
			if (!RegistryChunkIDs.Contains(ChunkID))
			{
				RegistryChunkIDs.Add(ChunkID);
			}
		}
		// Now clear the original chunk id list. We will fill it with real IDs when cooking.
		AssetData.ChunkIDs.Empty();
		// Map asset data to its package (there can be more than one asset data per package).
		auto& PackageData = PackageToRegistryDataMap.FindOrAdd(AssetData.PackageName);
		PackageData.Add(Index);
	}
	DefaultEngineChunkID = LargestChunkID + 1;
	DefaultGameChunkID = DefaultEngineChunkID + 1;

	// Hook up game delegate
	if (bGenerateChunks)
	{
		FCoreDelegates::OnAssetLoaded.Add(FCoreDelegates::FOnAssetLoaded::FDelegate::CreateRaw(this, &FChunkManifestGenerator::OnAssetLoaded));
	}
}

void FChunkManifestGenerator::AddPackageToChunkManifest(UPackage* Package, const FString& SandboxFilename, const FString& LastLoadedMapName)
{		
	int32 TargetChunk = bGenerateChunks ? INDEX_NONE : 0;
	
	// Collect any existing ChunkIDs this asset has been added to.
	TArray<int32> ExistingChunkIDs;
	FindPackageInManifests(SandboxFilename, ExistingChunkIDs);

	if (TargetChunk == INDEX_NONE || ExistingChunkIDs.Num() == 0)
	{
		if (bGenerateChunks)
		{
			// Try to determine if this package has been loaded as a result of loading a map package.
			FString MapThisAssetWasLoadedWith;
			if (!LastLoadedMapName.IsEmpty())
			{
				if (AssetsLoadedWithLastPackage.Contains(Package->GetFName()))
				{
					MapThisAssetWasLoadedWith = LastLoadedMapName;
				}
			}

			// Collect all chunk IDs associated with this package from the asset registry
			auto& RegistryChunkIDs = RegistryChunkIDsMap.FindOrAdd(Package->GetFName());

			// Try to call game-specific delegate to determine the target chunk ID
			FString Name = Package->GetPathName();
			FGameDelegates::Get().GetAssignStreamingChunkDelegate().ExecuteIfBound(Name, MapThisAssetWasLoadedWith, RegistryChunkIDs, ExistingChunkIDs, TargetChunk);
			if (TargetChunk == INDEX_NONE)
			{
				// Delegate was not bound, or it did not assign any chunk ID
				// Try with asset registry chunks first, we'll pick the first one 
				if (RegistryChunkIDs.Num())
				{
					TargetChunk = RegistryChunkIDs[0];
				}
				// Asset registry doesn't define any chunks for this asset, put it in a default chunk
				else if (Name.StartsWith(TEXT("/Game/")))
				{
					// Game content chunk
					TargetChunk = DefaultGameChunkID;
				}
				else
				{
					// Engine content chunk
					TargetChunk = DefaultEngineChunkID;
				}
			}
		}
		// Now actually add the package to the manifest
		if (!ExistingChunkIDs.Contains(TargetChunk))
		{
			AddPackageToManifest(SandboxFilename, TargetChunk);
			// Fill asset registry data with real IDs
			auto PackageDataList = PackageToRegistryDataMap.Find(Package->GetFName());
			if (PackageDataList != NULL)
			{
				for (auto DataIndex : *PackageDataList)
				{
					auto& AssetData = AssetRegistryData[DataIndex];
					AssetData.ChunkIDs.AddUnique(TargetChunk);
				}
			}
		}
	}
}

void FChunkManifestGenerator::PrepareToLoadNewPackage(const FString& Filename)
{
	AssetsLoadedWithLastPackage.Empty();

	// To make the above work, we need to do a full GC before we load the package
	// to make sure we get all the assets loaded (or at least created) with a map
	TScopedPointer< FArchive > FileReader(IFileManager::Get().CreateFileReader(*Filename));
	if (FileReader)
	{
		// Read package file summary from the file
		FPackageFileSummary PackageSummary;
		(*FileReader) << PackageSummary;
		if (PackageSummary.PackageFlags & PKG_ContainsMap)
		{
			// FULL GC
			UE_LOG(LogChunkManifestGenerator, Display, TEXT("Pre map load Full GC..."));
			CollectGarbage(RF_Native);
		}
	}
}

void FChunkManifestGenerator::CleanManifestDirectories()
{
	for (auto Platform : Platforms)
	{
		CleanTempPackagingDirectory(Platform->PlatformName());
	}
}

bool FChunkManifestGenerator::SaveManifests()
{
	for (auto Platform : Platforms)
	{
		if (!GenerateStreamingInstallManifest(Platform->PlatformName()))
		{
			return false;
		}

		// Generate map for the platform abstraction
		TMultiMap<FString, int32> ChunkMap;	// asset -> ChunkIDs map
		TSet<int32> ChunkIDsInUse;
		const FString PlatformName = Platform->PlatformName();

		// Collect all unique chunk indices and map all files to their chunks
		for (int32 ChunkIndex = 0; ChunkIndex < ChunkManifests.Num(); ++ChunkIndex)
		{
			auto& Manifest = *ChunkManifests[ChunkIndex];
			if (Manifest.Num())
			{
				ChunkIDsInUse.Add(ChunkIndex);				
				for (auto& Filename : Manifest)
				{
					FString PlatFilename = Filename.Replace(TEXT("[Platform]"), *PlatformName);
					ChunkMap.Add(PlatFilename, ChunkIndex);
				}
			}
		}

		// Sort our chunk IDs and file paths
		ChunkMap.KeySort(TLess<FString>());
		ChunkIDsInUse.Sort(TLess<int32>());

		// Platform abstraction will generate any required platform-specific files for the chunks
		if (!Platform->GenerateStreamingInstallManifest(ChunkMap, ChunkIDsInUse))
		{
			return false;
		}
	}
	return true;
}

bool FChunkManifestGenerator::SaveAssetRegistry(const FString& SandboxPath)
{
	// Create asset registry data
	FArrayWriter SerializedAssetRegistry;
	TMap<FName, FAssetData*> GeneratedAssetRegistryData;
	for (auto& AssetData : AssetRegistryData)
	{
		// Add only assets that have actually been cooked and belong to any chunk
		if (AssetData.ChunkIDs.Num() > 0)
		{
			GeneratedAssetRegistryData.Add(AssetData.ObjectPath, &AssetData);
		}
	}
	AssetRegistry.SaveRegistryData(SerializedAssetRegistry, GeneratedAssetRegistryData, GeneratedAssetRegistryData.Num());

	// Save the generated registry for each platform
	for (auto Platform : Platforms)
	{
		FString PlatformSandboxPath = SandboxPath.Replace(TEXT("[Platform]"), *Platform->PlatformName());
		FFileHelper::SaveArrayToFile(SerializedAssetRegistry, *PlatformSandboxPath);
	}
	return true;
}