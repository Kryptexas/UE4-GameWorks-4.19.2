// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Commandlets/ChunkManifestGenerator.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/ArrayReader.h"
#include "Serialization/ArrayWriter.h"
#include "Misc/App.h"
#include "Serialization/JsonTypes.h"
#include "Serialization/JsonReader.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Settings/ProjectPackagingSettings.h"
#include "CollectionManagerTypes.h"
#include "ICollectionManager.h"
#include "CollectionManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "AssetRegistryModule.h"
#include "GameDelegates.h"
#include "Commandlets/ChunkDependencyInfo.h"
#include "IPlatformFileSandboxWrapper.h"
#include "Misc/ConfigCacheIni.h"
#include "Stats/StatsMisc.h"
#include "UniquePtr.h"
#include "Engine/AssetManager.h"

#include "JsonWriter.h"
#include "JsonReader.h"
#include "JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogChunkManifestGenerator, Log, All);

#define LOCTEXT_NAMESPACE "ChunkManifestGenerator"

//////////////////////////////////////////////////////////////////////////
// Static functions
FName GetPackageNameFromDependencyPackageName(const FName RawPackageFName)
{
	FName PackageFName = RawPackageFName;
	if ((FPackageName::IsValidLongPackageName(RawPackageFName.ToString()) == false) &&
		(FPackageName::IsScriptPackage(RawPackageFName.ToString()) == false))
	{
		FText OutReason;
		if (!FPackageName::IsValidLongPackageName(RawPackageFName.ToString(), true, &OutReason))
		{
			const FText FailMessage = FText::Format(LOCTEXT("UnableToGeneratePackageName", "Unable to generate long package name for {0}. {1}"),
				FText::FromString(RawPackageFName.ToString()), OutReason);

			UE_LOG(LogChunkManifestGenerator, Warning, TEXT("%s"), *(FailMessage.ToString()));
			return NAME_None;
		}


		FString LongPackageName;
		if (FPackageName::SearchForPackageOnDisk(RawPackageFName.ToString(), &LongPackageName) == false)
		{
			return NAME_None;
		}
		PackageFName = FName(*LongPackageName);
	}

	// don't include script packages in dependencies as they are always in memory
	if (FPackageName::IsScriptPackage(PackageFName.ToString()))
	{
		// no one likes script packages
		return NAME_None;
	}
	return PackageFName;
}


//////////////////////////////////////////////////////////////////////////
// FChunkManifestGenerator

FChunkManifestGenerator::FChunkManifestGenerator(const ITargetPlatform* InPlatform)
	: AssetRegistry(FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get())
	, TargetPlatform(InPlatform)
	, bGenerateChunks(false)
	, bUseAssetManager(false)
{
	DependencyInfo = GetMutableDefault<UChunkDependencyInfo>();

	bool bOnlyHardReferences = false;
	const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
	if (PackagingSettings)
	{
		bOnlyHardReferences = PackagingSettings->bChunkHardReferencesOnly;
	}	

	UE_LOG(LogChunkManifestGenerator, Log, TEXT("bChunkHardReferencesOnly: %i"), (int32)bOnlyHardReferences);
	DependencyType = bOnlyHardReferences ? EAssetRegistryDependencyType::Hard : EAssetRegistryDependencyType::Packages;

	if (UAssetManager::IsValid() && !FGameDelegates::Get().GetAssignStreamingChunkDelegate().IsBound() && !FGameDelegates::Get().GetGetPackageDependenciesForManifestGeneratorDelegate().IsBound())
	{
		bUseAssetManager = true;

		UAssetManager::Get().UpdateManagementDatabase();
	}
}

FChunkManifestGenerator::~FChunkManifestGenerator()
{
	for (auto ChunkSet : ChunkManifests)
	{
		delete ChunkSet;
	}
	ChunkManifests.Empty();
	for (auto ChunkSet : FinalChunkManifests)
	{
		delete ChunkSet;
	}
	FinalChunkManifests.Empty();
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

	FString ChunkListDir = FPaths::Combine(*FPaths::GameLogDir(), TEXT("ChunkLists"));
	if (IFileManager::Get().DirectoryExists(*ChunkListDir))
	{
		if (!IFileManager::Get().DeleteDirectory(*ChunkListDir, false, true))
		{
			UE_LOG(LogChunkManifestGenerator, Error, TEXT("Failed to delete directory: %s"), *ChunkListDir);
			return false;
		}
	}
	return true;
}

