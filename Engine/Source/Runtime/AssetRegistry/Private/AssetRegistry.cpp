// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "AssetRegistry.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/ArrayReader.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "UObject/MetaData.h"
#include "AssetRegistryPrivate.h"
#include "ARFilter.h"
#include "DependsNode.h"
#include "PackageReader.h"
#include "GenericPlatform/GenericPlatformChunkInstall.h"
#include "IPluginManager.h"

#if WITH_EDITOR
#include "IDirectoryWatcher.h"
#include "DirectoryWatcherModule.h"
#endif // WITH_EDITOR

DEFINE_LOG_CATEGORY(LogAssetRegistry);

/** Returns the appropriate ChunkProgressReportingType for the given Asset enum */
EChunkProgressReportingType::Type GetChunkAvailabilityProgressType(EAssetAvailabilityProgressReportingType::Type ReportType)
{
	EChunkProgressReportingType::Type ChunkReportType;
	switch (ReportType)
	{
	case EAssetAvailabilityProgressReportingType::ETA:
		ChunkReportType = EChunkProgressReportingType::ETA;
		break;
	case EAssetAvailabilityProgressReportingType::PercentageComplete:
		ChunkReportType = EChunkProgressReportingType::PercentageComplete;
		break;
	default:
		ChunkReportType = EChunkProgressReportingType::PercentageComplete;
		UE_LOG(LogAssetRegistry, Error, TEXT("Unsupported assetregistry report type: %i"), (int)ReportType);
		break;
	}
	return ChunkReportType;
}

FAssetRegistry::FAssetRegistry()
{
	const double StartupStartTime = FPlatformTime::Seconds();

	bInitialSearchCompleted = true;
	AmortizeStartTime = 0;
	TotalAmortizeTime = 0;

	MaxSecondsPerFrame = 0.015;

	// By default update the disk cache once on asset load, to incorporate changes made in PostLoad. This only happens in editor builds
	bUpdateDiskCacheAfterLoad = true;

	// Collect all code generator classes (currently BlueprintCore-derived ones)
	CollectCodeGeneratorClasses();

	// Read default serialization options
	InitializeSerializationOptions(SerializationOptions);

	// If in the editor, we scan all content right now
	// If in the game, we expect user to make explicit sync queries using ScanPathsSynchronous
	// If in a commandlet, we expect the commandlet to decide when to perform a synchronous scan
	if(GIsEditor && !IsRunningCommandlet())
	{
		bInitialSearchCompleted = false;
		SearchAllAssets(false);
	}
	// for platforms that require cooked data, we attempt to load a premade asset registry
	else if (FPlatformProperties::RequiresCookedData())
	{
		bool bLoadedDevelopment = false;
		// load the cooked data
		FArrayReader SerializedAssetData;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// Allow loading development asset registry
		FString DevAssetRegistryFilename = (FPaths::GameDir() / TEXT("DevelopmentAssetRegistry.bin"));
		if (FParse::Param(FCommandLine::Get(), TEXT("LoadDevAssetRegistry")) && IFileManager::Get().FileExists(*DevAssetRegistryFilename) && FFileHelper::LoadFileToArray(SerializedAssetData, *DevAssetRegistryFilename))
		{
			SerializationOptions.ModifyForDevelopment();

			Serialize(SerializedAssetData);
			bLoadedDevelopment = true;
		}
#endif
		FString AssetRegistryFilename = (FPaths::GameDir() / TEXT("AssetRegistry.bin"));
		if (SerializationOptions.bSerializeAssetRegistry && !bLoadedDevelopment && IFileManager::Get().FileExists(*AssetRegistryFilename) && FFileHelper::LoadFileToArray(SerializedAssetData, *AssetRegistryFilename))
		{
			// serialize the data with the memory reader (will convert FStrings to FNames, etc)
			Serialize(SerializedAssetData);
		}
		TArray<TSharedRef<IPlugin>> PakPlugins = IPluginManager::Get().GetPluginsWithPakFile();
		for (TSharedRef<IPlugin> PakPlugin : PakPlugins)
		{
			if (FFileHelper::LoadFileToArray(SerializedAssetData, *(PakPlugin->GetBaseDir() / TEXT("AssetRegistry.bin"))))
			{
				SerializedAssetData.Seek(0);
				Serialize(SerializedAssetData);
			}
		}
	}

	// Report startup time. This does not include DirectoryWatcher startup time.
	UE_LOG(LogAssetRegistry, Log, TEXT( "FAssetRegistry took %0.4f seconds to start up" ), FPlatformTime::Seconds() - StartupStartTime );

#if WITH_EDITOR
	// Commandlets and in-game don't listen for directory changes
	if ( !IsRunningCommandlet() && GIsEditor)
	{
		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

		if (DirectoryWatcher)
		{
			TArray<FString> RootContentPaths;
			FPackageName::QueryRootContentPaths( RootContentPaths );
			for( TArray<FString>::TConstIterator RootPathIt( RootContentPaths ); RootPathIt; ++RootPathIt )
			{
				const FString& RootPath = *RootPathIt;
				const FString& ContentFolder = FPackageName::LongPackageNameToFilename( RootPath );

				// This could be due to a plugin that specifies it contains content, yet has no content yet. PluginManager
				// Mounts these folders anyway which results in then being returned from QueryRootContentPaths
				if (IFileManager::Get().DirectoryExists(*ContentFolder))
				{
					FDelegateHandle NewHandle;
					DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(ContentFolder, IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FAssetRegistry::OnDirectoryChanged), NewHandle);
					OnDirectoryChangedDelegateHandles.Add(ContentFolder, NewHandle);
				}
			}
		}
	}

	if (GConfig)
	{
		GConfig->GetBool(TEXT("AssetRegistry"), TEXT("bUpdateDiskCacheAfterLoad"), bUpdateDiskCacheAfterLoad, GEngineIni);
	}

	if (bUpdateDiskCacheAfterLoad)
	{
		FCoreUObjectDelegates::OnAssetLoaded.AddRaw(this, &FAssetRegistry::OnAssetLoaded);
	}
#endif // WITH_EDITOR

	// Listen for new content paths being added or removed at runtime.  These are usually plugin-specific asset paths that
	// will be loaded a bit later on.
	FPackageName::OnContentPathMounted().AddRaw( this, &FAssetRegistry::OnContentPathMounted );
	FPackageName::OnContentPathDismounted().AddRaw( this, &FAssetRegistry::OnContentPathDismounted );
}

void FAssetRegistry::InitializeSerializationOptions(FAssetRegistrySerializationOptions& Options, const FString& PlatformIniName) const
{
	// Use passed in platform, or current platform if empty
	const TCHAR* FinalPlatformName = !PlatformIniName.IsEmpty() ? *PlatformIniName : ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName());

	FConfigFile PlatformEngineIni;
	FConfigCacheIni::LoadLocalIniFile(PlatformEngineIni, TEXT("Engine"), true, FinalPlatformName);

	PlatformEngineIni.GetBool(TEXT("AssetRegistry"), TEXT("bSerializeAssetRegistry"), Options.bSerializeAssetRegistry);
	PlatformEngineIni.GetBool(TEXT("AssetRegistry"), TEXT("bSerializeDependencies"), Options.bSerializeDependencies);
	PlatformEngineIni.GetBool(TEXT("AssetRegistry"), TEXT("bSerializeNameDependencies"), Options.bSerializeSearchableNameDependencies);
	PlatformEngineIni.GetBool(TEXT("AssetRegistry"), TEXT("bSerializeManageDependencies"), Options.bSerializeManageDependencies);
	PlatformEngineIni.GetBool(TEXT("AssetRegistry"), TEXT("bSerializePackageData"), Options.bSerializePackageData);
	PlatformEngineIni.GetBool(TEXT("AssetRegistry"), TEXT("bUseAssetRegistryTagsWhitelistInsteadOfBlacklist"), Options.bUseAssetRegistryTagsWhitelistInsteadOfBlacklist);
		
	TArray<FString> FilterlistItems;
	if (Options.bUseAssetRegistryTagsWhitelistInsteadOfBlacklist)
	{
		PlatformEngineIni.GetArray(TEXT("AssetRegistry"), TEXT("CookedTagsWhitelist"), FilterlistItems);
	}
	else
	{
		PlatformEngineIni.GetArray(TEXT("AssetRegistry"), TEXT("CookedTagsBlacklist"), FilterlistItems);
	}

	// Takes on the pattern "(Class=SomeClass,Tag=SomeTag)"
	for (const FString& FilterlistItem : FilterlistItems)
	{
		FString TrimmedFilterlistItem = FilterlistItem;
		TrimmedFilterlistItem.Trim();
		TrimmedFilterlistItem.TrimTrailing();
		if (TrimmedFilterlistItem.Left(1) == TEXT("("))
		{
			TrimmedFilterlistItem = TrimmedFilterlistItem.RightChop(1);
		}
		if (TrimmedFilterlistItem.Right(1) == TEXT(")"))
		{
			TrimmedFilterlistItem = TrimmedFilterlistItem.LeftChop(1);
		}

		TArray<FString> Tokens;
		TrimmedFilterlistItem.ParseIntoArray(Tokens, TEXT(","));
		FString ClassName;
		FString TagName;

		for(const FString& Token : Tokens)
		{
			FString KeyString;
			FString ValueString;
			if (Token.Split(TEXT("="), &KeyString, &ValueString))
			{
				KeyString.Trim();
				KeyString.TrimTrailing();
				ValueString.Trim();
				ValueString.TrimTrailing();
				if (KeyString == TEXT("Class"))
				{
					ClassName = ValueString;
				}
				else if (KeyString == TEXT("Tag"))
				{
					TagName = ValueString;
				}
			}
		}

		if (!ClassName.IsEmpty() && !TagName.IsEmpty())
		{
			FName TagFName = FName(*TagName);

			// Include subclasses if the class is in memory at this time (native classes only)
			UClass* FilterlistClass = Cast<UClass>(StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *ClassName));
			if (FilterlistClass)
			{
				Options.CookFilterlistTagsByClass.FindOrAdd(FilterlistClass->GetFName()).Add(TagFName);

				TArray<UClass*> DerivedClasses;
				GetDerivedClasses(FilterlistClass, DerivedClasses);
				for (UClass* DerivedClass : DerivedClasses)
				{
					Options.CookFilterlistTagsByClass.FindOrAdd(DerivedClass->GetFName()).Add(TagFName);
				}
			}
			else
			{
				// Class is not in memory yet. Just add an explicit filter.
				// Automatically adding subclasses of non-native classes is not supported.
				// In these cases, using Class=* is usually sufficient
				Options.CookFilterlistTagsByClass.FindOrAdd(FName(*ClassName)).Add(TagFName);
			}
		}
	}
}

void FAssetRegistry::CollectCodeGeneratorClasses()
{
	// Work around the fact we don't reference Engine module directly
	UClass* BlueprintCoreClass = Cast<UClass>(StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, TEXT("BlueprintCore")));
	if (BlueprintCoreClass)
	{
		ClassGeneratorNames.Add(BlueprintCoreClass->GetFName());

		TArray<UClass*> BlueprintCoreDerivedClasses;
		GetDerivedClasses(BlueprintCoreClass, BlueprintCoreDerivedClasses);
		for (UClass* BPCoreClass : BlueprintCoreDerivedClasses)
		{
			ClassGeneratorNames.Add(BPCoreClass->GetFName());
		}
	}
}

