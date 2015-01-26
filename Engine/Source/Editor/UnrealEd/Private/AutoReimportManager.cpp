// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AssetToolsModule.h"

#include "Developer/DirectoryWatcher/Public/DirectoryWatcherModule.h"
#include "Engine/CurveTable.h"
#include "Engine/DataTable.h"
#include "AutoReimport/AutoReimportManager.h"
#include "PackageTools.h"
#include "ObjectTools.h"
#include "ARFilter.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "AutoReimportManager"

DEFINE_LOG_CATEGORY_STATIC(LogAutoReimportManager, Log, All);

namespace
{
	/** Template helper function to count the number of elements in the specified array that pass a predicate */
	template<typename T, typename P>
	int32 CountIf(const TArray<T>& InArray, P Predicate)
	{
		int32 Count = 0;
		for (const T& X : InArray)
		{
			if (Predicate(X))
			{
				++Count;
			}
		}
		return Count;
	}

	/** Retrieve a semi-colon separated string of file extensions supported by all available editor import factories */
	FString GetAllFactoryExtensions()
	{
		FString AllExtensions;

		// Use a scratch buffer to avoid unnecessary re-allocation
		FString Scratch;
		Scratch.Reserve(32);

		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* Class = *ClassIt;
			if (Class->IsChildOf(UFactory::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
			{
				UFactory* Factory = Cast<UFactory>(Class->GetDefaultObject());
				if (Factory->bEditorImport)
				{
					for (const FString& Format : Factory->Formats)
					{
						int32 Index = INDEX_NONE;
						if (Format.FindChar(';', Index) && Index > 0)
						{
							Scratch.GetCharArray().Reset();
							// Include the ;
							Scratch.AppendChars(*Format, Index + 1);

							if (AllExtensions.Find(Scratch) == INDEX_NONE)
							{
								AllExtensions += Scratch;
							}
						}
					}
				}
			}
		}

		return AllExtensions;
	}

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

	/** Generate a config from the specified options, to pass to FFileCache on construction */
	FFileCacheConfig GenerateFileCacheConfig(const FString& InPath, const FString& InSupportedExtensions, const FString& InMountedContentPath)
	{
		FString Directory = FPaths::ConvertRelativePathToFull(InPath);

		const FString& HashString = InMountedContentPath.IsEmpty() ? Directory : InMountedContentPath;
		const uint32 CRC = FCrc::MemCrc32(*HashString, HashString.Len()*sizeof(TCHAR));	
		FString CacheFilename = FPaths::ConvertRelativePathToFull(FPaths::GameIntermediateDir()) / TEXT("ReimportCache") / FString::Printf(TEXT("%u.bin"), CRC);

		FFileCacheConfig Config(MoveTemp(Directory), MoveTemp(CacheFilename));
		Config.BatchDelayS = 1;
		Config.IncludeExtensions = InSupportedExtensions;

		return Config;
	}

	/** Choose a location to import the specified asset into */
	TOptional<FString> ChooseAssetImportLocation(const FString& Filename)
	{
		FString DefaultPath = TEXT("/Game/");

		FString NameSuggestion;
		{
			FString PackageName = DefaultPath / FPaths::GetBaseFilename(Filename);
			FString Name;
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
			AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

			NameSuggestion = FPaths::GetCleanFilename(PackageName);
		}

		FSaveAssetDialogConfig Config;
		Config.DialogTitleOverride = LOCTEXT("ImportAssetDialogTitle", "Import asset to:");
		Config.DefaultPath = DefaultPath;
		Config.DefaultAssetName = NameSuggestion;
		Config.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::Disallow;

		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		FString SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(Config);
		if ( !SaveObjectPath.IsEmpty() )
		{
			return SaveObjectPath;
		}

		return TOptional<FString>();
	}
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

void FContentDirectoryMonitor::Tick(const IAssetRegistry& Registry, const FWorkLimiter& Limiter)
{
	Cache.Tick(Limiter);

	if (Limiter.ShouldLimit())
	{
		return;
	}
	// We can't do this if the registry is loading assets as we won't necesssarily be able to react to changes effectively
	else if (!Registry.IsLoadingAssets())
	{
		OutstandingChanges = Cache.GetOutstandingChanges();
		if (OutstandingChanges.Num() != 0)
		{
			ProcessOutstandingChanges(Registry, Limiter);
		}
	}
	
	if (Limiter.ShouldLimit())
	{
		return;
	}
	// Save out the cache file if we've got time to do so, and have not saved for a while
	else
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - LastSaveTime > ResaveIntervalS)
		{
			LastSaveTime = Now;
			Cache.WriteCache();
		}
	}
}