bool FChunkManifestGenerator::ShouldPlatformGenerateStreamingInstallManifest(const ITargetPlatform* Platform) const
{
	if (Platform)
	{
		FConfigFile PlatformIniFile;
		FConfigCacheIni::LoadLocalIniFile(PlatformIniFile, TEXT("Game"), true, *Platform->IniPlatformName());
		FString ConfigString;
		if (PlatformIniFile.GetString(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bGenerateChunks"), ConfigString))
		{
			return FCString::ToBool(*ConfigString);
		}
	}

	return false;
}


int64 FChunkManifestGenerator::GetMaxChunkSizePerPlatform(const ITargetPlatform* Platform) const
{
	if ( Platform )
	{
		FConfigFile PlatformIniFile;
		FConfigCacheIni::LoadLocalIniFile(PlatformIniFile, TEXT("Game"), true, *Platform->IniPlatformName());
		FString ConfigString;
		if (PlatformIniFile.GetString(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("MaxChunkSize"), ConfigString))
		{
			return FCString::Atoi64(*ConfigString);
		}
	}

	return -1;
}

bool FChunkManifestGenerator::GenerateStreamingInstallManifest()
{
	const FString Platform = TargetPlatform->PlatformName();

	FString GameNameLower = FString(FApp::GetGameName()).ToLower();

	// empty out the current paklist directory
	FString TmpPackagingDir = GetTempPackagingDirectoryForPlatform(Platform);

	int64 MaxChunkSize = GetMaxChunkSizePerPlatform( TargetPlatform );

	if (!IFileManager::Get().MakeDirectory(*TmpPackagingDir, true))
	{
		UE_LOG(LogChunkManifestGenerator, Error, TEXT("Failed to create directory: %s"), *TmpPackagingDir);
		return false;
	}
	
	// open a file for writing the list of pak file lists that we've generated
	FString PakChunkListFilename = TmpPackagingDir / TEXT("pakchunklist.txt");
	TUniquePtr<FArchive> PakChunkListFile(IFileManager::Get().CreateFileWriter(*PakChunkListFilename));

	if (!PakChunkListFile)
	{
		UE_LOG(LogChunkManifestGenerator, Error, TEXT("Failed to open output pakchunklist file %s"), *PakChunkListFilename);
		return false;
	}

	FString PakChunkLayerInfoFilename = FString::Printf(TEXT("%s/pakchunklayers.txt"), *TmpPackagingDir);
	TUniquePtr<FArchive> ChunkLayerFile(IFileManager::Get().CreateFileWriter(*PakChunkLayerInfoFilename));

	// generate per-chunk pak list files
	for (int32 Index = 0; Index < FinalChunkManifests.Num(); ++Index)
	{
		// Is this chunk empty?
		if (!FinalChunkManifests[Index] || FinalChunkManifests[Index]->Num() == 0)
		{
			continue;
		}

		int32 FilenameIndex = 0;
		TArray<FString> ChunkFilenames;
		FinalChunkManifests[Index]->GenerateValueArray(ChunkFilenames);
		int32 SubChunkIndex = 0;
		while ( true )
		{
			FString PakChunkFilename = FString::Printf(TEXT("pakchunk%d.txt"), Index);
			if ( SubChunkIndex > 0 )
			{
				PakChunkFilename = FString::Printf(TEXT("pakchunk%d_s%d.txt"), Index, SubChunkIndex);
			}
			++SubChunkIndex;
			FString PakListFilename = FString::Printf(TEXT("%s/%s"), *TmpPackagingDir, *PakChunkFilename, Index);
			TUniquePtr<FArchive> PakListFile(IFileManager::Get().CreateFileWriter(*PakListFilename));

			if (!PakListFile)
			{
				UE_LOG(LogChunkManifestGenerator, Error, TEXT("Failed to open output paklist file %s"), *PakListFilename);
				return false;
			}

			int64 CurrentPakSize = 0;
			bool bFinishedAllFiles = true;

			for (; FilenameIndex < ChunkFilenames.Num(); ++FilenameIndex)
			{
				FString Filename = ChunkFilenames[FilenameIndex];
				FString PakListLine = FPaths::ConvertRelativePathToFull(Filename.Replace(TEXT("[Platform]"), *Platform));
				{
					TArray<FString> FoundFiles;
					FString FileSearchString = FString::Printf(TEXT("%s.*"), *PakListLine);
					IFileManager::Get().FindFiles(FoundFiles, *FileSearchString, true,false);
					const FString Path = FPaths::GetPath(FileSearchString);
					for ( const FString& FoundFile : FoundFiles )
					{
						int64 FileSize = IFileManager::Get().FileSize(*(FPaths::Combine(Path,FoundFile)));
						CurrentPakSize +=  FileSize > 0 ? FileSize : 0;
					}
				}
				if ((MaxChunkSize > 0) && (MaxChunkSize < CurrentPakSize))
				{
					// early out if we are over memory limit
					bFinishedAllFiles = false;
					break;
				}

				PakListLine.ReplaceInline(TEXT("/"), TEXT("\\"));
				PakListLine += TEXT("\r\n");
				PakListFile->Serialize(TCHAR_TO_ANSI(*PakListLine), PakListLine.Len());
			}

			PakListFile->Close();

			// add this pakfilelist to our master list of pakfilelists
			FString PakChunkListLine = FString::Printf(TEXT("%s\r\n"), *PakChunkFilename);
			PakChunkListFile->Serialize(TCHAR_TO_ANSI(*PakChunkListLine), PakChunkListLine.Len());

			int32 TargetLayer = 0;
			FGameDelegates::Get().GetAssignLayerChunkDelegate().ExecuteIfBound(FinalChunkManifests[Index], Platform, Index, TargetLayer);

			FString LayerString = FString::Printf(TEXT("%d\r\n"), TargetLayer);

			ChunkLayerFile->Serialize(TCHAR_TO_ANSI(*LayerString), LayerString.Len());
			
			if (bFinishedAllFiles)
			{
				break;
			}
		}

	}

	ChunkLayerFile->Close();
	PakChunkListFile->Close();

	return true;
}

void FChunkManifestGenerator::GenerateChunkManifestForPackage(const FName& PackageFName, const FString& PackagePathName, const FString& SandboxFilename, const FString& LastLoadedMapName, FSandboxPlatformFile* InSandboxFile)
{
	TArray<int32> TargetChunks;
	TArray<int32> ExistingChunkIDs;

	if (!bGenerateChunks)
	{
		TargetChunks.AddUnique(0);
		ExistingChunkIDs.AddUnique(0);
	}

	if (bGenerateChunks)
	{
		// Collect all chunk IDs associated with this package from the asset registry
		TArray<int32> RegistryChunkIDs = GetAssetRegistryChunkAssignments(PackageFName);

		ExistingChunkIDs = GetExistingPackageChunkAssignments(PackageFName);
		if (bUseAssetManager)
		{
			// No distinction between source of existing chunks for new flow
			RegistryChunkIDs.Append(ExistingChunkIDs);

			UAssetManager::Get().GetPackageChunkIds(PackageFName, TargetPlatform, RegistryChunkIDs, TargetChunks);
		}
		else
		{
			// Try to call game-specific delegate to determine the target chunk ID
			// FString Name = Package->GetPathName();
			if (FGameDelegates::Get().GetAssignStreamingChunkDelegate().IsBound())
			{
				FGameDelegates::Get().GetAssignStreamingChunkDelegate().ExecuteIfBound(PackagePathName, LastLoadedMapName, RegistryChunkIDs, ExistingChunkIDs, TargetChunks);
			}
			else
			{
				//Take asset registry assignments and existing assignments
				TargetChunks.Append(RegistryChunkIDs);
				TargetChunks.Append(ExistingChunkIDs);
			}
		}
	}

	// if the delegate requested a specific chunk assignment, add them package to it now.
	for (const auto& PackageChunk : TargetChunks)
	{
		AddPackageToManifest(SandboxFilename, PackageFName, PackageChunk);
	}
	// If the delegate requested to remove the package from any chunk, remove it now
	for (const auto& PackageChunk : ExistingChunkIDs)
	{
		if (!TargetChunks.Contains(PackageChunk))
		{
			RemovePackageFromManifest(PackageFName, PackageChunk);
		}
	}

}


void FChunkManifestGenerator::CleanManifestDirectories()
{
	CleanTempPackagingDirectory(TargetPlatform->PlatformName());
}

bool FChunkManifestGenerator::SaveManifests(FSandboxPlatformFile* InSandboxFile)
{
	// Always do package dependency work, is required to modify asset registry
	FixupPackageDependenciesForChunks(InSandboxFile);

	if (bGenerateChunks)
	{	
		if (!GenerateStreamingInstallManifest())
		{
			return false;
		}

		// Generate map for the platform abstraction
		TMultiMap<FString, int32> ChunkMap;	// asset -> ChunkIDs map
		TSet<int32> ChunkIDsInUse;
		const FString PlatformName = TargetPlatform->PlatformName();

		// Collect all unique chunk indices and map all files to their chunks
		for (int32 ChunkIndex = 0; ChunkIndex < FinalChunkManifests.Num(); ++ChunkIndex)
		{
			if (FinalChunkManifests[ChunkIndex] && FinalChunkManifests[ChunkIndex]->Num())
			{
				ChunkIDsInUse.Add(ChunkIndex);
				for (auto& Filename : *FinalChunkManifests[ChunkIndex])
				{
					FString PlatFilename = Filename.Value.Replace(TEXT("[Platform]"), *PlatformName);
					ChunkMap.Add(PlatFilename, ChunkIndex);
				}
			}
		}

		// Sort our chunk IDs and file paths
		ChunkMap.KeySort(TLess<FString>());
		ChunkIDsInUse.Sort(TLess<int32>());

		// Platform abstraction will generate any required platform-specific files for the chunks
		if (!TargetPlatform->GenerateStreamingInstallManifest(ChunkMap, ChunkIDsInUse))
		{
			return false;
		}

		if (!bUseAssetManager)
		{
			GenerateAssetChunkInformationCSV(FPaths::Combine(*FPaths::GameLogDir(), TEXT("ChunkLists")));
		}
	}

	return true;
}

bool FChunkManifestGenerator::ContainsMap(const FName& PackageName) const
{
	const auto& Assets = PackageToRegistryDataMap.FindChecked(PackageName);
	
	for (const auto& AssetIndex : Assets)
	{
		const auto& Asset = AssetRegistryData[AssetIndex];
		if (Asset.GetClass()->IsChildOf(UWorld::StaticClass()) || Asset.GetClass()->IsChildOf(ULevel::StaticClass()))
		{
			return true;
		}
	}
	return false;
}

void FChunkManifestGenerator::Initialize(const TArray<FName> &InStartupPackages)
{
	StartupPackages = InStartupPackages;
}


void FChunkManifestGenerator::BuildChunkManifest(const TArray<FName>& CookedPackages, FSandboxPlatformFile* InSandboxFile, bool bGenerateStreamingInstallManifest)
{
	// If we were asked to generate a streaming install manifest explicitly we will generate chunks.
	// Otherwise, we will defer to the config settings for the platform.
	if (bGenerateStreamingInstallManifest)
	{
		bGenerateChunks = true;
	}
	else
	{
		bGenerateChunks = ShouldPlatformGenerateStreamingInstallManifest(TargetPlatform);
	}

	// initialize FoundIDList, PackageChunkIDMap, AssetRegistryData
	AssetRegistry.GetAllAssets(AssetRegistryData, true);

	for (int32 Index = 0; Index < AssetRegistryData.Num(); ++Index)
	{
		auto& AssetData = AssetRegistryData[Index];
		for (auto ChunkIt = AssetData.ChunkIDs.CreateConstIterator(); ChunkIt; ++ChunkIt)
		{
			int32 ChunkID = *ChunkIt;
			if (ChunkID < 0)
			{
				UE_LOG(LogChunkManifestGenerator, Warning, TEXT("Out of range ChunkID: %d"), ChunkID);
				ChunkID = 0;
			}
			
			auto* FoundIDList = PackageChunkIDMap.Find(AssetData.PackageName);
			if (!FoundIDList)
			{
				FoundIDList = &PackageChunkIDMap.Add(AssetData.PackageName);
			}
			FoundIDList->AddUnique(ChunkID);
		}
		// Now clear the original chunk id list. We will fill it with real IDs when cooking.
		AssetData.ChunkIDs.Empty();
		// Map asset data to its package (there can be more than one asset data per package).
		auto& PackageData = PackageToRegistryDataMap.FindOrAdd(AssetData.PackageName);
		PackageData.Add(Index);
	}

	// add all the packages to the unassigned package list
	for (FName CookedPackage : CookedPackages)
	{
		const FString SandboxPath = InSandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*FPackageName::LongPackageNameToFilename(CookedPackage.ToString()));

		AllCookedPackages.Add(CookedPackage, SandboxPath);
		UnassignedPackageSet.Add(CookedPackage, SandboxPath);
	}

	TArray<FName> UnassignedPackageList;

	// Old path has map specific code, new code doesn't care about map or load order
	if (!bUseAssetManager)
	{
		// Assign startup packages, these will generally end up in chunk 0
		FString StartupPackageMapName(TEXT("None"));
		for (FName CookedPackage : StartupPackages)
		{
			const FString SandboxPath = InSandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*FPackageName::LongPackageNameToFilename(CookedPackage.ToString()));
			const FString PackagePathName = CookedPackage.ToString();
			AllCookedPackages.Add(CookedPackage, SandboxPath);
			GenerateChunkManifestForPackage(CookedPackage, PackagePathName, SandboxPath, StartupPackageMapName, InSandboxFile);
		}

		// Capture list at start as it may change during iteration
		UnassignedPackageSet.GenerateKeyArray(UnassignedPackageList);

		// assign chunks for all the map packages
		for (FName MapFName : UnassignedPackageList)
		{
			// this package could be missing from the map because it didn't get cooked. 
			// the reason for this might be that it's a redirector therefore we cooked the package which actually contains the asset
			if (PackageToRegistryDataMap.Find(MapFName) == nullptr)
				continue;

			if (ContainsMap(MapFName) == false)
				continue;

			// get all the dependencies for this map
			TArray<FName> MapDependencies;
			ensure(GatherAllPackageDependencies(MapFName, MapDependencies));

			for (const auto& RawPackageFName : MapDependencies)
			{
				const FName PackageFName = GetPackageNameFromDependencyPackageName(RawPackageFName);

				if (PackageFName == NAME_None)
				{
					continue;
				}

				const FString PackagePathName = PackageFName.ToString();
				const FString MapName = MapFName.ToString();
				const FString* SandboxFilenamePtr = AllCookedPackages.Find(PackageFName);
				if (!SandboxFilenamePtr)
				{
					const FString SandboxPath = InSandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*FPackageName::LongPackageNameToFilename(PackagePathName));

					AllCookedPackages.Add(PackageFName, SandboxPath);

					SandboxFilenamePtr = AllCookedPackages.Find(PackageFName);
					check(SandboxFilenamePtr);
				}
				const FString& SandboxFilename = *SandboxFilenamePtr;

				GenerateChunkManifestForPackage(PackageFName, PackagePathName, SandboxFilename, MapName, InSandboxFile);
			}
		}
	}

	// Capture list at start as it may change during iteration
	UnassignedPackageSet.GenerateKeyArray(UnassignedPackageList);

	// process the remaining unassigned packages
	for (FName PackageFName : UnassignedPackageList)
	{
		const FString& SandboxFilename = AllCookedPackages.FindChecked(PackageFName);
		const FString PackagePathName = PackageFName.ToString();

		GenerateChunkManifestForPackage(PackageFName, PackagePathName, SandboxFilename, FString(), InSandboxFile);
	}

	// anything that remains in the UnAssignedPackageSet will be put in chunk0 when we save the asset registry

}