FAssetRegistry::~FAssetRegistry()
{
	// Make sure the asset search thread is closed
	if ( BackgroundAssetSearch.IsValid() )
	{
		BackgroundAssetSearch->EnsureCompletion();
		BackgroundAssetSearch.Reset();
	}

	// Stop listening for content mount point events
	FPackageName::OnContentPathMounted().RemoveAll( this );
	FPackageName::OnContentPathDismounted().RemoveAll( this );

	// Commandlets dont listen for directory changes
#if WITH_EDITOR
	if ( !IsRunningCommandlet() && GIsEditor )
	{
		// If the directory module is still loaded, unregister any delegates
		if ( FModuleManager::Get().IsModuleLoaded("DirectoryWatcher") )
		{
			FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::GetModuleChecked<FDirectoryWatcherModule>("DirectoryWatcher");
			IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

			if (DirectoryWatcher)
			{
				TArray<FString> RootContentPaths;
				FPackageName::QueryRootContentPaths( RootContentPaths );
				for( TArray<FString>::TConstIterator RootPathIt( RootContentPaths ); RootPathIt; ++RootPathIt )
				{
					const FString& RootPath = *RootPathIt;
					const FString& ContentFolder = FPackageName::LongPackageNameToFilename( RootPath );
					DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle( ContentFolder, OnDirectoryChangedDelegateHandles.FindRef(ContentFolder));
					OnDirectoryChangedDelegateHandles.Remove(ContentFolder);
				}
			}
		}
	}

	if (bUpdateDiskCacheAfterLoad)
	{
		FCoreUObjectDelegates::OnAssetLoaded.RemoveAll(this);
	}
#endif // WITH_EDITOR

	// Clear all listeners
	AssetAddedEvent.Clear();
	AssetRemovedEvent.Clear();
	AssetRenamedEvent.Clear();
	InMemoryAssetCreatedEvent.Clear();
	InMemoryAssetDeletedEvent.Clear();
	FileLoadedEvent.Clear();
	FileLoadProgressUpdatedEvent.Clear();
}

void FAssetRegistry::SearchAllAssets(bool bSynchronousSearch)
{
	// Mark the time before the first search started
	FullSearchStartTime = FPlatformTime::Seconds();

	// Figure out what all of the root asset directories are.  This will include Engine content, Game content, but also may include
	// mounted content directories for one or more plugins.	Also keep in mind that plugins may become loaded later on.  We'll listen
	// for that via a delegate, and add those directories to scan later as them come in.
	TArray<FString> PathsToSearch;
	FPackageName::QueryRootContentPaths( PathsToSearch );

	// Start the asset search (synchronous in commandlets)
	if ( bSynchronousSearch )
	{
		const bool bForceRescan = false;
		ScanPathsAndFilesSynchronous(PathsToSearch, TArray<FString>(), bForceRescan, EAssetDataCacheMode::UseMonolithicCache);
	}
	else if ( !BackgroundAssetSearch.IsValid() )
	{
		// if the BackgroundAssetSearch is already valid then we have already called it before
		BackgroundAssetSearch = MakeShareable(new FAssetDataGatherer(PathsToSearch, TArray<FString>(), bSynchronousSearch, EAssetDataCacheMode::UseMonolithicCache));
	}
}


bool FAssetRegistry::GetAssetsByPackageName(FName PackageName, TArray<FAssetData>& OutAssetData, bool bIncludeOnlyOnDiskAssets ) const
{
	FARFilter Filter;
	Filter.PackageNames.Add(PackageName);
	Filter.bIncludeOnlyOnDiskAssets = bIncludeOnlyOnDiskAssets;
	return GetAssets(Filter, OutAssetData);
}

bool FAssetRegistry::GetAssetsByPath(FName PackagePath, TArray<FAssetData>& OutAssetData, bool bRecursive, bool bIncludeOnlyOnDiskAssets) const
{
	FARFilter Filter;
	Filter.bRecursivePaths = bRecursive;
	Filter.PackagePaths.Add(PackagePath);
	Filter.bIncludeOnlyOnDiskAssets = bIncludeOnlyOnDiskAssets;
	return GetAssets(Filter, OutAssetData);
}

bool FAssetRegistry::GetAssetsByClass(FName ClassName, TArray<FAssetData>& OutAssetData, bool bSearchSubClasses) const
{
	FARFilter Filter;
	Filter.ClassNames.Add(ClassName);
	Filter.bRecursiveClasses = bSearchSubClasses;
	return GetAssets(Filter, OutAssetData);
}

bool FAssetRegistry::GetAssetsByTagValues(const TMultiMap<FName, FString>& AssetTagsAndValues, TArray<FAssetData>& OutAssetData) const
{
	FARFilter Filter;
	Filter.TagsAndValues = AssetTagsAndValues;
	return GetAssets(Filter, OutAssetData);
}

bool FAssetRegistry::GetAssets(const FARFilter& InFilter, TArray<FAssetData>& OutAssetData) const
{
	double GetAssetsStartTime = FPlatformTime::Seconds();

	// Verify filter input. If all assets are needed, use GetAllAssets() instead.
	if ( !FAssetRegistryState::IsFilterValid(InFilter, true) || InFilter.IsEmpty() )
	{
		return false;
	}

	// Expand recursion on filter
	FARFilter Filter;
	ExpandRecursiveFilter(InFilter, Filter);

	// Start with in memory assets
	TSet<FName> PackagesToSkip = CachedEmptyPackages;

	// Prepare a set of each filter component for fast searching
	TSet<FName> FilterPackageNames(Filter.PackageNames);
	TSet<FName> FilterPackagePaths(Filter.PackagePaths);
	TSet<FName> FilterClassNames(Filter.ClassNames);
	TSet<FName> FilterObjectPaths(Filter.ObjectPaths);
	const int32 NumFilterPackageNames = FilterPackageNames.Num();
	const int32 NumFilterPackagePaths = FilterPackagePaths.Num();
	const int32 NumFilterClasses = FilterClassNames.Num();
	const int32 NumFilterObjectPaths = FilterObjectPaths.Num();

	if ( !Filter.bIncludeOnlyOnDiskAssets )
	{
		auto FilterInMemoryObjectLambda = [&](const UObject* Obj)
		{
			if ( Obj->IsAsset() )
			{
				UPackage* InMemoryPackage = Obj->GetOutermost();

				static const bool bUsingWorldAssets = FAssetRegistry::IsUsingWorldAssets();
				// Skip assets in map packages... unless we are showing world assets
				if ( InMemoryPackage->ContainsMap() && !bUsingWorldAssets )
				{
					return;
				}

				// Skip assets that were loaded for diffing
				if (InMemoryPackage->HasAnyPackageFlags(PKG_ForDiffing))
				{
					return;
				}

				// Package name
				const FName ObjectPath = FName(*Obj->GetPathName());
				const FName PackageName = InMemoryPackage->GetFName();

				PackagesToSkip.Add(PackageName);

				if ( NumFilterPackageNames && !FilterPackageNames.Contains(PackageName) )
				{
					return;
				}

				// Object Path
				if ( NumFilterObjectPaths && !FilterObjectPaths.Contains(ObjectPath) )
				{
					return;
				}

				// Package path
				const FName PackagePath = FName(*FPackageName::GetLongPackagePath(InMemoryPackage->GetName()));
				if ( NumFilterPackagePaths && !FilterPackagePaths.Contains(PackagePath) )
				{
					return;
				}

				// Tags and values
				TArray<UObject::FAssetRegistryTag> ObjectTags;
				Obj->GetAssetRegistryTags(ObjectTags);
				if (Filter.TagsAndValues.Num())
				{
					bool bMatch = false;
					for (auto FilterTagIt = Filter.TagsAndValues.CreateConstIterator(); FilterTagIt; ++FilterTagIt)
					{
						const FName Tag = FilterTagIt.Key();
						const FString& Value = FilterTagIt.Value();

						for (UObject::FAssetRegistryTag& AssetRegistryTag : ObjectTags)
						{
							if (AssetRegistryTag.Name == Tag)
							{
								if (AssetRegistryTag.Value == Value)
								{
									bMatch = true;
								}

								break;
							}
						}

						if (bMatch)
						{
							break;
						}
					}

					if (!bMatch)
					{
						return;
					}
				}

				// Find the group names
				FString GroupNamesStr;
				FString AssetNameStr;
				Obj->GetPathName(InMemoryPackage).Split(TEXT("."), &GroupNamesStr, &AssetNameStr, ESearchCase::CaseSensitive, ESearchDir::FromEnd);

				TMap<FName, FString> TagMap;
				for (UObject::FAssetRegistryTag& AssetRegistryTag : ObjectTags)
				{
					TagMap.Add(AssetRegistryTag.Name, AssetRegistryTag.Value);
				}

				// This asset is in memory and passes all filters
				FAssetData* AssetData = new (OutAssetData)FAssetData(PackageName, PackagePath, FName(*GroupNamesStr), Obj->GetFName(), Obj->GetClass()->GetFName(), TagMap, InMemoryPackage->GetChunkIDs(), InMemoryPackage->GetPackageFlags());
			}
		};

		// Iterate over all in-memory assets to find the ones that pass the filter components
		if(NumFilterClasses)
		{
			TArray<UObject*> InMemoryObjects;
			for (FName ClassName : FilterClassNames)
			{
				UClass* Class = FindObjectFast<UClass>(nullptr, ClassName, false, true, RF_NoFlags);
				if(Class != nullptr)
				{
					GetObjectsOfClass(Class, InMemoryObjects, false, RF_NoFlags);
				}
			}

			for (UObject* Object : InMemoryObjects)
			{
				FilterInMemoryObjectLambda(Object);
			}
		}
		else
		{
			for (FObjectIterator ObjIt; ObjIt; ++ObjIt)
			{
				FilterInMemoryObjectLambda(*ObjIt);
			}
		}
	}

	State.GetAssets(Filter, PackagesToSkip, OutAssetData);

	UE_LOG(LogAssetRegistry, Verbose, TEXT("GetAssets completed in %0.4f seconds"), FPlatformTime::Seconds() - GetAssetsStartTime);

	return true;
}

FAssetData FAssetRegistry::GetAssetByObjectPath( const FName ObjectPath, bool bIncludeOnlyOnDiskAssets ) const
{
	if (!bIncludeOnlyOnDiskAssets)
	{
		UObject* Asset = FindObject<UObject>( nullptr, *ObjectPath.ToString() );

		if ( Asset != nullptr )
		{
			return FAssetData( Asset );
		}
	}

	const FAssetData* FoundData = State.GetAssetByObjectPath(ObjectPath);

	if (FoundData)
	{
		return *FoundData;
	}
	return FAssetData();
}

