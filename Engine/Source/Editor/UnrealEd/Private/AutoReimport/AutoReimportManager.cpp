// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "AutoReimportUtilities.h"
#include "AutoReimport/AutoReimportManager.h"
#include "ContentDirectoryMonitor.h"

#include "ObjectTools.h"
#include "ARFilter.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "NotificationManager.h"
#include "SNotificationList.h"

#define LOCTEXT_NAMESPACE "AutoReimportManager"
#define CONDITIONAL_YIELD if (TimeLimit.Exceeded()) { return; }

DEFINE_LOG_CATEGORY_STATIC(LogAutoReimportManager, Log, All);

/** Feedback context that overrides GWarn for import operations to prevent popup spam */
struct FReimportFeedbackContext : public FFeedbackContext
{
	/** Constructor - created when we are processing changes */
	FReimportFeedbackContext()
	{
	}

	/** Destructor - called when we are done processing changes */
	~FReimportFeedbackContext()
	{
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Success);
			Notification->ExpireAndFadeout();
		}
	}

	/** Update the text on the notification */
	void UpdateText(int32 Progress, int32 Total)
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

private:
	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) {}

private:
	/** The notification that is shown when the context is active */
	TSharedPtr<SNotificationItem> Notification;

	/** The notification content */
	TSharedPtr<class SAutoReimportFeedback> NotificationContent;
};

/* Deals with auto reimporting of objects when the objects file on disk is modified*/
class FAutoReimportManager : public FTickableEditorObject
{
public:

	FAutoReimportManager();
	~FAutoReimportManager();

private:

	/** FTickableEditorObject interface*/
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

private:

	/** Called when a new asset path has been mounted */
	void OnContentPathMounted(const FString& InAssetPath, const FString& FileSystemPath);

	/** Called when a new asset path has been dismounted */
	void OnContentPathDismounted(const FString& InAssetPath, const FString& FileSystemPath);

	/** Called when an asset has been deleted */
	void OnAssetDeleted(UObject* Object);

private:

	/** Callback for when an editor user setting has changed */
	void HandleLoadingSavingSettingChanged(FName PropertyName);

	/** Set up monitors to all the mounted content directories */
	void SetUpMountedContentDirectories();

	/** Set up the user directories from the editor Loading/Saving settings */
	void SetUpUserDirectories();

	/** Retrieve a semi-colon separated string of file extensions supported by all available editor import factories */
	static FString GetAllFactoryExtensions();

private:

	/** Get the total amount of work in the current operation */
	int32 GetTotalWork() const;

	/** Get the total progress through the current operation */
	int32 GetTotalProgress() const;
	
	/** Get the number of unprocessed changes that are not part of the current processing operation */
	int32 GetNumUnprocessedChanges() const;

private:

	/** Process any remaining pending additions we have */
	void ProcessAdditions(const FTimeLimit& TimeLimit);

	/** Process any remaining pending modifications we have */
	void ProcessModifications(const IAssetRegistry& Registry, const FTimeLimit& TimeLimit);

	/** Process any remaining pending deletions we have */
	void ProcessDeletions(const IAssetRegistry& Registry);

	/** Enum and value to specify the current state of our processing */
	enum class ECurrentState
	{
		Idle, Initialize, ProcessAdditions, SavePackages, ProcessModifications, ProcessDeletions, Cleanup
	};
	ECurrentState CurrentState;

private:

	/** Feedback context that can selectively override the global context to consume progress events for saving of assets */
	TUniquePtr<FReimportFeedbackContext> FeedbackContextOverride;

	/** Cached string of extensions that are supported by all available factories */
	FString SupportedExtensions;

	/** Array of directory monitors for automatically managed mounted content paths */
	TArray<FContentDirectoryMonitor> MountedDirectoryMonitors;

	/** Additional external directories to monitor for changes specified by the user */
	TArray<FContentDirectoryMonitor> UserDirectoryMonitors;

	/** A list of packages to save when we've added a bunch of assets */
	TArray<UPackage*> PackagesToSave;

	/** Set when we are processing changes to avoid reentrant logic for asset deletions etc */
	bool bIsProcessingChanges;

	/** The time when the last change to the cache was reported */
	FTimeLimit StartProcessingDelay;

	/** The cached number of unprocessed changes we currently have to process */
	int32 CachedNumUnprocessedChanges;
};

FAutoReimportManager::FAutoReimportManager()
	: CurrentState(ECurrentState::Idle)
	, bIsProcessingChanges(false)
	, StartProcessingDelay(1)
{
	// @todo: arodham: update this when modules are reloaded or new factory types are available?
	SupportedExtensions = GetAllFactoryExtensions();

	auto* Settings = GetMutableDefault<UEditorLoadingSavingSettings>();
	Settings->OnSettingChanged().AddRaw(this, &FAutoReimportManager::HandleLoadingSavingSettingChanged);

	FPackageName::OnContentPathMounted().AddRaw(this, &FAutoReimportManager::OnContentPathMounted);
	FPackageName::OnContentPathDismounted().AddRaw(this, &FAutoReimportManager::OnContentPathDismounted);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.OnInMemoryAssetDeleted().AddRaw(this, &FAutoReimportManager::OnAssetDeleted);


	// Only set this up for content directories if the user has this enabled
	if (Settings->bMonitorContentDirectories)
	{
		SetUpMountedContentDirectories();
	}

	// We always monitor user directories if they are set
	SetUpUserDirectories();
}