void FChunkManifestGenerator::AddAssetToFileOrderRecursive(FAssetData* InAsset, TArray<FName>& OutFileOrder, TArray<FName>& OutEncounteredNames, const TMap<FName, FAssetData*>& InAssets, const TArray<FName>& InMapList)
{
	if (!OutEncounteredNames.Contains(InAsset->PackageName))
	{
		OutEncounteredNames.Add(InAsset->PackageName);

		TArray<FName> Dependencies;
		AssetRegistry.GetDependencies(InAsset->PackageName, Dependencies, EAssetRegistryDependencyType::Hard);

		for (auto DependencyName : Dependencies)
		{
			if (InAssets.Contains(DependencyName) && !OutFileOrder.Contains(DependencyName))
			{
				if (!InMapList.Contains(DependencyName))
				{
					auto Dependency = InAssets[DependencyName];
					AddAssetToFileOrderRecursive(Dependency, OutFileOrder, OutEncounteredNames, InAssets, InMapList);
				}
			}
		}

		OutFileOrder.Add(InAsset->PackageName);
	}
}

bool FChunkManifestGenerator::SaveAssetRegistry(const FString& SandboxPath, const TArray<FName>* IgnorePackageList)
{
	UE_LOG(LogChunkManifestGenerator, Display, TEXT("Saving asset registry."));


	TSet<FName> IgnorePackageSet;
	if (IgnorePackageList != nullptr)
	{
		for (const auto& IgnorePackage : *IgnorePackageList)
		{
			IgnorePackageSet.Add(IgnorePackage);
		}
	}
	

	// Create asset registry data
	TArray<FName> MapList;
	FArrayWriter SerializedAssetRegistry;
	SerializedAssetRegistry.SetFilterEditorOnly(true);
	TMap<FName, FAssetData*> GeneratedAssetRegistryData;
	for (auto& AssetData : AssetRegistryData)
	{
		if (IgnorePackageSet.Contains(AssetData.PackageName))
		{
			continue;
		}

		// Add only assets that have actually been cooked and belong to any chunk
		if (AssetData.ChunkIDs.Num() > 0)
		{
			GeneratedAssetRegistryData.Add(AssetData.PackageName, &AssetData);

			if (ContainsMap(AssetData.PackageName))
			{
				MapList.Add(AssetData.PackageName);
			}
		}
	}

	AssetRegistry.SaveRegistryData(SerializedAssetRegistry, GeneratedAssetRegistryData, &MapList);
	UE_LOG(LogChunkManifestGenerator, Display, TEXT("Generated asset registry num assets %d, size is %5.2fkb"), GeneratedAssetRegistryData.Num(), (float)SerializedAssetRegistry.Num() / 1024.f);

	auto CookerFileOrderString = CreateCookerFileOrderString(GeneratedAssetRegistryData, MapList);

	// Save the generated registry

	FString PlatformSandboxPath = SandboxPath.Replace(TEXT("[Platform]"), *TargetPlatform->PlatformName());
	FFileHelper::SaveArrayToFile(SerializedAssetRegistry, *PlatformSandboxPath);

	if (CookerFileOrderString.Len())
	{
		auto OpenOrderFilename = FString::Printf(TEXT("%sBuild/%s/FileOpenOrder/CookerOpenOrder.log"), *FPaths::GameDir(), *TargetPlatform->PlatformName());
		FFileHelper::SaveStringToFile(CookerFileOrderString, *OpenOrderFilename);
	}

	UE_LOG(LogChunkManifestGenerator, Display, TEXT("Done saving asset registry."));

	return true;
}