void FContentDirectoryMonitor::ProcessOutstandingChanges(const IAssetRegistry& Registry, const FWorkLimiter& Limiter)
{
	// Count the number of additions so we can apprximate the work load
	const int32 NumAdditions = CountIf(OutstandingChanges, [](const FUpdateCacheTransaction& Change){ return Change.Action == FFileChangeData::FCA_Added; });

	auto* ReimportManager = FReimportManager::Instance();

	TArray<UObject*> AssetsToDelete;
	{
		bool bCreatedDialog = false;
		FScopedSlowTask SlowTask(OutstandingChanges.Num() + NumAdditions, LOCTEXT("ProcessingChanges", "Processing source content changes"));
		SlowTask.MakeDialog();

		TArray<UPackage*> PackagesToSave;

		for (auto& Change : OutstandingChanges)
		{
			const FString FullFilename = Cache.GetDirectory() + Change.Filename.Get();

			if (!bCreatedDialog && FPlatformTime::Seconds() - SlowTask.StartTime > 2)
			{
				//SlowTask.MakeDialog();
				bCreatedDialog = true;
			}
			
			SlowTask.EnterProgressFrame();
			switch(Change.Action)
			{
				case FFileChangeData::FCA_Modified:
					// @todo: arodham: enable/disable specific extensions/asset types?
					{
						for (const auto& AssetData : FindAssetsPertainingToFile(Registry, FullFilename))
						{
							UObject* Asset = AssetData.GetAsset();
							ReimportManager->Reimport(Asset);
						}
					}
					break;

				case FFileChangeData::FCA_Added:
					{
						FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

						TArray<FString> File;
						File.Add(FullFilename);

						FString ParentPackage;
						if (MountedContentPath.IsEmpty())
						{
							// @todo: arodham: this is sketchy and needs revisiting. PromptUserForDirectory?
							auto UserChosenPath = ChooseAssetImportLocation(FullFilename);
							if (UserChosenPath.IsSet())
							{
								ParentPackage = UserChosenPath.GetValue();
							}
						}
						else
						{
							ParentPackage = MountedContentPath / PackageTools::SanitizePackageName(FPaths::GetPath(Change.Filename.Get()));
						}

						if (!ParentPackage.IsEmpty())
						{
							auto NewAssets = AssetToolsModule.Get().ImportAssets(File, ParentPackage);

							// @todo: arodham: Error reporting

							for (auto* Asset : NewAssets)
							{
								UPackage* Package = Asset->GetOutermost();
								if (Package)
								{
									PackagesToSave.Add(Package);
								}
							}
						}
					}
					break;

				case FFileChangeData::FCA_Removed:
					for (const auto& AssetData : FindAssetsPertainingToFile(Registry, FullFilename))
					{
						AssetsToDelete.Add(AssetData.GetAsset());
					}
					break;

				default:
					break;
			}

			// Let the cache know that we've dealt with this change
			Cache.CompleteTransaction(MoveTemp(Change));
		}

		SlowTask.EnterProgressFrame(NumAdditions);
		if (PackagesToSave.Num() > 0)
		{
			const bool bAlreadyCheckedOut = false;
			const bool bCheckDirty = false;
			const bool bPromptToSave = false;
			// @todo: arodham: Error reporting?
			FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave, nullptr, bAlreadyCheckedOut);
		}
	}

	// Open a dialog to ask about deleted files
	// @todo: this needs improvement

	if (AssetsToDelete.Num() > 0)
	{
		ObjectTools::DeleteObjects(AssetsToDelete);
	}

	OutstandingChanges.Empty();
}

UAutoReimportManager::UAutoReimportManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsProcessingChanges(false)
{

}

void UAutoReimportManager::Initialize()
{
	// @todo: arodham: update this when modules are reloaded or new factory types are available?
	SupportedExtensions = GetAllFactoryExtensions();

	auto* Settings = GetMutableDefault<UEditorLoadingSavingSettings>();
	Settings->OnSettingChanged().AddUObject(this, &UAutoReimportManager::HandleLoadingSavingSettingChanged);

	FPackageName::OnContentPathMounted().AddUObject(this, &UAutoReimportManager::OnContentPathMounted);
	FPackageName::OnContentPathDismounted().AddUObject(this, &UAutoReimportManager::OnContentPathDismounted);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.OnInMemoryAssetDeleted().AddUObject(this, &UAutoReimportManager::OnAssetDeleted);


	// Only set this up for content directories if the user has this enabled
	if (Settings->bMonitorContentDirectories)
	{
		SetUpMountedContentDirectories();
	}

	// We always monitor user directories if they are set
	SetUpUserDirectories();
}

void UAutoReimportManager::BeginDestroy() 
{
	// Force a save of all the caches
	MountedDirectoryMonitors.Empty();
	UserDirectoryMonitors.Empty();

	Super::BeginDestroy();
}

void UAutoReimportManager::Tick(float DeltaTime)
{
	// Only spend half a frame on this, at most
	const FWorkLimiter Limiter(1.0 / 120);

	const IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	TGuardValue<bool> Guard(bIsProcessingChanges, true);

	for (auto& Monitor : MountedDirectoryMonitors)
	{
		Monitor.Tick(Registry, Limiter);
		if (Limiter.ShouldLimit())
		{
			return;
		}
	}

	for (auto& Monitor : UserDirectoryMonitors)
	{
		Monitor.Tick(Registry, Limiter);
		if (Limiter.ShouldLimit())
		{
			return;
		}
	}
}

TStatId UAutoReimportManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAutoReimportManager, STATGROUP_Tickables);
}