FAutoReimportManager::~FAutoReimportManager() 
{
	if (auto* Settings = GetMutableDefault<UEditorLoadingSavingSettings>())
	{
		Settings->OnSettingChanged().RemoveAll(this);
	}

	FPackageName::OnContentPathMounted().RemoveAll(this);
	FPackageName::OnContentPathDismounted().RemoveAll(this);

	if (FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
	{
		AssetRegistryModule->Get().OnInMemoryAssetDeleted().RemoveAll(this);
	}

	// Force a save of all the caches
	MountedDirectoryMonitors.Empty();
	UserDirectoryMonitors.Empty();
}

int32 FAutoReimportManager::GetNumUnprocessedChanges() const
{
	auto Pred = [](const FContentDirectoryMonitor& Monitor, int32 Total){ return Total + Monitor.GetNumUnprocessedChanges(); };
	return Utils::Reduce(MountedDirectoryMonitors, Pred, 0) + Utils::Reduce(UserDirectoryMonitors, Pred, 0);
}

int32 FAutoReimportManager::GetTotalWork() const
{
	auto Pred = [](const FContentDirectoryMonitor& Monitor, int32 Total){ return Total + Monitor.GetTotalWork(); };
	return Utils::Reduce(MountedDirectoryMonitors, Pred, 0) + Utils::Reduce(UserDirectoryMonitors, Pred, 0);
}

int32 FAutoReimportManager::GetTotalProgress() const
{
	auto Pred = [](const FContentDirectoryMonitor& Monitor, int32 Total){ return Total + Monitor.GetWorkProgress(); };
	return Utils::Reduce(MountedDirectoryMonitors, Pred, 0) + Utils::Reduce(UserDirectoryMonitors, Pred, 0);
}

void FAutoReimportManager::ProcessAdditions(const FTimeLimit& TimeLimit)
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

void FAutoReimportManager::ProcessModifications(const IAssetRegistry& Registry, const FTimeLimit& TimeLimit)
{
	for (auto& Monitor : MountedDirectoryMonitors)
	{
		Monitor.ProcessModifications(Registry, TimeLimit);
		CONDITIONAL_YIELD
	}

	for (auto& Monitor : UserDirectoryMonitors)
	{
		Monitor.ProcessModifications(Registry, TimeLimit);
		CONDITIONAL_YIELD
	}

	CurrentState = ECurrentState::ProcessDeletions;
}

void FAutoReimportManager::ProcessDeletions(const IAssetRegistry& Registry)
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


void FAutoReimportManager::Tick(float DeltaTime)
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

TStatId FAutoReimportManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FAutoReimportManager, STATGROUP_Tickables);
}

void FAutoReimportManager::HandleLoadingSavingSettingChanged(FName PropertyName)
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

void FAutoReimportManager::OnContentPathMounted(const FString& InAssetPath, const FString& FileSystemPath)
{
	MountedDirectoryMonitors.Emplace(FPaths::ConvertRelativePathToFull(FileSystemPath), SupportedExtensions, InAssetPath);
}

void FAutoReimportManager::OnContentPathDismounted(const FString& InAssetPath, const FString& FileSystemPath)
{
	const auto FullDismountedPath = FPaths::ConvertRelativePathToFull(FileSystemPath);

	MountedDirectoryMonitors.RemoveAll([&](const FContentDirectoryMonitor& Monitor){
		return Monitor.GetDirectory() == FullDismountedPath;
	});
}

void FAutoReimportManager::OnAssetDeleted(UObject* Object)
{
	if (bIsProcessingChanges)
	{
		return;
	}

	const IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	bool bCanDelete = true;

	TArray<UObject::FAssetRegistryTag> Tags;
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
			if (Utils::FindAssetsPertainingToFile(Registry, SourceFile).Num() != 0)
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

void FAutoReimportManager::SetUpMountedContentDirectories()
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

void FAutoReimportManager::SetUpUserDirectories()
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

FString FAutoReimportManager::GetAllFactoryExtensions()
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

UAutoReimportManager::UAutoReimportManager(const FObjectInitializer& Init)
	: Super(Init)
{
}

UAutoReimportManager::~UAutoReimportManager()
{
}

void UAutoReimportManager::Initialize()
{
	Implementation.Reset(new FAutoReimportManager);
}

void UAutoReimportManager::BeginDestroy()
{
	Super::BeginDestroy();
	Implementation.Reset();
}


#undef CONDITIONAL_YIELD
#undef LOCTEXT_NAMESPACE