/** Helper function which reroots a sandbox path to the staging area directory which UnrealPak expects */
inline void ConvertFilenameToPakFormat(FString& InOutPath)
{
	auto GameDir = FPaths::GameDir();
	auto EngineDir = FPaths::EngineDir();
	auto GameName = FApp::GetGameName();

	if (InOutPath.Contains(GameDir))
	{
		FPaths::MakePathRelativeTo(InOutPath, *GameDir);
		InOutPath = FString::Printf(TEXT("../../../%s/%s"), GameName, *InOutPath);
	}
	else if (InOutPath.Contains(EngineDir))
	{
		FPaths::MakePathRelativeTo(InOutPath, *EngineDir);
		InOutPath = FPaths::Combine(TEXT("../../../Engine/"), *InOutPath);
	}
}

void InsertIntoOrder(const TMap<FName, FAssetData*>& InAssets, TMap<FAssetData*, int32>& InOrder, FName InPackageName, int32 InOrderIndex)
{
	if (InAssets.Contains(InPackageName))
	{
		InOrder.Add(InAssets[InPackageName], InOrderIndex);
	}
}

FString FChunkManifestGenerator::CreateCookerFileOrderString(const TMap<FName, FAssetData*>& InAssetData, const TArray<FName>& InMaps)
{
	FString FileOrderString;
	TArray<FAssetData*> TopLevelMapNodes;
	TArray<FAssetData*> TopLevelNodes;
	TMap<FAssetData*, int32> PreferredMapOrders;

#if 0
	// TODO: PreferredMapOrders should be filled out with the maps in the order we wish them to be pak'd.
	int32 Order = 0;
	static const TCHAR* MapOrderList[] = 
	{
	};

	for (int32 i = 0; i < ARRAY_COUNT(MapOrderList); ++i)
	{
		InsertIntoOrder(InAssetData, PreferredMapOrders, FName(MapOrderList[i]), i);
	}
#endif

	for (auto Asset : InAssetData)
	{
		auto PackageName = Asset.Value->PackageName;
		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(PackageName, Referencers);

		bool bIsTopLevel = true;
		bool bIsMap = InMaps.Contains(PackageName);

		if (!bIsMap && Referencers.Num() > 0)
		{
			for (auto ReferencerName : Referencers)
			{
				if (InAssetData.Contains(ReferencerName))
				{
					bIsTopLevel = false;
					break;
				}
			}
		}

		if (bIsTopLevel)
		{
			if (bIsMap)
			{
				TopLevelMapNodes.Add(Asset.Value);
			}
			else
			{
				TopLevelNodes.Add(Asset.Value);
			}
		}
	}

	TopLevelMapNodes.Sort([&PreferredMapOrders](const FAssetData& A, const FAssetData& B)
	{
		auto OrderA = PreferredMapOrders.Find(&A);
		auto OrderB = PreferredMapOrders.Find(&B);
		auto IndexA = OrderA ? *OrderA : INT_MAX;
		auto IndexB = OrderB ? *OrderB : INT_MAX;
		return IndexA < IndexB;
	});

	TArray<FName> FileOrder;
	TArray<FName> EncounteredNames;
	for (auto Asset : TopLevelNodes)
	{
		AddAssetToFileOrderRecursive(Asset, FileOrder, EncounteredNames, InAssetData, InMaps);
	}

	for (auto Asset : TopLevelMapNodes)
	{
		AddAssetToFileOrderRecursive(Asset, FileOrder, EncounteredNames, InAssetData, InMaps);
	}

	int32 CurrentIndex = 0;
	for (auto PackageName : FileOrder)
	{
		auto Asset = InAssetData[PackageName];
		bool bIsMap = InMaps.Contains(Asset->PackageName);
		auto Filename = FPackageName::LongPackageNameToFilename(Asset->PackageName.ToString(), bIsMap ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension());

		ConvertFilenameToPakFormat(Filename);
		auto Line = FString::Printf(TEXT("\"%s\" %i\n"), *Filename, CurrentIndex++);
		FileOrderString.Append(Line);
	}

	return FileOrderString;
}