bool FAssetRegistry::GetAllAssets(TArray<FAssetData>& OutAssetData, bool bIncludeOnlyOnDiskAssets) const
{
	TSet<FName> PackageNamesToSkip = CachedEmptyPackages;
	double GetAllAssetsStartTime = FPlatformTime::Seconds();

	// All in memory assets
	if (!bIncludeOnlyOnDiskAssets)
	{
		for (FObjectIterator ObjIt; ObjIt; ++ObjIt)
		{
			if (ObjIt->IsAsset())
			{
				const FAssetData& AssetData = OutAssetData[OutAssetData.Emplace(*ObjIt)];
				PackageNamesToSkip.Add(AssetData.PackageName);
			}
		}
	}

	State.GetAllAssets(PackageNamesToSkip, OutAssetData);

	UE_LOG(LogAssetRegistry, VeryVerbose, TEXT("GetAllAssets completed in %0.4f seconds"), FPlatformTime::Seconds() - GetAllAssetsStartTime);

	return true;
}

bool FAssetRegistry::GetDependencies(const FAssetIdentifier& AssetIdentifier, TArray<FAssetIdentifier>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType) const
{
	return State.GetDependencies(AssetIdentifier, OutDependencies, InDependencyType);
}

bool FAssetRegistry::GetDependencies(FName PackageName, TArray<FName>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType) const
{
	TArray<FAssetIdentifier> TempDependencies;

	if (GetDependencies(FAssetIdentifier(PackageName), TempDependencies, InDependencyType))
	{
		for (const FAssetIdentifier& AssetId : TempDependencies)
		{
			if (AssetId.PackageName != NAME_None)
			{
				OutDependencies.AddUnique(AssetId.PackageName);
			}
		}
		return true;
	}

	return false;
}

bool FAssetRegistry::GetReferencers(const FAssetIdentifier& AssetIdentifier, TArray<FAssetIdentifier>& OutReferencers, EAssetRegistryDependencyType::Type InReferenceType) const
{
	return State.GetReferencers(AssetIdentifier, OutReferencers, InReferenceType);
}

bool FAssetRegistry::GetReferencers(FName PackageName, TArray<FName>& OutReferencers, EAssetRegistryDependencyType::Type InReferenceType) const
{
	TArray<FAssetIdentifier> TempReferencers;

	if (GetReferencers(FAssetIdentifier(PackageName), TempReferencers, InReferenceType))
	{
		for (const FAssetIdentifier& AssetId : TempReferencers)
		{
			if (AssetId.PackageName != NAME_None)
			{
				OutReferencers.AddUnique(AssetId.PackageName);
			}
		}
		return true;
	}

	return false;
}

const FAssetPackageData* FAssetRegistry::GetAssetPackageData(FName PackageName) const
{
	return State.GetAssetPackageData(PackageName);
}

bool FAssetRegistry::GetAncestorClassNames(FName ClassName, TArray<FName>& OutAncestorClassNames) const
{
	// Start with the cached inheritance map
	TMap<FName, FName> InheritanceMap = CachedInheritanceMap;

	// And add all in-memory classes at request time
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		if ( !ClassIt->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists) )
		{
			if ( ClassIt->GetSuperClass() )
			{
				InheritanceMap.Add(ClassIt->GetFName(), ClassIt->GetSuperClass()->GetFName());
			}
			else
			{
				InheritanceMap.Add(ClassIt->GetFName(), NAME_None);
			}
		}
	}

	// Make sure the requested class is in the inheritance map
	if ( !InheritanceMap.Contains(ClassName) )
	{
		return false;
	}

	// Now follow the map pairs until we cant find any more parents
	FName* CurrentClassName = &ClassName;
	const uint32 MaxInheritanceDepth = 65536;
	uint32 CurrentInheritanceDepth = 0;
	while ( CurrentInheritanceDepth < MaxInheritanceDepth && CurrentClassName != nullptr )
	{
		CurrentClassName = InheritanceMap.Find(*CurrentClassName);

		if ( CurrentClassName )
		{
			if ( *CurrentClassName == NAME_None )
			{
				// No parent, we are at the root
				CurrentClassName = nullptr;
			}
			else
			{
				OutAncestorClassNames.Add(*CurrentClassName);
			}
		}
		CurrentInheritanceDepth++;
	}

	if ( CurrentInheritanceDepth == MaxInheritanceDepth )
	{
		UE_LOG(LogAssetRegistry, Error, TEXT("IsChildClass exceeded max inheritance depth. There is probably an infinite loop of parent classes."));
		return false;
	}
	else 
	{
		return true;
	}
}

void FAssetRegistry::GetDerivedClassNames(const TArray<FName>& ClassNames, const TSet<FName>& ExcludedClassNames, TSet<FName>& OutDerivedClassNames) const
{
	GetSubClasses(ClassNames, ExcludedClassNames, OutDerivedClassNames);
}

void FAssetRegistry::GetAllCachedPaths(TArray<FString>& OutPathList) const
{
	TSet<FName> PathList;
	CachedPathTree.GetAllPaths(PathList);
	
	OutPathList.Empty(PathList.Num());
	for (FName PathName : PathList)
	{
		OutPathList.Add(PathName.ToString());
	}
}

void FAssetRegistry::GetSubPaths(const FString& InBasePath, TArray<FString>& OutPathList, bool bInRecurse) const
{
	TSet<FName> PathList;
	CachedPathTree.GetSubPaths(FName(*InBasePath), PathList, bInRecurse);
	
	OutPathList.Empty(PathList.Num());
	for (FName PathName : PathList)
	{
		OutPathList.Add(PathName.ToString());
	}
}

void FAssetRegistry::RunAssetsThroughFilter(TArray<FAssetData>& AssetDataList, const FARFilter& Filter) const
{
	if ( !Filter.IsEmpty() )
	{
		TSet<FName> RequestedClassNames;
		if ( Filter.bRecursiveClasses && Filter.ClassNames.Num() > 0 )
		{
			// First assemble a full list of requested classes from the ClassTree
			// GetSubClasses includes the base classes
			GetSubClasses(Filter.ClassNames, Filter.RecursiveClassesExclusionSet, RequestedClassNames);
		}

		for ( int32 AssetDataIdx = AssetDataList.Num() - 1; AssetDataIdx >= 0; --AssetDataIdx )
		{
			const FAssetData& AssetData = AssetDataList[AssetDataIdx];

			// Package Names
			if ( Filter.PackageNames.Num() > 0 )
			{
				bool bPassesPackageNames = false;
				for ( int32 NameIdx = 0; NameIdx < Filter.PackageNames.Num(); ++NameIdx )
				{
					if (Filter.PackageNames[NameIdx] == AssetData.PackageName)
					{
						bPassesPackageNames = true;
						break;
					}
				}

				if ( !bPassesPackageNames )
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}

			// Package Paths
			if ( Filter.PackagePaths.Num() > 0 )
			{
				bool bPassesPackagePaths = false;
				if ( Filter.bRecursivePaths )
				{
					FString AssetPackagePath = AssetData.PackagePath.ToString();
					for ( int32 PathIdx = 0; PathIdx < Filter.PackagePaths.Num(); ++PathIdx )
					{
						const FString Path = Filter.PackagePaths[PathIdx].ToString();
						if ( AssetPackagePath.StartsWith(Path) )
						{
							// Only match the exact path or a path that starts with the target path followed by a slash
							if ( Path.Len() == 1 || Path.Len() == AssetPackagePath.Len() || AssetPackagePath.Mid(Path.Len(), 1) == TEXT("/") )
							{
								bPassesPackagePaths = true;
								break;
							}
						}
					}
				}
				else
				{
					// Non-recursive. Just request data for each requested path.
					for ( int32 PathIdx = 0; PathIdx < Filter.PackagePaths.Num(); ++PathIdx )
					{
						if (Filter.PackagePaths[PathIdx] == AssetData.PackagePath)
						{
							bPassesPackagePaths = true;
							break;
						}
					}
				}

				if ( !bPassesPackagePaths )
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}

			// ObjectPaths
			if ( Filter.ObjectPaths.Num() > 0 )
			{
				bool bPassesObjectPaths = Filter.ObjectPaths.Contains(AssetData.ObjectPath);

				if ( !bPassesObjectPaths )
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}

			// Classes
			if ( Filter.ClassNames.Num() > 0 )
			{
				bool bPassesClasses = false;
				if ( Filter.bRecursiveClasses )
				{
					// Now check against each discovered class
					for (FName ClassName : RequestedClassNames)
					{
						if (ClassName == AssetData.AssetClass)
						{
							bPassesClasses = true;
							break;
						}
					}
				}
				else
				{
					// Non-recursive. Just request data for each requested classes.
					for ( int32 ClassIdx = 0; ClassIdx < Filter.ClassNames.Num(); ++ClassIdx )
					{
						if (Filter.ClassNames[ClassIdx] == AssetData.AssetClass)
						{
							bPassesClasses = true;
							break;
						}
					}
				}

				if ( !bPassesClasses )
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}

			// Tags and values
			if ( Filter.TagsAndValues.Num() > 0 )
			{
				bool bPassesTags = false;
				for (auto FilterTagIt = Filter.TagsAndValues.CreateConstIterator(); FilterTagIt; ++FilterTagIt)
				{
					const FString* Value = AssetData.TagsAndValues.Find(FilterTagIt.Key());

					if ( Value != nullptr && (*Value) == FilterTagIt.Value() )
					{
						bPassesTags = true;
						break;
					}
				}

				if ( !bPassesTags )
				{
					AssetDataList.RemoveAt(AssetDataIdx);
					continue;
				}
			}
		}
	}
}

void FAssetRegistry::ExpandRecursiveFilter(const FARFilter& InFilter, FARFilter& ExpandedFilter) const
{
	 TSet<FName> FilterPackagePaths;
	 TSet<FName> FilterClassNames;
	 const int32 NumFilterPackagePaths = InFilter.PackagePaths.Num();
	 const int32 NumFilterClasses = InFilter.ClassNames.Num();

	 ExpandedFilter = InFilter;

	 for (int32 PathIdx = 0; PathIdx < NumFilterPackagePaths; ++PathIdx)
	 {
		 FilterPackagePaths.Add(InFilter.PackagePaths[PathIdx]);
	 }

	 if (InFilter.bRecursivePaths)
	 {
		 // Add subpaths to all the input paths to the list
		 for (int32 PathIdx = 0; PathIdx < NumFilterPackagePaths; ++PathIdx)
		 {
			 CachedPathTree.GetSubPaths(InFilter.PackagePaths[PathIdx], FilterPackagePaths);
		 }
	 }

	 ExpandedFilter.bRecursivePaths = false;
	 ExpandedFilter.PackagePaths = FilterPackagePaths.Array();

	 if (InFilter.bRecursiveClasses)
	 {
		 // GetSubClasses includes the base classes
		 GetSubClasses(InFilter.ClassNames, InFilter.RecursiveClassesExclusionSet, FilterClassNames);
	 }
	 else
	 {
		 for (int32 ClassIdx = 0; ClassIdx < NumFilterClasses; ++ClassIdx)
		 {
			 FilterClassNames.Add(InFilter.ClassNames[ClassIdx]);
		 }
	 }

	 ExpandedFilter.ClassNames = FilterClassNames.Array();
	 ExpandedFilter.bRecursiveClasses = false;
	 ExpandedFilter.RecursiveClassesExclusionSet.Empty();
}

