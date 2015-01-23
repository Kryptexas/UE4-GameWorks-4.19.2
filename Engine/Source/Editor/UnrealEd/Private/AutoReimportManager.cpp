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
#include "NotificationManager.h"
#include "SNotificationList.h"

#define LOCTEXT_NAMESPACE "AutoReimportManager"
#define CONDITIONAL_YIELD if (TimeLimit.Exceeded()) { return; }

DEFINE_LOG_CATEGORY_STATIC(LogAutoReimportManager, Log, All);

namespace
{
	/** Reduce the array to the specified accumulator */
	template<typename T, typename P, typename A>
	A Reduce(const TArray<T>& InArray, P Predicate, A Accumulator)
	{
		for (const T& X : InArray)
		{
			Accumulator = Predicate(X, Accumulator);
		}
		return Accumulator;
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
		Config.IncludeExtensions = InSupportedExtensions;
		// We always store paths inside content folders relative to the folder
		Config.PathType = EPathType::Relative;

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

FReimportFeedbackContext::FReimportFeedbackContext()
{

}

FReimportFeedbackContext::~FReimportFeedbackContext()
{
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->ExpireAndFadeout();
	}
}

void FReimportFeedbackContext::UpdateText(int32 Progress, int32 Total)
{
	FFormatOrderedArguments Args;
	Args.Add(Progress);
	Args.Add(Total);

	const FText Message = FText::Format(LOCTEXT("ProcessingChanges", "Processing outstanding content changes ({0} of {1})"), Args);
	if (Notification.IsValid())
	{
		Notification->SetText(Message);
	}
	else
	{
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 2.0f;
		Info.bUseLargeFont = false;
		Info.bFireAndForget = false;
		Info.bUseSuccessFailIcons = false;
		Info.Image = FEditorStyle::GetBrush("NoBrush");

		Notification = FSlateNotificationManager::Get().AddNotification(Info);
		Notification->SetCompletionState(SNotificationItem::CS_Pending);
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

void FContentDirectoryMonitor::ProcessModifications(const IAssetRegistry& Registry, const FTimeLimit& TimeLimit, FReimportFeedbackContext& Context)
{
	auto* ReimportManager = FReimportManager::Instance();

	for (int32 Index = 0; Index < ModifiedFiles.Num(); ++Index)
	{
		auto& Change = ModifiedFiles[Index];

		const FString FullFilename = Cache.GetDirectory() + Change.Filename.Get();
		for (const auto& AssetData : FindAssetsPertainingToFile(Registry, FullFilename))
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
		for (const auto& AssetData : FindAssetsPertainingToFile(Registry, Cache.GetDirectory() + Deletion.Filename.Get()))
		{
			OutAssetsToDelete.Add(AssetData);
		}

		// Let the cache know that we've dealt with this change (it will be imported in due course)
		Cache.CompleteTransaction(MoveTemp(Deletion));
	}

	WorkProgress += DeletedFiles.Num();
	DeletedFiles.Empty();
}

UAutoReimportManager::UAutoReimportManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentState(ECurrentState::Idle)
	, bIsProcessingChanges(false)
	, StartProcessingDelay(1)
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

int32 UAutoReimportManager::GetNumUnprocessedChanges() const
{
	auto Pred = [](const FContentDirectoryMonitor& Monitor, int32 Total){ return Total + Monitor.GetNumUnprocessedChanges(); };
	return Reduce(MountedDirectoryMonitors, Pred, 0) + Reduce(UserDirectoryMonitors, Pred, 0);
}

int32 UAutoReimportManager::GetTotalWork() const
{
	auto Pred = [](const FContentDirectoryMonitor& Monitor, int32 Total){ return Total + Monitor.GetTotalWork(); };
	return Reduce(MountedDirectoryMonitors, Pred, 0) + Reduce(UserDirectoryMonitors, Pred, 0);
}

int32 UAutoReimportManager::GetTotalProgress() const
{
	auto Pred = [](const FContentDirectoryMonitor& Monitor, int32 Total){ return Total + Monitor.GetWorkProgress(); };
	return Reduce(MountedDirectoryMonitors, Pred, 0) + Reduce(UserDirectoryMonitors, Pred, 0);
}

void UAutoReimportManager::ProcessAdditions(const FTimeLimit& TimeLimit)
{
	TMap<FString, TArray<UFactory*>> Factories;

	TArray<FString> FactoryExtensions;
	FactoryExtensions.Reserve(16);

	// Get the list of valid factories
	for (TObjectIterator<UClass> It ; It ; ++It)
	{
		UClass* CurrentClass = (*It);

		if (CurrentClass->IsChildOf(UFactory::StaticClass()) && !(CurrentClass->HasAnyClassFlags(CLASS_Abstract)))
		{
			UFactory* Factory = Cast<UFactory>(CurrentClass->GetDefaultObject());
			if (Factory->bEditorImport && Factory->ImportPriority >= 0)
			{
				FactoryExtensions.Reset();
				Factory->GetSupportedFileExtensions(FactoryExtensions);

				for (const auto& Ext : FactoryExtensions)
				{
					auto& Array = Factories.FindOrAdd(Ext);
					Array.Add(Factory);
				}
			}
		}
	}

	for (auto& Pair : Factories)
	{
		Pair.Value.Sort([](const UFactory& A, const UFactory& B) { return A.ImportPriority > B.ImportPriority; });
	}

	for (auto& Monitor : MountedDirectoryMonitors)
	{
		Monitor.ProcessAdditions(PackagesToSave, TimeLimit, Factories);
	}

	if (!TimeLimit.Exceeded())
	{
		for (auto& Monitor : UserDirectoryMonitors)
		{
			Monitor.ProcessAdditions(PackagesToSave, TimeLimit, Factories);
		}
	}

	CONDITIONAL_YIELD

	CurrentState = ECurrentState::SavePackages;
}

void UAutoReimportManager::ProcessModifications(const IAssetRegistry& Registry, const FTimeLimit& TimeLimit)
{
	for (auto& Monitor : MountedDirectoryMonitors)
	{
		Monitor.ProcessModifications(Registry, TimeLimit, *FeedbackContextOverride);
		CONDITIONAL_YIELD
	}

	for (auto& Monitor : UserDirectoryMonitors)
	{
		Monitor.ProcessModifications(Registry, TimeLimit, *FeedbackContextOverride);
		CONDITIONAL_YIELD
	}

	CurrentState = ECurrentState::ProcessDeletions;
}

void UAutoReimportManager::ProcessDeletions(const IAssetRegistry& Registry)
{
	TArray<FAssetData> AssetsToDelete;

	for (auto& Monitor : MountedDirectoryMonitors)
	{
		Monitor.ExtractAssetsToDelete(Registry, AssetsToDelete);
	}
	for (auto& Monitor : UserDirectoryMonitors)
	{
		Monitor.ExtractAssetsToDelete(Registry, AssetsToDelete);
	}

	if (AssetsToDelete.Num() > 0)
	{
		ObjectTools::DeleteAssets(AssetsToDelete);
	}

	CurrentState = ECurrentState::Cleanup;
}


void UAutoReimportManager::Tick(float DeltaTime)
{
	// Never spend more than a 60fps frame doing this work (meaning we shouldn't drop below 30fps), we can do more if we're throttling CPU usage (ie editor running in background)
	const FTimeLimit TimeLimit(GEditor->ShouldThrottleCPUUsage() ? 1 / 6.f : 1.f / 60.f);

	if (CurrentState == ECurrentState::Idle)
	{
		for (auto& Monitor : MountedDirectoryMonitors)
		{
			Monitor.Tick(TimeLimit);
			CONDITIONAL_YIELD
		}
		for (auto& Monitor : UserDirectoryMonitors)
		{
			Monitor.Tick(TimeLimit);
			CONDITIONAL_YIELD
		}

		const int32 NumUnprocessedChanges = GetNumUnprocessedChanges();
		if (NumUnprocessedChanges > 0)
		{
			// Check if we have any more unprocessed changes. If we have, we wait a delay before processing them.
			if (NumUnprocessedChanges != CachedNumUnprocessedChanges)
			{
				CachedNumUnprocessedChanges = NumUnprocessedChanges;
				StartProcessingDelay.Reset();
			}
			else
			{
				if (StartProcessingDelay.Exceeded())
				{
					// We have some stuff to do, let's kick that off
					CurrentState = ECurrentState::Initialize;
					FeedbackContextOverride.Reset(new FReimportFeedbackContext);
				}
			}
		}
	}
	else if (FSlateThrottleManager::Get().IsAllowingExpensiveTasks())
	{
		TGuardValue<bool> GuardProcessingChanges(bIsProcessingChanges, true);
		
		FeedbackContextOverride->UpdateText(GetTotalProgress(), GetTotalWork());

		if (CurrentState == ECurrentState::Initialize)
		{
			for (auto& Monitor : MountedDirectoryMonitors)
			{
				Monitor.StartProcessing();
			}
			for (auto& Monitor : UserDirectoryMonitors)
			{
				Monitor.StartProcessing();
			}

			CurrentState = ECurrentState::ProcessAdditions;

			FeedbackContextOverride->UpdateText(GetTotalProgress(), GetTotalWork());
			CONDITIONAL_YIELD
		}

		if (CurrentState == ECurrentState::ProcessAdditions)
		{
			// Override the global feedback context while we do this to avoid popping up dialogs
			TGuardValue<FFeedbackContext*> ScopedContextOverride(GWarn, FeedbackContextOverride.Get());

			ProcessAdditions(TimeLimit);

			FeedbackContextOverride->UpdateText(GetTotalProgress(), GetTotalWork());
			CONDITIONAL_YIELD
		}

		if (CurrentState == ECurrentState::SavePackages)
		{
			// Override the global feedback context while we do this to avoid popping up dialogs
			TGuardValue<FFeedbackContext*> ScopedContextOverride(GWarn, FeedbackContextOverride.Get());

			if (PackagesToSave.Num() > 0)
			{
				const bool bAlreadyCheckedOut = false;
				const bool bCheckDirty = false;
				const bool bPromptToSave = false;
				// @todo: arodham: Error reporting?
				FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave, nullptr, bAlreadyCheckedOut);

				PackagesToSave.Empty();
			}

			CurrentState = ECurrentState::ProcessModifications;
			CONDITIONAL_YIELD
		}

		const IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		if (CurrentState == ECurrentState::ProcessModifications)
		{
			// Override the global feedback context while we do this to avoid popping up dialogs
			TGuardValue<FFeedbackContext*> ScopedContextOverride(GWarn, FeedbackContextOverride.Get());

			ProcessModifications(Registry, TimeLimit);

			FeedbackContextOverride->UpdateText(GetTotalProgress(), GetTotalWork());
			CONDITIONAL_YIELD
		}

		if (CurrentState == ECurrentState::ProcessDeletions)
		{
			ProcessDeletions(Registry);

			FeedbackContextOverride->UpdateText(GetTotalProgress(), GetTotalWork());
			CONDITIONAL_YIELD
		}

		if (CurrentState == ECurrentState::Cleanup)
		{
			// Finished
			FeedbackContextOverride->UpdateText(GetTotalWork(), GetTotalWork());

			CachedNumUnprocessedChanges = 0;
			FeedbackContextOverride.Reset();

			CurrentState = ECurrentState::Idle;
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
			// Destroy all the existing monitors, including their file caches
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
	const auto FullDismountedPath = FPaths::ConvertRelativePathToFull(FileSystemPath);

	MountedDirectoryMonitors.RemoveAll([&](const FContentDirectoryMonitor& Monitor){
		return Monitor.GetDirectory() == FullDismountedPath;
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

		// We ignore the engine content directory for now
		if (Directory != FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir()))
		{
			MountedDirectoryMonitors.Emplace(Directory, SupportedExtensions, ContentPath);
		}
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

#undef CONDITIONAL_YIELD
#undef LOCTEXT_NAMESPACE