bool FChunkManifestGenerator::GetPackageDependencyChain(FName SourcePackage, FName TargetPackage, TSet<FName>& VisitedPackages, TArray<FName>& OutDependencyChain)
{	
	//avoid crashing from circular dependencies.
	if (VisitedPackages.Contains(SourcePackage))
	{		
		return false;
	}
	VisitedPackages.Add(SourcePackage);

	if (SourcePackage == TargetPackage)
	{		
		OutDependencyChain.Add(SourcePackage);
		return true;
	}

	TArray<FName> SourceDependencies;
	if (GetPackageDependencies(SourcePackage, SourceDependencies, DependencyType) == false)
	{		
		return false;
	}

	int32 DependencyCounter = 0;
	while (DependencyCounter < SourceDependencies.Num())
	{		
		const FName& ChildPackageName = SourceDependencies[DependencyCounter];
		if (GetPackageDependencyChain(ChildPackageName, TargetPackage, VisitedPackages, OutDependencyChain))
		{
			OutDependencyChain.Add(SourcePackage);
			return true;
		}
		++DependencyCounter;
	}
	
	return false;
}

bool FChunkManifestGenerator::GetPackageDependencies(FName PackageName, TArray<FName>& DependentPackageNames, EAssetRegistryDependencyType::Type InDependencyType)
{	
	if (FGameDelegates::Get().GetGetPackageDependenciesForManifestGeneratorDelegate().IsBound())
	{
		return FGameDelegates::Get().GetGetPackageDependenciesForManifestGeneratorDelegate().Execute(PackageName, DependentPackageNames, InDependencyType);
	}
	else
	{
		return AssetRegistry.GetDependencies(PackageName, DependentPackageNames, InDependencyType);
	}
}

bool FChunkManifestGenerator::GatherAllPackageDependencies(FName PackageName, TArray<FName>& DependentPackageNames)
{	
	if (GetPackageDependencies(PackageName, DependentPackageNames, DependencyType) == false)
	{
		return false;
	}

	TSet<FName> VisitedPackages;
	VisitedPackages.Append(DependentPackageNames);

	int32 DependencyCounter = 0;
	while (DependencyCounter < DependentPackageNames.Num())
	{
		const FName& ChildPackageName = DependentPackageNames[DependencyCounter];
		++DependencyCounter;
		TArray<FName> ChildDependentPackageNames;
		if (GetPackageDependencies(ChildPackageName, ChildDependentPackageNames, DependencyType) == false)
		{
			return false;
		}

		for (const auto& ChildDependentPackageName : ChildDependentPackageNames)
		{
			if (!VisitedPackages.Contains(ChildDependentPackageName))
			{
				DependentPackageNames.Add(ChildDependentPackageName);
				VisitedPackages.Add(ChildDependentPackageName);
			}
		}
	}

	return true;
}