EAssetAvailability::Type FAssetRegistry::GetAssetAvailability(const FAssetData& AssetData) const
{
	IPlatformChunkInstall* ChunkInstall = FPlatformMisc::GetPlatformChunkInstall();

	EChunkLocation::Type BestLocation = EChunkLocation::DoesNotExist;

	// check all chunks to see which has the best locality
	for (int32 ChunkId : AssetData.ChunkIDs)
	{
		EChunkLocation::Type ChunkLocation = ChunkInstall->GetChunkLocation(ChunkId);

		// if we find one in the best location, early out
		if (ChunkLocation == EChunkLocation::BestLocation)
		{
			BestLocation = ChunkLocation;
			break;
		}

		if (ChunkLocation > BestLocation)
		{
			BestLocation = ChunkLocation;
		}
	}

	switch (BestLocation)
	{
	case EChunkLocation::LocalFast:
		return EAssetAvailability::LocalFast;
	case EChunkLocation::LocalSlow:
		return EAssetAvailability::LocalSlow;
	case EChunkLocation::NotAvailable:
		return EAssetAvailability::NotAvailable;
	case EChunkLocation::DoesNotExist:
		return EAssetAvailability::DoesNotExist;
	default:
		check(0);
		return EAssetAvailability::LocalFast;
	}
}

float FAssetRegistry::GetAssetAvailabilityProgress(const FAssetData& AssetData, EAssetAvailabilityProgressReportingType::Type ReportType) const
{
	IPlatformChunkInstall* ChunkInstall = FPlatformMisc::GetPlatformChunkInstall();
	EChunkProgressReportingType::Type ChunkReportType = GetChunkAvailabilityProgressType(ReportType);

	bool IsPercentageComplete = (ChunkReportType == EChunkProgressReportingType::PercentageComplete) ? true : false;
	check (ReportType == EAssetAvailabilityProgressReportingType::PercentageComplete || ReportType == EAssetAvailabilityProgressReportingType::ETA);

	float BestProgress = MAX_FLT;	

	// check all chunks to see which has the best time remaining
	for (int32 ChunkId : AssetData.ChunkIDs)
	{
		float Progress = ChunkInstall->GetChunkProgress(ChunkId, ChunkReportType);

		// need to flip percentage completes for the comparison
		if (IsPercentageComplete)
		{
			Progress = 100.0f - Progress;
		}

		if( Progress <= 0.0f )
		{
			BestProgress = 0.0f;
			break;
		}

		if (Progress < BestProgress)
		{
			BestProgress = Progress;
		}
	}

	// unflip percentage completes
	if (IsPercentageComplete)
	{
		BestProgress = 100.0f - BestProgress;
	}	
	return BestProgress;
}

bool FAssetRegistry::GetAssetAvailabilityProgressTypeSupported(EAssetAvailabilityProgressReportingType::Type ReportType) const
{
	IPlatformChunkInstall* ChunkInstall = FPlatformMisc::GetPlatformChunkInstall();	
	return ChunkInstall->GetProgressReportingTypeSupported(GetChunkAvailabilityProgressType(ReportType));
}

void FAssetRegistry::PrioritizeAssetInstall(const FAssetData& AssetData) const
{
	IPlatformChunkInstall* ChunkInstall = FPlatformMisc::GetPlatformChunkInstall();

	if (AssetData.ChunkIDs.Num() == 0)
	{
		return;
	}

	ChunkInstall->PrioritizeChunk(AssetData.ChunkIDs[0], EChunkPriority::Immediate);
}

bool FAssetRegistry::AddPath(const FString& PathToAdd)
{
	return AddAssetPath(FName(*PathToAdd));
}

bool FAssetRegistry::RemovePath(const FString& PathToRemove)
{
	return RemoveAssetPath(FName(*PathToRemove));
}

void FAssetRegistry::ScanPathsSynchronous(const TArray<FString>& InPaths, bool bForceRescan)
{
	ScanPathsAndFilesSynchronous(InPaths, TArray<FString>(), bForceRescan, EAssetDataCacheMode::UseModularCache);
}

void FAssetRegistry::ScanFilesSynchronous(const TArray<FString>& InFilePaths, bool bForceRescan) 
{
	ScanPathsAndFilesSynchronous(TArray<FString>(), InFilePaths, bForceRescan, EAssetDataCacheMode::UseModularCache);
}

void FAssetRegistry::PrioritizeSearchPath(const FString& PathToPrioritize)
{
	// Prioritize the background search
	if (BackgroundAssetSearch.IsValid())
	{
		BackgroundAssetSearch->PrioritizeSearchPath(PathToPrioritize);
	}

	// Also prioritize the queue of background search results
	{
		// Swap all priority files to the top of the list
		int32 LowestNonPriorityFileIdx = 0;
		for (int32 ResultIdx = 0; ResultIdx < BackgroundAssetResults.Num(); ++ResultIdx)
		{
			FAssetData* BackgroundAssetResult = BackgroundAssetResults[ResultIdx];
			if (BackgroundAssetResult && BackgroundAssetResult->PackagePath.ToString().StartsWith(PathToPrioritize))
			{
				BackgroundAssetResults.Swap(ResultIdx, LowestNonPriorityFileIdx);
				LowestNonPriorityFileIdx++;
			}
		}
	}
}

void FAssetRegistry::AssetCreated(UObject* NewAsset)
{
	if ( ensure(NewAsset) && NewAsset->IsAsset() )
	{
		// Add the newly created object to the package file cache because its filename can already be
		// determined by its long package name.
		// @todo AssetRegistry We are assuming it will be saved in a single asset package.
		UPackage* NewPackage = NewAsset->GetOutermost();

		// Mark this package as newly created.
		NewPackage->SetPackageFlags(PKG_NewlyCreated);

		const FString NewPackageName = NewPackage->GetName();
		const FString Filename = FPackageName::LongPackageNameToFilename(NewPackageName, FPackageName::GetAssetPackageExtension());

		// This package not empty, in case it ever was
		RemoveEmptyPackage(NewPackage->GetFName());

		// Add the path to the Path Tree, in case it wasn't already there
		AddAssetPath(*FPackageName::GetLongPackagePath(NewPackageName));

		// Let subscribers know that the new asset was added to the registry
		AssetAddedEvent.Broadcast(FAssetData(NewAsset));

		// Notify listeners that an asset was just created
		InMemoryAssetCreatedEvent.Broadcast(NewAsset);
	}
}

void FAssetRegistry::AssetDeleted(UObject* DeletedAsset)
{
	if ( ensure(DeletedAsset) && DeletedAsset->IsAsset() )
	{
		UPackage* DeletedObjectPackage = DeletedAsset->GetOutermost();
		if ( DeletedObjectPackage != nullptr )
		{
			const FString PackageName = DeletedObjectPackage->GetName();

			// Deleting the last asset in a package causes the package to be garbage collected.
			// If the UPackage object is GCed, it will be considered 'Unloaded' which will cause it to
			// be fully loaded from disk when save is invoked.
			// We want to keep the package around so we can save it empty or delete the file.
			if ( UPackage::IsEmptyPackage( DeletedObjectPackage, DeletedAsset ) )
			{
				AddEmptyPackage( DeletedObjectPackage->GetFName() );

				// If there is a package metadata object, clear the standalone flag so the package can be truly emptied upon GC
				if ( UMetaData* MetaData = DeletedObjectPackage->GetMetaData() )
				{
					MetaData->ClearFlags( RF_Standalone );
				}
			}
		}

		// Let subscribers know that the asset was removed from the registry
		AssetRemovedEvent.Broadcast(FAssetData(DeletedAsset));

		// Notify listeners that an in-memory asset was just deleted
		InMemoryAssetDeletedEvent.Broadcast(DeletedAsset);
	}
}

void FAssetRegistry::AssetRenamed(const UObject* RenamedAsset, const FString& OldObjectPath)
{
	if ( ensure(RenamedAsset) && RenamedAsset->IsAsset() )
	{
		// Add the renamed object to the package file cache because its filename can already be
		// determined by its long package name.
		// @todo AssetRegistry We are assuming it will be saved in a single asset package.
		UPackage* NewPackage = RenamedAsset->GetOutermost();
		const FString NewPackageName = NewPackage->GetName();
		const FString Filename = FPackageName::LongPackageNameToFilename(NewPackageName, FPackageName::GetAssetPackageExtension());

		RemoveEmptyPackage(NewPackage->GetFName());

		// We want to keep track of empty packages so we can properly merge cached assets with in-memory assets
		FString OldPackageName;
		FString OldAssetName;
		if ( OldObjectPath.Split(TEXT("."), &OldPackageName, &OldAssetName) )
		{
			UPackage* OldPackage = FindPackage(nullptr, *OldPackageName);

			if ( UPackage::IsEmptyPackage(OldPackage) )
			{
				AddEmptyPackage(OldPackage->GetFName());
			}
		}

		// Add the path to the Path Tree, in case it wasn't already there
		AddAssetPath(*FPackageName::GetLongPackagePath(NewPackageName));

		AssetRenamedEvent.Broadcast(FAssetData(RenamedAsset), OldObjectPath);
	}
}

void FAssetRegistry::PackageDeleted(UPackage* DeletedPackage)
{
	if (ensure(DeletedPackage))
	{
		RemovePackageData(*DeletedPackage->GetName());
	}
}

bool FAssetRegistry::IsLoadingAssets() const
{
	return !bInitialSearchCompleted;
}

IAssetRegistry::FAssetEditSearchableNameDelegate& FAssetRegistry::OnEditSearchableName(FName PackageName, FName ObjectName)
{
	return EditSearchableNameDelegates.FindOrAdd(FAssetIdentifier(PackageName, ObjectName));
}