void UAutoReimportManager::HandleLoadingSavingSettingChanged(FName PropertyName)
{
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UEditorLoadingSavingSettings, AutoReimportDirectories))
	{
		SetUpUserDirectories();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UEditorLoadingSavingSettings, bMonitorContentDirectories))
	{
		const auto* Settings = GetDefault<UEditorLoadingSavingSettings>();
		if (Settings->bMonitorContentDirectories)
		{
			// Should already be empty, but just to be sure
			MountedDirectoryMonitors.Empty();
			SetUpMountedContentDirectories();
		}
		else
		{
			// Destroy all the existing montirs, including their file caches
			for (auto& Monitor : MountedDirectoryMonitors)
			{
				Monitor.Destroy();
			}
			MountedDirectoryMonitors.Empty();
		}
	}
}

void UAutoReimportManager::OnContentPathMounted(const FString& InAssetPath, const FString& FileSystemPath)
{
	MountedDirectoryMonitors.Emplace(FPaths::ConvertRelativePathToFull(FileSystemPath), SupportedExtensions, InAssetPath);
}

void UAutoReimportManager::OnContentPathDismounted(const FString& InAssetPath, const FString& FileSystemPath)
{
	MountedDirectoryMonitors.RemoveAll([&](const FContentDirectoryMonitor& Monitor){
		return Monitor.GetMountedContentPath() == InAssetPath;
	});
}

void UAutoReimportManager::OnAssetDeleted(UObject* Object)
{
	if (bIsProcessingChanges)
	{
		return;
	}

	const IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	bool bCanDelete = true;

	TArray<FAssetRegistryTag> Tags;
	Object->GetAssetRegistryTags(Tags);

	TArray<FString> AbsoluteSourceFileNames;

	const FName TagName = UObject::SourceFileTagName();

	for (const auto& Tag : Tags)
	{
		// Don't compare the FName number suffix
		if (Tag.Name.IsEqual(TagName, ENameCase::IgnoreCase, false))
		{
			const FString SourceFile = FReimportManager::ResolveImportFilename(Tag.Value, Object);
			// Check there aren't any other assets referencing this thing
			if (FindAssetsPertainingToFile(Registry, SourceFile).Num() != 0)
			{
				bCanDelete = false;
				break;
			}
			else
			{
				AbsoluteSourceFileNames.Add(MoveTemp(SourceFile));
			}
		}
	}

	if (bCanDelete)
	{
		// Offer to delete the source file(s) as well
		const FText MessageText = FText::Format(LOCTEXT("DeleteRelatedAssets", "The asset '{0}' has been deleted. Would you like to delete the source files it was imported from as well?"), FText::FromString(Object->GetName()) );
		if (FMessageDialog::Open(EAppMsgType::YesNo, MessageText) == EAppReturnType::Yes)
		{
			auto& FileManager = IFileManager::Get();

			for (auto& Filename : AbsoluteSourceFileNames)
			{
				FileManager.Delete(*Filename, false /* RequireExists */, true /* Even if read only */, true /* Quiet */);
			}
		}
	}
}

void UAutoReimportManager::SetUpMountedContentDirectories()
{
	TArray<FString> ContentPaths;
	FPackageName::QueryRootContentPaths(ContentPaths);
	for (const auto& ContentPath : ContentPaths)
	{
		const FString Directory = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(ContentPath));
		MountedDirectoryMonitors.Emplace(Directory, SupportedExtensions, ContentPath);
	}
}

void UAutoReimportManager::SetUpUserDirectories()
{
	TArray<bool> PersistedDirectories;
	PersistedDirectories.AddZeroed(UserDirectoryMonitors.Num());

	// Also set up user-specified directories
	TArray<FString> NewDirectories = GetDefault<UEditorLoadingSavingSettings>()->AutoReimportDirectories;

	for (int32 Index = 0; Index < NewDirectories.Num(); ++Index)
	{
		NewDirectories[Index] = FPaths::ConvertRelativePathToFull(NewDirectories[Index]);
		const int32 Existing = UserDirectoryMonitors.IndexOfByPredicate([&](const FContentDirectoryMonitor& InMonitor){ return InMonitor.GetDirectory() == NewDirectories[Index]; });
		if (Existing != INDEX_NONE)
		{
			PersistedDirectories[Existing] = true;
		}
		else
		{
			// Add a new one
			UserDirectoryMonitors.Emplace(NewDirectories[Index], SupportedExtensions);
			PersistedDirectories.Add(true);
		}
	}

	// Delete anything that is no longer needed (we delete the cache file altogether)
	for (int32 Index = PersistedDirectories.Num() - 1; Index >= 0; --Index)
	{
		if (!PersistedDirectories[Index])
		{
			UserDirectoryMonitors[Index].Destroy();
			UserDirectoryMonitors.RemoveAt(Index, 1, false);
		}
	}

	if (UserDirectoryMonitors.GetSlack() > UserDirectoryMonitors.Max() / 2)
	{
		UserDirectoryMonitors.Shrink();
	}
}

#undef LOCTEXT_NAMESPACE