bool FChunkManifestGenerator::GenerateAssetChunkInformationCSV(const FString& OutputPath)
{
	FString TmpString;
	FString CSVString;
	FString HeaderText(TEXT("ChunkID, Package Name, Class Type, Hard or Soft Chunk, File Size, Other Chunks\n"));
	FString EndLine(TEXT("\n"));
	FString NoneText(TEXT("None\n"));
	CSVString = HeaderText;

	for (int32 ChunkID = 0, ChunkNum = FinalChunkManifests.Num(); ChunkID < ChunkNum; ++ChunkID)
	{
		FString PerChunkManifestCSV = HeaderText;
		TMap<FName, FAssetData*> GeneratedAssetRegistryData;
		for (auto& AssetData : AssetRegistryData)
		{
			// Add only assets that have actually been cooked and belong to any chunk
			if (AssetData.ChunkIDs.Num() > 0)
			{
				FString Fullname;
				if (AssetData.ChunkIDs.Contains(ChunkID) && FPackageName::DoesPackageExist(*AssetData.PackageName.ToString(), nullptr, &Fullname))
				{
					auto FileSize = IFileManager::Get().FileSize(*FPackageName::LongPackageNameToFilename(*AssetData.PackageName.ToString(), FPackageName::GetAssetPackageExtension()));
					if (FileSize == INDEX_NONE)
					{
						FileSize = IFileManager::Get().FileSize(*FPackageName::LongPackageNameToFilename(*AssetData.PackageName.ToString(), FPackageName::GetMapPackageExtension()));
					}

					if (FileSize == INDEX_NONE)
					{
						FileSize = 0;
					}

					FString SoftChain;
					bool bHardChunk = false;
					if (ChunkID < ChunkManifests.Num())
					{
						bHardChunk = ChunkManifests[ChunkID] && ChunkManifests[ChunkID]->Contains(AssetData.PackageName);
					
						if (!bHardChunk)
						{
							//
							SoftChain = GetShortestReferenceChain(AssetData.PackageName, ChunkID);
						}
					}
					if (SoftChain.IsEmpty())
					{
						SoftChain = TEXT("Soft: Possibly Unassigned Asset");
					}

					TmpString = FString::Printf(TEXT("%d,%s,%s,%s,%lld,"), ChunkID, *AssetData.PackageName.ToString(), *AssetData.AssetClass.ToString(), bHardChunk ? TEXT("Hard") : *SoftChain, FileSize);
					CSVString += TmpString;
					PerChunkManifestCSV += TmpString;
					if (AssetData.ChunkIDs.Num() == 1)
					{
						CSVString += NoneText;
						PerChunkManifestCSV += NoneText;
					}
					else
					{
						for (const auto& OtherChunk : AssetData.ChunkIDs)
						{
							if (OtherChunk != ChunkID)
							{
								TmpString = FString::Printf(TEXT("%d "), OtherChunk);
								CSVString += TmpString;
								PerChunkManifestCSV += TmpString;
							}
						}
						CSVString += EndLine;
						PerChunkManifestCSV += EndLine;
					}
				}
			}
		}

		FFileHelper::SaveStringToFile(PerChunkManifestCSV, *FPaths::Combine(*OutputPath, *FString::Printf(TEXT("Chunks%dInfo.csv"), ChunkID)));
	}

	return FFileHelper::SaveStringToFile(CSVString, *FPaths::Combine(*OutputPath, TEXT("AllChunksInfo.csv")));
}

void FChunkManifestGenerator::AddPackageToManifest(const FString& PackageSandboxPath, FName PackageName, int32 ChunkId)
{
	while (ChunkId >= ChunkManifests.Num())
	{
		ChunkManifests.Add(nullptr);
	}
	if (!ChunkManifests[ChunkId])
	{
		ChunkManifests[ChunkId] = new FChunkPackageSet();
	}
	ChunkManifests[ChunkId]->Add(PackageName, PackageSandboxPath);
	//Safety check, it the package happens to exist in the unassigned list remove it now.
	UnassignedPackageSet.Remove(PackageName);
}


void FChunkManifestGenerator::RemovePackageFromManifest(FName PackageName, int32 ChunkId)
{
	if (ChunkManifests[ChunkId])
	{
		ChunkManifests[ChunkId]->Remove(PackageName);
	}
}

void FChunkManifestGenerator::ResolveChunkDependencyGraph(const FChunkDependencyTreeNode& Node, FChunkPackageSet BaseAssetSet, TArray<TArray<FName>>& OutPackagesMovedBetweenChunks)
{
	if (FinalChunkManifests.Num() > Node.ChunkID && FinalChunkManifests[Node.ChunkID])
	{
		for (auto It = BaseAssetSet.CreateConstIterator(); It; ++It)
		{
			// Remove any assets belonging to our parents.			
			if (FinalChunkManifests[Node.ChunkID]->Remove(It.Key()) > 0)
			{
				OutPackagesMovedBetweenChunks[Node.ChunkID].Add(It.Key());
				UE_LOG(LogChunkManifestGenerator, Verbose, TEXT("Removed %s from chunk %i because it is duplicated in another chunk."), *It.Key().ToString(), Node.ChunkID);
			}
		}
		// Add the current Chunk's assets
		for (auto It = FinalChunkManifests[Node.ChunkID]->CreateConstIterator(); It; ++It)//for (const auto It : *(FinalChunkManifests[Node.ChunkID]))
		{
			BaseAssetSet.Add(It.Key(), It.Value());
		}
		for (const auto It : Node.ChildNodes)
		{
			ResolveChunkDependencyGraph(It, BaseAssetSet, OutPackagesMovedBetweenChunks);
		}
	}
}