bool FAssetRegistry::EditSearchableName(const FAssetIdentifier& SearchableName)
{
	for (TPair<FAssetIdentifier, FAssetEditSearchableNameDelegate> MapPair : EditSearchableNameDelegates)
	{
		if (MapPair.Key.PackageName == SearchableName.PackageName &&
			(MapPair.Key.ObjectName == SearchableName.ObjectName || MapPair.Key.ObjectName.IsNone()) &&
			(MapPair.Key.ValueName == SearchableName.ValueName || MapPair.Key.ValueName.IsNone()))
		{
			// Try this callback
			if (MapPair.Value.IsBound())
			{
				if (MapPair.Value.Execute(SearchableName))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void FAssetRegistry::Tick(float DeltaTime)
{
	double TickStartTime = FPlatformTime::Seconds();

	// Gather results from the background search
	bool bIsSearching = false;
	TArray<double> SearchTimes;
	int32 NumFilesToSearch = 0;
	int32 NumPathsToSearch = 0;
	bool bIsDiscoveringFiles = false;
	if ( BackgroundAssetSearch.IsValid() )
	{
		bIsSearching = BackgroundAssetSearch->GetAndTrimSearchResults(BackgroundAssetResults, BackgroundPathResults, BackgroundDependencyResults, BackgroundCookedPackageNamesWithoutAssetDataResults, SearchTimes, NumFilesToSearch, NumPathsToSearch, bIsDiscoveringFiles);
	}

	// Report the search times
	for (int32 SearchTimeIdx = 0; SearchTimeIdx < SearchTimes.Num(); ++SearchTimeIdx)
	{
		UE_LOG(LogAssetRegistry, Verbose, TEXT("### Background search completed in %0.4f seconds"), SearchTimes[SearchTimeIdx]);
	}

	// Add discovered paths
	if ( BackgroundPathResults.Num() )
	{
		PathDataGathered(TickStartTime, BackgroundPathResults);
	}

	// Process the asset results
	const bool bHadAssetsToProcess = BackgroundAssetResults.Num() > 0 || BackgroundDependencyResults.Num() > 0;
	if ( BackgroundAssetResults.Num() )
	{
		// Mark the first amortize time
		if ( AmortizeStartTime == 0 )
		{
			AmortizeStartTime = FPlatformTime::Seconds();
		}

		AssetSearchDataGathered(TickStartTime, BackgroundAssetResults);

		if ( BackgroundAssetResults.Num() == 0 )
		{
			TotalAmortizeTime += FPlatformTime::Seconds() - AmortizeStartTime;
			AmortizeStartTime = 0;
		}
	}

	// Add dependencies
	if ( BackgroundDependencyResults.Num() )
	{
		DependencyDataGathered(TickStartTime, BackgroundDependencyResults);
	}

	// Load cooked packages that do not have asset data
	if ( BackgroundCookedPackageNamesWithoutAssetDataResults.Num() )
	{
		CookedPackageNamesWithoutAssetDataGathered(TickStartTime, BackgroundCookedPackageNamesWithoutAssetDataResults);
	}

	// Notify the status change
	if (bIsSearching || bHadAssetsToProcess)
	{
		const FFileLoadProgressUpdateData ProgressUpdateData(
			State.CachedAssetsByObjectPath.Num() + BackgroundAssetResults.Num() + BackgroundDependencyResults.Num() + NumFilesToSearch,	// NumTotalAssets
			State.CachedAssetsByObjectPath.Num() - BackgroundDependencyResults.Num(),					// NumAssetsProcessedByAssetRegistry
			NumFilesToSearch + BackgroundDependencyResults.Num(),								// NumAssetsPendingDataLoad
			bIsDiscoveringFiles																	// bIsDiscoveringAssetFiles
			);
		FileLoadProgressUpdatedEvent.Broadcast(ProgressUpdateData);
	}

	// If completing an initial search, refresh the content browser
	if ( NumFilesToSearch == 0 && NumPathsToSearch == 0 && !bIsSearching && BackgroundPathResults.Num() == 0 && BackgroundAssetResults.Num() == 0 && BackgroundDependencyResults.Num() == 0 && BackgroundCookedPackageNamesWithoutAssetDataResults.Num() == 0 )
	{
		if ( !bInitialSearchCompleted )
		{
			UE_LOG(LogAssetRegistry, Verbose, TEXT("### Time spent amortizing search results: %0.4f seconds"), TotalAmortizeTime);
			UE_LOG(LogAssetRegistry, Log, TEXT("Asset discovery search completed in %0.4f seconds"), FPlatformTime::Seconds() - FullSearchStartTime);

			bInitialSearchCompleted = true;

			FileLoadedEvent.Broadcast();
		}
#if WITH_EDITOR
		else if (bUpdateDiskCacheAfterLoad)
		{
			ProcessLoadedAssetsToUpdateCache(TickStartTime);
		}
#endif
	}
}


bool FAssetRegistry::IsUsingWorldAssets()
{
	return !FParse::Param(FCommandLine::Get(), TEXT("DisableWorldAssets"));
}

void FAssetRegistry::Serialize(FArchive& Ar)
{
	State.Serialize(Ar, SerializationOptions);

	if (Ar.IsLoading())
	{
		// Add paths to cache
		for (const TPair<FName, FAssetData*>& AssetDataPair : State.CachedAssetsByObjectPath)
		{
			const FAssetData* AssetData = AssetDataPair.Value;

			if (AssetData != nullptr)
			{
				AddAssetPath(AssetData->PackagePath);

				// Populate the class map if adding blueprint
				if (ClassGeneratorNames.Contains(AssetData->AssetClass))
				{
					const FString GeneratedClass = AssetData->GetTagValueRef<FString>("GeneratedClass");
					const FString ParentClass = AssetData->GetTagValueRef<FString>("ParentClass");
					if (!GeneratedClass.IsEmpty() && !ParentClass.IsEmpty())
					{
						const FName GeneratedClassFName = *ExportTextPathToObjectName(GeneratedClass);
						const FName ParentClassFName = *ExportTextPathToObjectName(ParentClass);
						CachedInheritanceMap.Add(GeneratedClassFName, ParentClassFName);
					}
				}
			}
		}
	}
}

void FAssetRegistry::LoadPackageRegistryData(FArchive& Ar, TArray<FAssetData*> &AssetDataList ) const
{
	
	FPackageReader Reader;
	Reader.OpenPackageFile(&Ar);

	Reader.ReadAssetRegistryData(AssetDataList);

	Reader.ReadAssetDataFromThumbnailCache(AssetDataList);

	TArray<FString> CookedPackageNamesWithoutAssetDataGathered;
	Reader.ReadAssetRegistryDataIfCookedPackage(AssetDataList, CookedPackageNamesWithoutAssetDataGathered);

	//bool ReadDependencyData(FPackageDependencyData& OutDependencyData);
}

void FAssetRegistry::SaveRegistryData(FArchive& Ar, TMap<FName, FAssetData*>& Data, TArray<FName>* InMaps /* = nullptr */)
{
	FAssetRegistryState TempState;
	InitializeTemporaryAssetRegistryState(TempState, SerializationOptions, Data);

	TempState.Serialize(Ar, SerializationOptions);
}

void FAssetRegistry::LoadRegistryData(FArchive& Ar, TMap<FName, FAssetData*>& Data)
{
	FAssetRegistryState TempState;

	TempState.Serialize(Ar, SerializationOptions);

	if (Ar.IsLoading())
	{
		for (const TPair<FName, FAssetData*>& AssetDataPair : State.CachedAssetsByObjectPath)
		{
			const FAssetData* AssetData = AssetDataPair.Value;

			if (AssetData != nullptr)
			{
				FAssetData *NewAssetData = new FAssetData(*AssetData);

				Data.Add(NewAssetData->PackageName, NewAssetData);
			}
		}
	}
}

void FAssetRegistry::InitializeTemporaryAssetRegistryState(FAssetRegistryState& OutState, const FAssetRegistrySerializationOptions& Options, const TMap<FName, FAssetData*>& OverrideData) const
{
	const TMap<FName, FAssetData*>& DataToUse = OverrideData.Num() > 0 ? OverrideData : State.CachedAssetsByObjectPath;

	OutState.InitializeFromExisting(DataToUse, State.CachedDependsNodes, State.CachedPackageData, Options);
}

void FAssetRegistry::ScanPathsAndFilesSynchronous(const TArray<FString>& InPaths, const TArray<FString>& InSpecificFiles, bool bForceRescan, EAssetDataCacheMode AssetDataCacheMode)
{
	ScanPathsAndFilesSynchronous(InPaths, InSpecificFiles, bForceRescan, AssetDataCacheMode, nullptr, nullptr);
}

void FAssetRegistry::ScanPathsAndFilesSynchronous(const TArray<FString>& InPaths, const TArray<FString>& InSpecificFiles, bool bForceRescan, EAssetDataCacheMode AssetDataCacheMode, TArray<FName>* OutFoundAssets, TArray<FName>* OutFoundPaths)
{
	const double SearchStartTime = FPlatformTime::Seconds();

	// Only scan paths that were not previously synchronously scanned, unless we were asked to force rescan.
	TArray<FString> PathsToScan;
	TArray<FString> FilesToScan;
	for (const FString& Path : InPaths)
	{
		if (bForceRescan || !SynchronouslyScannedPathsAndFiles.Contains(Path))
		{
			PathsToScan.Add(Path);
			SynchronouslyScannedPathsAndFiles.Add(Path);
		}
	}

	for (const FString& SpecificFile : InSpecificFiles)
	{
		if (bForceRescan || !SynchronouslyScannedPathsAndFiles.Contains(SpecificFile))
		{
			FilesToScan.Add(SpecificFile);
			SynchronouslyScannedPathsAndFiles.Add(SpecificFile);
		}
	}

	if ( PathsToScan.Num() > 0 || FilesToScan.Num() > 0 )
	{
		// Start the sync asset search
		FAssetDataGatherer AssetSearch(PathsToScan, FilesToScan, /*bSynchronous=*/true, AssetDataCacheMode);

		// Get the search results
		TArray<FAssetData*> AssetResults;
		TArray<FString> PathResults;
		TArray<FPackageDependencyData> DependencyResults;
		TArray<FString> CookedPackageNamesWithoutAssetDataResults;
		TArray<double> SearchTimes;
		int32 NumFilesToSearch = 0;
		int32 NumPathsToSearch = 0;
		bool bIsDiscoveringFiles = false;
		AssetSearch.GetAndTrimSearchResults(AssetResults, PathResults, DependencyResults, CookedPackageNamesWithoutAssetDataResults, SearchTimes, NumFilesToSearch, NumPathsToSearch, bIsDiscoveringFiles);

		if (OutFoundAssets)
		{
			OutFoundAssets->Reserve(OutFoundAssets->Num() + AssetResults.Num());
			for (const FAssetData* AssetData : AssetResults)
			{
				OutFoundAssets->Add(AssetData->ObjectPath);
			}
		}

		if (OutFoundPaths)
		{
			OutFoundPaths->Reserve(OutFoundPaths->Num() + PathResults.Num());
			for (const FString& Path : PathResults)
			{
				OutFoundPaths->Add(*Path);
			}
		}

		// Cache the search results
		const int32 NumResults = AssetResults.Num();
		AssetSearchDataGathered(-1, AssetResults);
		PathDataGathered(-1, PathResults);
		DependencyDataGathered(-1, DependencyResults);
		CookedPackageNamesWithoutAssetDataGathered(-1, CookedPackageNamesWithoutAssetDataResults);

#if WITH_EDITOR
		if (bUpdateDiskCacheAfterLoad && bInitialSearchCompleted)
		{
			ProcessLoadedAssetsToUpdateCache(-1);
		}
#endif

		// Log stats
		TArray<FString> LogPathsAndFilenames = PathsToScan;
		LogPathsAndFilenames.Append(FilesToScan);

		const FString& Path = LogPathsAndFilenames[0];
		FString PathsString;
		if (LogPathsAndFilenames.Num() > 1)
		{
			PathsString = FString::Printf(TEXT("'%s' and %d other paths/filenames"), *Path, LogPathsAndFilenames.Num() - 1);
		}
		else
		{
			PathsString = FString::Printf(TEXT("'%s'"), *Path);
		}

		UE_LOG(LogAssetRegistry, Verbose, TEXT("ScanPathsSynchronous completed scanning %s to find %d assets in %0.4f seconds"), *PathsString, NumResults, FPlatformTime::Seconds() - SearchStartTime);
	}
}

void FAssetRegistry::AssetSearchDataGathered(const double TickStartTime, TArray<FAssetData*>& AssetResults)
{
	const bool bFlushFullBuffer = TickStartTime < 0;
	TSet<FName> ModifiedPaths;

	// Add the found assets
	int32 AssetIndex = 0;
	for (AssetIndex = 0; AssetIndex < AssetResults.Num(); ++AssetIndex)
	{
		FAssetData*& BackgroundResult = AssetResults[AssetIndex];

		CA_ASSUME(BackgroundResult);

		// Try to update any asset data that may already exist
		FAssetData* AssetData = nullptr;
		FAssetData** AssetDataPtr = State.CachedAssetsByObjectPath.Find(BackgroundResult->ObjectPath);
		if (AssetDataPtr != nullptr)
		{
			AssetData = *AssetDataPtr;
		}

		const FName PackagePath = BackgroundResult->PackagePath;
		if ( AssetData != nullptr )
		{
			// The asset exists in the cache, update it
			UpdateAssetData(AssetData, *BackgroundResult);

			// Delete the result that was originally created by an FPackageReader
			delete BackgroundResult;
			BackgroundResult = nullptr;
		}
		else
		{
			// The asset isn't in the cache yet, add it and notify subscribers
			AddAssetData(BackgroundResult);
		}

		// Populate the path tree
		AddAssetPath(PackagePath);

		// Check to see if we have run out of time in this tick
		if ( !bFlushFullBuffer && (FPlatformTime::Seconds() - TickStartTime) > MaxSecondsPerFrame)
		{
			// Increment the index to properly trim the buffer below
			++AssetIndex;
			break;
		}
	}

	// Trim the results array
	if (AssetIndex > 0)
	{
		AssetResults.RemoveAt(0, AssetIndex);
	}
}

void FAssetRegistry::PathDataGathered(const double TickStartTime, TArray<FString>& PathResults)
{
	const bool bFlushFullBuffer = TickStartTime < 0;
	int32 ResultIdx = 0;
	for (ResultIdx = 0; ResultIdx < PathResults.Num(); ++ResultIdx)
	{
		const FString& Path = PathResults[ResultIdx];
		AddAssetPath(FName(*Path));
		
		// Check to see if we have run out of time in this tick
		if ( !bFlushFullBuffer && (FPlatformTime::Seconds() - TickStartTime) > MaxSecondsPerFrame)
		{
			// Increment the index to properly trim the buffer below
			++ResultIdx;
			break;
		}
	}

	// Trim the results array
	PathResults.RemoveAt(0, ResultIdx);
}

void FAssetRegistry::DependencyDataGathered(const double TickStartTime, TArray<FPackageDependencyData>& DependsResults)
{
	const bool bFlushFullBuffer = TickStartTime < 0;

	int32 ResultIdx = 0;
	for (ResultIdx = 0; ResultIdx < DependsResults.Num(); ++ResultIdx)
	{
		FPackageDependencyData& Result = DependsResults[ResultIdx];

		// Update package data
		FAssetPackageData* PackageData = State.CreateOrGetAssetPackageData(Result.PackageName);
		*PackageData = Result.PackageData;

		FDependsNode* Node = State.CreateOrFindDependsNode(Result.PackageName);

		// We will populate the node dependencies below. Empty the set here in case this file was already read
		// Also remove references to all existing dependencies, those will be also repopulated below
		Node->IterateOverDependencies([Node](FDependsNode* InDependency, EAssetRegistryDependencyType::Type InDependencyType)
		{
			InDependency->RemoveReferencer(Node);
		});

		Node->ClearDependencies();

		// Don't bother registering dependencies on these packages, every package in the game will depend on them
		static TArray<FName> ScriptPackagesToSkip = TArray<FName>{ TEXT("/Script/CoreUObject"), TEXT("/Script/Engine"), TEXT("/Script/BlueprintGraph"), TEXT("/Script/UnrealEd") };

		// Determine the new package dependencies
		TMap<FName, EAssetRegistryDependencyType::Type> PackageDependencies;
		for (int32 ImportIdx = 0; ImportIdx < Result.ImportMap.Num(); ++ImportIdx)
		{
			const FName AssetReference = Result.GetImportPackageName(ImportIdx);

			// Should we skip this because it's too common?
			if (ScriptPackagesToSkip.Contains(AssetReference))
			{
				continue;
			}

			// Already processed?
			if (PackageDependencies.Contains(AssetReference))
			{
				continue;
			}

			PackageDependencies.Add(AssetReference, EAssetRegistryDependencyType::Hard);
		}

		for (const FString& StringAssetReference : Result.StringAssetReferencesMap)
		{
			// Possibly resolve ini:name references before adding to dependency list
			const FString* IniFilename = GetIniFilenameFromObjectsReference(StringAssetReference);
			const FName AssetReference = IniFilename ? *ResolveIniObjectsReference(StringAssetReference, IniFilename) : *StringAssetReference;

			// Already processed?
			if (PackageDependencies.Contains(AssetReference))
			{
				continue;
			}

			if (FPackageName::IsShortPackageName(AssetReference))
			{
				UE_LOG(LogAssetRegistry, Warning, TEXT("Package with string asset reference with short asset path: %s. This is unsupported, can couse errors and be slow on loading. Please resave the package to fix this."), *Result.PackageName.ToString());
			}

			PackageDependencies.Add(AssetReference, EAssetRegistryDependencyType::Soft);
		}

		for (const TPair<FPackageIndex, TArray<FName>>& SearchableNameList : Result.SearchableNamesMap)
		{
			FName ObjectName;
			FName PackageName;

			// Find object and package name from linker
			for (FPackageIndex LinkerIndex = SearchableNameList.Key; !LinkerIndex.IsNull();)
			{
				if (LinkerIndex.IsExport())
				{
					// Package name has to be this package, take a guess at object name
					PackageName = Result.PackageName;
					ObjectName = FName(*FPackageName::GetLongPackageAssetName(Result.PackageName.ToString()));

					break;
				}

				FObjectResource& Resource = Result.ImpExp(LinkerIndex);
				LinkerIndex = Resource.OuterIndex;
				if (ObjectName.IsNone() && !LinkerIndex.IsNull())
				{
					ObjectName = Resource.ObjectName;
				}
				else if (LinkerIndex.IsNull())
				{
					PackageName = Resource.ObjectName;
				}
			}

			for (FName NameReference : SearchableNameList.Value)
			{
				FAssetIdentifier AssetId = FAssetIdentifier(PackageName, ObjectName, NameReference);

				// Add node for all name references
				FDependsNode* DependsNode = State.CreateOrFindDependsNode(AssetId);

				if (DependsNode != nullptr)
				{
					Node->AddDependency(DependsNode, EAssetRegistryDependencyType::SearchableName);
					DependsNode->AddReferencer(Node);
				}
			}	
		}

		// Doubly-link all new dependencies for this package
		for (auto NewDependsIt : PackageDependencies)
		{
			FDependsNode* DependsNode = State.CreateOrFindDependsNode(NewDependsIt.Key);
			
			if (DependsNode != nullptr)
			{
				Node->AddDependency(DependsNode, NewDependsIt.Value);
				DependsNode->AddReferencer(Node);
			}
		}

		// Check to see if we have run out of time in this tick
		if ( !bFlushFullBuffer && (FPlatformTime::Seconds() - TickStartTime) > MaxSecondsPerFrame)
		{
			// Increment the index to properly trim the buffer below
			++ResultIdx;
			break;
		}
	}

	// Trim the results array
	DependsResults.RemoveAt(0, ResultIdx);
}

void FAssetRegistry::CookedPackageNamesWithoutAssetDataGathered(const double TickStartTime, TArray<FString>& CookedPackageNamesWithoutAssetDataResults)
{
	const bool bFlushFullBuffer = TickStartTime < 0;

	// Add the found assets
	int32 PackageNameIndex = 0;
	for (PackageNameIndex = 0; PackageNameIndex < CookedPackageNamesWithoutAssetDataResults.Num(); ++PackageNameIndex)
	{
		// If this data is cooked and it we couldn't find any asset in its export table then try load the entire package 
		const FString& BackgroundResult = CookedPackageNamesWithoutAssetDataResults[PackageNameIndex];
		LoadPackage(nullptr, *BackgroundResult, 0);

		// Check to see if we have run out of time in this tick
		if (!bFlushFullBuffer && (FPlatformTime::Seconds() - TickStartTime) > MaxSecondsPerFrame)
		{
			// Increment the index to properly trim the buffer below
			++PackageNameIndex;
			break;
		}
	}

	// Trim the results array
	if (PackageNameIndex > 0)
	{
		CookedPackageNamesWithoutAssetDataResults.RemoveAt(0, PackageNameIndex);
	}
}

void FAssetRegistry::AddEmptyPackage(FName PackageName)
{
	CachedEmptyPackages.Add(PackageName);
}

bool FAssetRegistry::RemoveEmptyPackage(FName PackageName)
{
	return CachedEmptyPackages.Remove( PackageName ) > 0;
}

bool FAssetRegistry::AddAssetPath(FName PathToAdd)
{
	if (CachedPathTree.CachePath(PathToAdd))
	{		
		PathAddedEvent.Broadcast(PathToAdd.ToString());
		return true;
	}

	return false;
}

bool FAssetRegistry::RemoveAssetPath(FName PathToRemove, bool bEvenIfAssetsStillExist)
{
	if ( !bEvenIfAssetsStillExist )
	{
		// Check if there were assets in the specified folder. You can not remove paths that still contain assets
		TArray<FAssetData> AssetsInPath;
		GetAssetsByPath(PathToRemove, AssetsInPath, true);
		if ( AssetsInPath.Num() > 0 )
		{
			// At least one asset still exists in the path. Fail the remove.
			return false;
		}
	}

	if (CachedPathTree.RemovePath(PathToRemove))
	{
		PathRemovedEvent.Broadcast(PathToRemove.ToString());
		return true;
	}
	else
	{
		// The folder did not exist in the tree, fail the remove
		return false;
	}
}

FString FAssetRegistry::ExportTextPathToObjectName(const FString& InExportTextPath) const
{
	const FString ObjectPath = FPackageName::ExportTextPathToObjectPath(InExportTextPath);
	return FPackageName::ObjectPathToObjectName(ObjectPath);
}

void FAssetRegistry::AddAssetData(FAssetData* AssetData)
{
	State.AddAssetData(AssetData);

	// Notify subscribers
	AssetAddedEvent.Broadcast(*AssetData);

	// Populate the class map if adding blueprint
	if (ClassGeneratorNames.Contains(AssetData->AssetClass))
	{
		const FString GeneratedClass = AssetData->GetTagValueRef<FString>("GeneratedClass");
		const FString ParentClass = AssetData->GetTagValueRef<FString>("ParentClass");
		if ( !GeneratedClass.IsEmpty() && !ParentClass.IsEmpty() )
		{
			const FName GeneratedClassFName = *ExportTextPathToObjectName(GeneratedClass);
			const FName ParentClassFName = *ExportTextPathToObjectName(ParentClass);
			CachedInheritanceMap.Add(GeneratedClassFName, ParentClassFName);
		}
	}
}

void FAssetRegistry::UpdateAssetData(FAssetData* AssetData, const FAssetData& NewAssetData)
{
	// Update the class map if updating a blueprint
	if (ClassGeneratorNames.Contains(AssetData->AssetClass))
	{
		const FString OldGeneratedClass = AssetData->GetTagValueRef<FString>("GeneratedClass");
		if (!OldGeneratedClass.IsEmpty())
		{
			const FName OldGeneratedClassFName = *ExportTextPathToObjectName(OldGeneratedClass);
			CachedInheritanceMap.Remove(OldGeneratedClassFName);
		}

		const FString NewGeneratedClass = NewAssetData.GetTagValueRef<FString>("GeneratedClass");
		const FString NewParentClass = NewAssetData.GetTagValueRef<FString>("ParentClass");
		if (!NewGeneratedClass.IsEmpty() && !NewParentClass.IsEmpty())
		{
			const FName NewGeneratedClassFName = *ExportTextPathToObjectName(*NewGeneratedClass);
			const FName NewParentClassFName = *ExportTextPathToObjectName(*NewParentClass);
			CachedInheritanceMap.Add(NewGeneratedClassFName, NewParentClassFName);
		}
	}

	State.UpdateAssetData(AssetData, NewAssetData);
}

bool FAssetRegistry::RemoveAssetData(FAssetData* AssetData)
{
	bool bRemoved = false;

	if ( ensure(AssetData) )
	{
		// Notify subscribers
		AssetRemovedEvent.Broadcast(*AssetData);

		// Remove from the class map if removing a blueprint
		if (ClassGeneratorNames.Contains(AssetData->AssetClass))
		{
			const FString OldGeneratedClass = AssetData->GetTagValueRef<FString>("GeneratedClass");
			if ( !OldGeneratedClass.IsEmpty() )
			{
				const FName OldGeneratedClassFName = *ExportTextPathToObjectName(OldGeneratedClass);
				CachedInheritanceMap.Remove(OldGeneratedClassFName);
			}
		}

		bRemoved = State.RemoveAssetData(AssetData);
	}

	return bRemoved;
}

void FAssetRegistry::RemovePackageData(const FName PackageName)
{
	TArray<FAssetData*>* PackageAssetsPtr = State.CachedAssetsByPackageName.Find(PackageName);
	if (PackageAssetsPtr && PackageAssetsPtr->Num() > 0)
	{
		// Copy the array since RemoveAssetData may re-allocate it!
		TArray<FAssetData*> PackageAssets = *PackageAssetsPtr;
		for (FAssetData* PackageAsset : PackageAssets)
		{
			RemoveAssetData(PackageAsset);
		}
	}
}

void FAssetRegistry::AddPathToSearch(const FString& Path)
{
	if ( BackgroundAssetSearch.IsValid() )
	{
		BackgroundAssetSearch->AddPathToSearch(Path);
	}
}

void FAssetRegistry::AddFilesToSearch (const TArray<FString>& Files)
{
	if ( BackgroundAssetSearch.IsValid() )
	{
		BackgroundAssetSearch->AddFilesToSearch(Files);
	}
}

#if WITH_EDITOR

void FAssetRegistry::OnDirectoryChanged (const TArray<FFileChangeData>& FileChanges)
{
	// Take local copy of FileChanges array as we wish to collapse pairs of 'Removed then Added' FileChangeData
	// entries into a single 'Modified' entry.
	TArray<FFileChangeData> FileChangesProcessed(FileChanges);

	for (int32 FileEntryIndex = 0; FileEntryIndex < FileChangesProcessed.Num(); FileEntryIndex++)
	{
		if (FileChangesProcessed[FileEntryIndex].Action == FFileChangeData::FCA_Added)
		{
			// Search back through previous entries to see if this Added can be paired with a previous Removed
			const FString& FilenameToCompare = FileChangesProcessed[FileEntryIndex].Filename;
			for (int32 SearchIndex = FileEntryIndex - 1; SearchIndex >= 0; SearchIndex--)
			{
				if (FileChangesProcessed[SearchIndex].Action == FFileChangeData::FCA_Removed &&
					FileChangesProcessed[SearchIndex].Filename == FilenameToCompare)
				{
					// Found a Removed which matches the Added - change the Added file entry to be a Modified...
					FileChangesProcessed[FileEntryIndex].Action = FFileChangeData::FCA_Modified;

					// ...and remove the Removed entry
					FileChangesProcessed.RemoveAt(SearchIndex);
					FileEntryIndex--;
					break;
				}
			}
		}
	}

	TArray<FString> NewFiles;
	TArray<FString> ModifiedFiles;

	for (int32 FileIdx = 0; FileIdx < FileChangesProcessed.Num(); ++FileIdx)
	{
		FString LongPackageName;
		const FString File = FString(FileChangesProcessed[FileIdx].Filename);
		const bool bIsPackageFile = FPackageName::IsPackageExtension(*FPaths::GetExtension(File, true));
		const bool bIsValidPackageName = FPackageName::TryConvertFilenameToLongPackageName(File, LongPackageName);
		const bool bIsValidPackage = bIsPackageFile && bIsValidPackageName;

		if ( bIsValidPackage )
		{
			switch( FileChangesProcessed[FileIdx].Action )
			{
				case FFileChangeData::FCA_Added:
					// This is a package file that was created on disk. Mark it to be scanned for asset data.
					NewFiles.AddUnique(File);
					UE_LOG(LogAssetRegistry, Verbose, TEXT("File was added to content directory: %s"), *File);
					break;

				case FFileChangeData::FCA_Modified:
					// This is a package file that changed on disk. Mark it to be scanned immediately for new or removed asset data.
					ModifiedFiles.AddUnique(File);
					UE_LOG(LogAssetRegistry, Verbose, TEXT("File changed in content directory: %s"), *File);
					break;

				case FFileChangeData::FCA_Removed:
					// This file was deleted. Remove all assets in the package from the registry.
					RemovePackageData(*LongPackageName);
					UE_LOG(LogAssetRegistry, Verbose, TEXT("File was removed from content directory: %s"), *File);
					break;
			}
		}
	}

	if (NewFiles.Num())
	{
		AddFilesToSearch(NewFiles);
	}

	if (ModifiedFiles.Num() > 0)
	{
		// Convert all the filenames to package names
		TArray<FString> ModifiedPackageNames;
		ModifiedPackageNames.Reserve(ModifiedFiles.Num());
		for (const FString& File : ModifiedFiles)
		{
			ModifiedPackageNames.Add(FPackageName::FilenameToLongPackageName(File));
		}

		// Get the assets that are currently inside the package
		TArray<TArray<FAssetData*>> ExistingFilesAssetData;
		ExistingFilesAssetData.Reserve(ModifiedFiles.Num());
		for (const FString& PackageName : ModifiedPackageNames)
		{
			TArray<FAssetData*>* PackageAssetsPtr = State.CachedAssetsByPackageName.Find(*PackageName);
			if (PackageAssetsPtr && PackageAssetsPtr->Num() > 0)
			{
				ExistingFilesAssetData.Add(*PackageAssetsPtr);
			}
			else
			{
				ExistingFilesAssetData.AddDefaulted();
			}
		}

		// Re-scan and update the asset registry with the new asset data
		TArray<FName> FoundAssets;
		ScanPathsAndFilesSynchronous(TArray<FString>(), ModifiedFiles, true, EAssetDataCacheMode::NoCache, &FoundAssets, nullptr);

		// Remove any assets that are no longer present in the package
		for (const TArray<FAssetData*>& OldPackageAssets : ExistingFilesAssetData)
		{
			for (FAssetData* OldPackageAsset : OldPackageAssets)
			{
				if (!FoundAssets.Contains(OldPackageAsset->ObjectPath))
				{
					RemoveAssetData(OldPackageAsset);
				}
			}
		}
	}
}

void FAssetRegistry::OnAssetLoaded(UObject *AssetLoaded)
{
	LoadedAssetsToProcess.Add(AssetLoaded);
}

void FAssetRegistry::ProcessLoadedAssetsToUpdateCache(const double TickStartTime)
{
	check(bInitialSearchCompleted && bUpdateDiskCacheAfterLoad);

	const bool bFlushFullBuffer = TickStartTime < 0;

	if (bFlushFullBuffer)
	{
		// Retry the previous failures on a flush
		LoadedAssetsToProcess.Append(LoadedAssetsThatDidNotHaveCachedData);
		LoadedAssetsThatDidNotHaveCachedData.Reset();
	}

	// Add the found assets
	int32 LoadedAssetIndex = 0;
	for (LoadedAssetIndex = 0; LoadedAssetIndex < LoadedAssetsToProcess.Num(); ++LoadedAssetIndex)
	{
		UObject* LoadedAsset = LoadedAssetsToProcess[LoadedAssetIndex].Get();
		
		if (!LoadedAsset)
		{
			// This could be null, in which case it already got freed, ignore
			continue;
		}

		const FName ObjectPath = FName(*LoadedAsset->GetPathName());
		if (AssetDataObjectPathsUpdatedOnLoad.Contains(ObjectPath))
		{
			// Already processed once, don't process again even if it loads a second time
			continue;
		}

		UPackage* InMemoryPackage = LoadedAsset->GetOutermost();
		if (InMemoryPackage->IsDirty())
		{
			// Package is dirty, which means it has temporary changes other than just a PostLoad, ignore
			continue;
		}

		FAssetData** CachedData = State.CachedAssetsByObjectPath.Find(ObjectPath);
		if (!CachedData)
		{
			// Not scanned, can't process right now but try again on next synchronous scan
			LoadedAssetsThatDidNotHaveCachedData.Add(LoadedAsset);
			continue;
		}

		AssetDataObjectPathsUpdatedOnLoad.Add(ObjectPath);

		FAssetData NewAssetData = FAssetData(LoadedAsset);

		if (!NewAssetData.TagsAndValues.GetMap().OrderIndependentCompareEqual((*CachedData)->TagsAndValues.GetMap()))
		{
			// We need to actually update disk cache
			UpdateAssetData(*CachedData, NewAssetData);
		}

		// Check to see if we have run out of time in this tick
		if (!bFlushFullBuffer && (FPlatformTime::Seconds() - TickStartTime) > MaxSecondsPerFrame)
		{
			// Increment the index to properly trim the buffer below
			++LoadedAssetIndex;
			break;
		}
	}

	// Trim the results array
	if (LoadedAssetIndex > 0)
	{
		LoadedAssetsToProcess.RemoveAt(0, LoadedAssetIndex);
	}
}

#endif // WITH_EDITOR


void FAssetRegistry::OnContentPathMounted( const FString& InAssetPath, const FString& FileSystemPath )
{
	// Sanitize
	FString AssetPath = InAssetPath;
	if (AssetPath.EndsWith(TEXT("/")) == false)
	{
		// We actually want a trailing slash here so the path can be properly converted while searching for assets
		AssetPath = AssetPath + TEXT("/");
	}

	// Add this to our list of root paths to process
	AddPathToSearch( AssetPath );

	// Listen for directory changes in this content path
#if WITH_EDITOR
	// Commandlets and in-game don't listen for directory changes
	if ( !IsRunningCommandlet() && GIsEditor)
	{
		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();
		if (DirectoryWatcher)
		{
			// If the path doesn't exist on disk, make it so the watcher will work.
			IFileManager::Get().MakeDirectory(*FileSystemPath);
			DirectoryWatcher->RegisterDirectoryChangedCallback_Handle( FileSystemPath, IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FAssetRegistry::OnDirectoryChanged), OnContentPathMountedOnDirectoryChangedDelegateHandle);
		}
	}
#endif // WITH_EDITOR

}

void FAssetRegistry::OnContentPathDismounted(const FString& InAssetPath, const FString& FileSystemPath)
{
	// Sanitize
	FString AssetPath = InAssetPath;
	if ( AssetPath.EndsWith(TEXT("/")) )
	{
		// We don't want a trailing slash here as it could interfere with RemoveAssetPath
		AssetPath = AssetPath.LeftChop(1);
	}

	// Remove all cached assets found at this location
	{
		TArray<FAssetData*> AllAssetDataToRemove;
		TArray<FString> PathList;
		const bool bRecurse = true;
		GetSubPaths(AssetPath, PathList, bRecurse);
		PathList.Add(AssetPath);
		for ( const FString& Path : PathList )
		{
			TArray<FAssetData*>* AssetsInPath = State.CachedAssetsByPath.Find(FName(*Path));
			if ( AssetsInPath )
			{
				AllAssetDataToRemove.Append(*AssetsInPath);
			}
		}

		for ( FAssetData* AssetData : AllAssetDataToRemove )
		{
			RemoveAssetData(AssetData);
		}
	}

	// Remove the root path
	{
		const bool bEvenIfAssetsStillExist = true;
		RemoveAssetPath(FName(*AssetPath), bEvenIfAssetsStillExist);
	}

	// Stop listening for directory changes in this content path
#if WITH_EDITOR
	// Commandlets and in-game don't listen for directory changes
	if (!IsRunningCommandlet() && GIsEditor)
	{
		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();
		if (DirectoryWatcher)
		{
			DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(FileSystemPath, OnContentPathMountedOnDirectoryChangedDelegateHandle);
		}
	}
#endif // WITH_EDITOR

}

void FAssetRegistry::GetSubClasses(const TArray<FName>& InClassNames, const TSet<FName>& ExcludedClassNames, TSet<FName>& SubClassNames) const
{
	// Build a reverse map of classes to their children for quick lookup
	TMap<FName, TSet<FName>> ReverseInheritanceMap;

	// And add all in-memory classes at request time
	TSet<FName> InMemoryClassNames;

	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;

		if ( !Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists) )
		{
			if ( Class->GetSuperClass() )
			{
				TSet<FName>& ChildClasses = ReverseInheritanceMap.FindOrAdd( Class->GetSuperClass()->GetFName() );
				ChildClasses.Add( Class->GetFName() );
			}

			// Add any implemented interfaces to the reverse inheritance map
			for (int32 i = 0; i < Class->Interfaces.Num(); ++i)
			{
				UClass* InterfaceClass = Class->Interfaces[i].Class;
				if (ensureMsgf(InterfaceClass, TEXT("Invalid inteface class (index %d) detected on '%s' - possibly deleted?"), i, *Class->GetName()))
				{
					TSet<FName>& ChildClasses = ReverseInheritanceMap.FindOrAdd(InterfaceClass->GetFName());
					ChildClasses.Add(Class->GetFName());
				}
			}

			InMemoryClassNames.Add(Class->GetFName());
		}
	}

	// Form a child list for all cached classes
	for (auto ClassNameIt = CachedInheritanceMap.CreateConstIterator(); ClassNameIt; ++ClassNameIt)
	{
		const FName ClassName = ClassNameIt.Key();
		if ( !InMemoryClassNames.Contains(ClassName) )
		{
			const FName ParentClassName = ClassNameIt.Value();
			if ( ParentClassName != NAME_None )
			{
				TSet<FName>& ChildClasses = ReverseInheritanceMap.FindOrAdd( ParentClassName );
				ChildClasses.Add( ClassName );
			}
		}
	}

	for (FName ClassName : InClassNames)
	{
		// Now find all subclass names
		GetSubClasses_Recursive(ClassName, SubClassNames, ReverseInheritanceMap, ExcludedClassNames);
	}
}

void FAssetRegistry::GetSubClasses_Recursive(FName InClassName, TSet<FName>& SubClassNames, const TMap<FName, TSet<FName>>& ReverseInheritanceMap, const TSet<FName>& ExcludedClassNames) const
{
	if ( ExcludedClassNames.Contains(InClassName) )
	{
		// This class is in the exclusion list. Exclude it.
	}
	else
	{
		SubClassNames.Add(InClassName);

		const TSet<FName>* FoundSubClassNames = ReverseInheritanceMap.Find(InClassName);
		if ( FoundSubClassNames )
		{
			for (FName ClassName : (*FoundSubClassNames))
			{
				GetSubClasses_Recursive(ClassName, SubClassNames, ReverseInheritanceMap, ExcludedClassNames);
			}
		}
	}
}

void FAssetRegistry::SetManageReferences(const TMultiMap<FAssetIdentifier, FAssetIdentifier>& ManagerMap, bool bClearExisting, EAssetRegistryDependencyType::Type RecurseType, ShouldSetManagerPredicate ShouldSetManager)
{
	TSet<FDependsNode*> ExistingManagedNodes;

	// Set default predicate if needed
	if (!ShouldSetManager)
	{
		ShouldSetManager = [](const FAssetIdentifier& Manager, const FAssetIdentifier& Source, const FAssetIdentifier& Target, EAssetRegistryDependencyType::Type DependencyType, EAssetSetManagerFlags::Type Flags)
		{
			return EAssetSetManagerResult::SetButDoNotRecurse;
		};
	}

	// Find all nodes with incoming manage dependencies
	for (const TPair<FAssetIdentifier, FDependsNode*>& Pair : State.CachedDependsNodes)
	{
		Pair.Value->IterateOverDependencies([&ExistingManagedNodes](FDependsNode* TestNode, EAssetRegistryDependencyType::Type DependencyType)
		{
			ExistingManagedNodes.Add(TestNode);
		}, EAssetRegistryDependencyType::Manage);
	}

	if (bClearExisting)
	{
		// Clear them
		for (FDependsNode* NodeToClear : ExistingManagedNodes)
		{
			NodeToClear->RemoveManageReferencesToNode();
		}
		ExistingManagedNodes.Empty();
	}

	TMap<FDependsNode*, TArray<FDependsNode *>> ExplicitMap; // Reverse of ManagerMap, specifies what relationships to add to each node

	for (const TPair<FAssetIdentifier, FAssetIdentifier>& Pair : ManagerMap)
	{
		FDependsNode* ManagedNode = State.FindDependsNode(Pair.Value);

		if (!ManagedNode)
		{
			UE_LOG(LogAssetRegistry, Error, TEXT("Cannot set %s to manage asset %s because it does not exist!"), *Pair.Key.ToString(), *Pair.Value.ToString());
			continue;
		}

		TArray<FDependsNode*>& ManagerList = ExplicitMap.FindOrAdd(ManagedNode);

		FDependsNode* ManagerNode = State.CreateOrFindDependsNode(Pair.Key);

		ManagerList.Add(ManagerNode);
	}

	TSet<FDependsNode*> Visited;
	TArray<FDependsNode*> NodesToManage;
	TArray<FDependsNode*> NodesToRecurse;

	// For each explicitly set asset
	for (const TPair<FDependsNode*, TArray<FDependsNode *>>& Pair : ExplicitMap)
	{
		FDependsNode* BaseManagedNode = Pair.Key;
		const TArray<FDependsNode*>& ManagerNodes = Pair.Value;

		for (FDependsNode* ManagerNode : ManagerNodes)
		{	
			Visited.Reset();
			NodesToManage.Reset();
			NodesToRecurse.Reset();
			
			FDependsNode* SourceNode = ManagerNode;

			auto IterateFunction = [&ManagerNode, &SourceNode, &ShouldSetManager, &NodesToManage, &NodesToRecurse, &Visited, &ExplicitMap, &ExistingManagedNodes](FDependsNode* TargetNode, EAssetRegistryDependencyType::Type DependencyType)
			{
				// Only recurse if we haven't already visited, and this node passes recursion test
				if (!Visited.Contains(TargetNode))
				{
					EAssetSetManagerFlags::Type Flags = (EAssetSetManagerFlags::Type)((SourceNode == ManagerNode ? EAssetSetManagerFlags::IsDirectSet : 0)
						| (ExistingManagedNodes.Contains(TargetNode) ? EAssetSetManagerFlags::TargetHasExistingManager : 0)
						| (ExplicitMap.Find(TargetNode) && SourceNode != ManagerNode ? EAssetSetManagerFlags::TargetHasDirectManager : 0));

					EAssetSetManagerResult::Type Result = ShouldSetManager(ManagerNode->GetIdentifier(), SourceNode->GetIdentifier(), TargetNode->GetIdentifier(), DependencyType, Flags);

					if (Result == EAssetSetManagerResult::DoNotSet)
					{
						return;
					}

					NodesToManage.Push(TargetNode);

					if (Result == EAssetSetManagerResult::SetAndRecurse)
					{
						NodesToRecurse.Push(TargetNode);
					}
				}
			};

			// Check initial node
			IterateFunction(BaseManagedNode, EAssetRegistryDependencyType::Manage);

			// Do all recursion first, but only if we have a recurse type
			if (RecurseType)
			{
				while (NodesToRecurse.Num())
				{
					// Pull off end of array, order doesn't matter
					SourceNode = NodesToRecurse.Pop();

					Visited.Add(SourceNode);

					SourceNode->IterateOverDependencies(IterateFunction, RecurseType);
				}
			}

			// Now set all the dependencies
			while (NodesToManage.Num())
			{
				// Pull off end of array, order doesn't matter
				FDependsNode* ManagedNode = NodesToManage.Pop();

				ManagerNode->AddDependency(ManagedNode, EAssetRegistryDependencyType::Manage);
				ManagedNode->AddReferencer(ManagerNode);
			}
		}
	}
}

bool FAssetRegistry::SetPrimaryAssetIdForObjectPath(const FName ObjectPath, FPrimaryAssetId PrimaryAssetId)
{
	FAssetData** FoundAssetData = State.CachedAssetsByObjectPath.Find(ObjectPath);

	if (!FoundAssetData)
	{
		return false;
	}

	FAssetData* AssetData = *FoundAssetData;

	TMap<FName, FString> TagsAndValues = AssetData->TagsAndValues.GetMap();
	TagsAndValues.Add(FPrimaryAssetId::PrimaryAssetTypeTag, PrimaryAssetId.PrimaryAssetType.ToString());
	TagsAndValues.Add(FPrimaryAssetId::PrimaryAssetNameTag, PrimaryAssetId.PrimaryAssetName.ToString());

	FAssetData NewAssetData = FAssetData(AssetData->PackageName, AssetData->PackagePath, AssetData->GroupNames, AssetData->AssetName, AssetData->AssetClass, TagsAndValues, AssetData->ChunkIDs, AssetData->PackageFlags);

	UpdateAssetData(AssetData, NewAssetData);

	return true;
}

const FAssetData* FAssetRegistry::GetCachedAssetDataForObjectPath(const FName ObjectPath) const
{
	return State.GetAssetByObjectPath(ObjectPath);
}