bool FChunkManifestGenerator::CheckChunkAssetsAreNotInChild(const FChunkDependencyTreeNode& Node)
{
	for (const auto It : Node.ChildNodes)
	{
		if (!CheckChunkAssetsAreNotInChild(It))
		{
			return false;
		}
	}

	if (!(FinalChunkManifests.Num() > Node.ChunkID && FinalChunkManifests[Node.ChunkID]))
	{
		return true;
	}

	for (const auto ChildIt : Node.ChildNodes)
	{
		if(FinalChunkManifests.Num() > ChildIt.ChunkID && FinalChunkManifests[ChildIt.ChunkID])
		{
			for (auto It = FinalChunkManifests[Node.ChunkID]->CreateConstIterator(); It; ++It)
			{
				if (FinalChunkManifests[ChildIt.ChunkID]->Find(It.Key()))
				{
					return false;
				}
			}
		}
	}

	return true;
}

void FChunkManifestGenerator::AddPackageAndDependenciesToChunk(FChunkPackageSet* ThisPackageSet, FName InPkgName, const FString& InSandboxFile, int32 ChunkID, FSandboxPlatformFile* SandboxPlatformFile)
{
	FChunkPackageSet* InitialPackageSetForThisChunk = ChunkManifests.IsValidIndex(ChunkID) ? ChunkManifests[ChunkID] : nullptr;

	//Add this asset
	ThisPackageSet->Add(InPkgName, InSandboxFile);

	// Only gather dependencies the slow way if we're chunking or not using asset manager
	if (!bGenerateChunks || bUseAssetManager)
	{
		return;
	}

	//now add any dependencies
	TArray<FName> DependentPackageNames;
	if (GatherAllPackageDependencies(InPkgName, DependentPackageNames))
	{
		for (const auto& PkgName : DependentPackageNames)
		{
			bool bSkip = false;
			if (ChunkID != 0 && FinalChunkManifests[0])
			{
				// Do not add if this asset was assigned to the 0 chunk. These assets always exist on disk
				bSkip = FinalChunkManifests[0]->Contains(PkgName);
			}
			if (!bSkip)
			{
				const FName FilteredPackageName = GetPackageNameFromDependencyPackageName(PkgName);
				if (FilteredPackageName == NAME_None)
				{
					continue;
				}
				FString DependentSandboxFile = SandboxPlatformFile->ConvertToAbsolutePathForExternalAppForWrite(*FPackageName::LongPackageNameToFilename(*FilteredPackageName.ToString()));
				if (!ThisPackageSet->Contains(FilteredPackageName))
				{
					if ((InitialPackageSetForThisChunk != nullptr) && InitialPackageSetForThisChunk->Contains(PkgName))
					{
						// Don't print anything out; it was pre-assigned to this chunk but we haven't gotten to it yet in the calling loop; we'll go ahead and grab it now
					}
					else
					{
						if (UE_LOG_ACTIVE(LogChunkManifestGenerator, Verbose))
						{
							// It was not assigned to this chunk and we're forcing it to be dragged in, let the user known
							UE_LOG(LogChunkManifestGenerator, Verbose, TEXT("Adding %s to chunk %i because %s depends on it."), *FilteredPackageName.ToString(), ChunkID, *InPkgName.ToString());

							TSet<FName> VisitedPackages;
							TArray<FName> DependencyChain;
							GetPackageDependencyChain(InPkgName, PkgName, VisitedPackages, DependencyChain);
							for (const auto& ChainName : DependencyChain)
							{
								UE_LOG(LogChunkManifestGenerator, Verbose, TEXT("\tchain: %s"), *ChainName.ToString());
							}
						}
					}
				}
				ThisPackageSet->Add(FilteredPackageName, DependentSandboxFile);
				UnassignedPackageSet.Remove(PkgName);
			}
		}
	}
}

void FChunkManifestGenerator::FixupPackageDependenciesForChunks(FSandboxPlatformFile* InSandboxFile)
{
	UE_LOG(LogChunkManifestGenerator, Log, TEXT("Starting FixupPackageDependenciesForChunks..."));
	SCOPE_LOG_TIME_IN_SECONDS(TEXT("... FixupPackageDependenciesForChunks complete."), nullptr);

	for (int32 ChunkID = 0, MaxChunk = ChunkManifests.Num(); ChunkID < MaxChunk; ++ChunkID)
	{
		FinalChunkManifests.Add(nullptr);
		if (!ChunkManifests[ChunkID])
		{
			continue;
		}
		FinalChunkManifests[ChunkID] = new FChunkPackageSet();
		for (auto It = ChunkManifests[ChunkID]->CreateConstIterator(); It; ++It)
		{
			AddPackageAndDependenciesToChunk(FinalChunkManifests[ChunkID], It.Key(), It.Value(), ChunkID, InSandboxFile);
		}
	}

	auto* ChunkDepGraph = DependencyInfo->GetChunkDependencyGraph(ChunkManifests.Num());
	//Once complete, Add any remaining assets (that are not assigned to a chunk) to the first chunk.
	if (FinalChunkManifests.Num() == 0)
	{
		FinalChunkManifests.Add(nullptr);
	}
	if (!FinalChunkManifests[0])
	{
		FinalChunkManifests[0] = new FChunkPackageSet();
	}
	// Copy the remaining assets
	auto RemainingAssets = UnassignedPackageSet;
	for (auto It = RemainingAssets.CreateConstIterator(); It; ++It)
	{
		AddPackageAndDependenciesToChunk(FinalChunkManifests[0], It.Key(), It.Value(), 0, InSandboxFile);
	}

	if (!CheckChunkAssetsAreNotInChild(*ChunkDepGraph))
	{
		UE_LOG(LogChunkManifestGenerator, Log, TEXT("Initial scan of chunks found duplicate assets in graph children"));
	}
		
	TArray<TArray<FName>> PackagesRemovedFromChunks;
	PackagesRemovedFromChunks.AddDefaulted(ChunkManifests.Num());

	//Finally, if the previous step may added any extra packages to the 0 chunk. Pull them out of other chunks and save space
	ResolveChunkDependencyGraph(*ChunkDepGraph, FChunkPackageSet(), PackagesRemovedFromChunks);

	for (int32 i = 0; i < ChunkManifests.Num(); ++i)
	{
		if (!bUseAssetManager)
		{
			FName CollectionName(*FString::Printf(TEXT("PackagesRemovedFromChunk%i"), i));
			if (CreateOrEmptyCollection(CollectionName))
			{
				WriteCollection(CollectionName, PackagesRemovedFromChunks[i]);
			}
		}
	}

	if (!CheckChunkAssetsAreNotInChild(*ChunkDepGraph))
	{
		UE_LOG(LogChunkManifestGenerator, Error, TEXT("Second Scan of chunks found duplicate asset entries in children."));
	}

	for (int32 ChunkID = 0, MaxChunk = ChunkManifests.Num(); ChunkID < MaxChunk; ++ChunkID)
	{
		const int32 ChunkManifestNum = ChunkManifests[ChunkID] ? ChunkManifests[ChunkID]->Num() : 0;
		const int32 FinalChunkManifestNum = FinalChunkManifests[ChunkID] ? FinalChunkManifests[ChunkID]->Num() : 0;
		UE_LOG(LogChunkManifestGenerator, Log, TEXT("Chunk: %i, Started with %i packages, Final after dependency resolve: %i"), ChunkID, ChunkManifestNum, FinalChunkManifestNum);
	}
	

	// Fix up the asset registry to reflect this chunk layout
	for (int32 ChunkID = 0, MaxChunk = FinalChunkManifests.Num(); ChunkID < MaxChunk; ++ChunkID)
	{
		if (!FinalChunkManifests[ChunkID])
		{
			continue;
		}
		for (const auto& Asset : *FinalChunkManifests[ChunkID])
		{
			auto* AssetIndexArray = PackageToRegistryDataMap.Find(Asset.Key);
			if (AssetIndexArray)
			{
				for (auto AssetIndex : *AssetIndexArray)
				{
					AssetRegistryData[AssetIndex].ChunkIDs.AddUnique(ChunkID);
				}
			}
		}
	}
}


void FChunkManifestGenerator::FindShortestReferenceChain(TArray<FReferencePair> PackageNames, int32 ChunkID, uint32& OutParentIndex, FString& OutChainPath)
{
	TArray<FReferencePair> ReferencesToCheck;
	uint32 Index = 0;
	for (const auto& Pkg : PackageNames)
	{
		if (ChunkManifests[ChunkID] && ChunkManifests[ChunkID]->Contains(Pkg.PackageName))
		{
			OutChainPath += TEXT("Soft: ");
			OutChainPath += Pkg.PackageName.ToString();
			OutParentIndex = Pkg.ParentNodeIndex;
			return;
		}
		TArray<FName> AssetReferences;
		AssetRegistry.GetReferencers(Pkg.PackageName, AssetReferences);
		for (const auto& Ref : AssetReferences)
		{
			if (!InspectedNames.Contains(Ref))
			{
				ReferencesToCheck.Add(FReferencePair(Ref, Index));
				InspectedNames.Add(Ref);
			}
		}

		++Index;
	}

	if (ReferencesToCheck.Num() > 0)
	{
		uint32 ParentIndex = INDEX_NONE;
		FindShortestReferenceChain(ReferencesToCheck, ChunkID, ParentIndex, OutChainPath);

		if (ParentIndex < (uint32)PackageNames.Num())
		{
			OutChainPath += TEXT("->");
			OutChainPath += PackageNames[ParentIndex].PackageName.ToString();
			OutParentIndex = PackageNames[ParentIndex].ParentNodeIndex;
		}
	}
	else if (PackageNames.Num() > 0)
	{
		//best guess
		OutChainPath += TEXT("Soft From Unassigned Package? Best Guess: ");
		OutChainPath += PackageNames[0].PackageName.ToString();
		OutParentIndex = PackageNames[0].ParentNodeIndex;
	}
}

FString FChunkManifestGenerator::GetShortestReferenceChain(FName PackageName, int32 ChunkID)
{
	FString StringChain;
	TArray<FReferencePair> ReferencesToCheck;
	uint32 ParentIndex;
	ReferencesToCheck.Add(FReferencePair(PackageName, 0));
	InspectedNames.Empty();
	InspectedNames.Add(PackageName);
	FindShortestReferenceChain(ReferencesToCheck, ChunkID, ParentIndex, StringChain);

	return StringChain;
}


bool FChunkManifestGenerator::CreateOrEmptyCollection(FName CollectionName)
{
	ICollectionManager& CollectionManager = FCollectionManagerModule::GetModule().Get();

	if (CollectionManager.CollectionExists(CollectionName, ECollectionShareType::CST_Local))
	{
		return CollectionManager.EmptyCollection(CollectionName, ECollectionShareType::CST_Local);
	}
	else if (CollectionManager.CreateCollection(CollectionName, ECollectionShareType::CST_Local, ECollectionStorageMode::Static))
	{
		return true;
	}

	return false;
}

void FChunkManifestGenerator::WriteCollection(FName CollectionName, const TArray<FName>& PackageNames)
{
	if (CreateOrEmptyCollection(CollectionName))
	{
		TArray<FName> AssetNames = PackageNames;

		// Convert package names to asset names
		for (FName& Name : AssetNames)
		{
			FString PackageName = Name.ToString();
			int32 LastPathDelimiter;
			if (PackageName.FindLastChar(TEXT('/'), /*out*/ LastPathDelimiter))
			{
				const FString AssetName = PackageName.Mid(LastPathDelimiter + 1);
				PackageName = PackageName + FString(TEXT(".")) + AssetName;
				Name = *PackageName;
			}
		}

		ICollectionManager& CollectionManager = FCollectionManagerModule::GetModule().Get();
		CollectionManager.AddToCollection(CollectionName, ECollectionShareType::CST_Local, AssetNames);

		UE_LOG(LogChunkManifestGenerator, Log, TEXT("Updated collection %s"), *CollectionName.ToString());
	}
	else
	{
		UE_LOG(LogChunkManifestGenerator, Warning, TEXT("Failed to update collection %s"), *CollectionName.ToString());
	}
}

#undef LOCTEXT_NAMESPACE
