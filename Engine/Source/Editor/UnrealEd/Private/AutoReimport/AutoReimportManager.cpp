// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "AutoReimportUtilities.h"
#include "AutoReimport/AutoReimportManager.h"
#include "ContentDirectoryMonitor.h"

#include "ObjectTools.h"
#include "ARFilter.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "ReimportFeedbackContext.h"
#include "MessageLogModule.h"

#define LOCTEXT_NAMESPACE "AutoReimportManager"
#define yield if (TimeLimit.Exceeded()) return

DEFINE_LOG_CATEGORY_STATIC(LogAutoReimportManager, Log, All);


enum class EStateMachineNode { CallOnce, CallMany };

/** A simple state machine class that calls functions mapped on enum values. If any function returns a new enum type, it moves onto that function */
template<typename TState>
struct FStateMachine
{
	typedef TFunction<TOptional<TState>(const FTimeLimit&)> FunctionType;

	/** Constructor that specifies the initial state of the machine */
	FStateMachine(TState InitialState) : CurrentState(InitialState){}

	/** Add an enum->function mapping for this state machine */
	void Add(TState State, EStateMachineNode NodeType, const FunctionType& Function)
	{
		Nodes.Add(State, FStateMachineNode(NodeType, Function));
	}

	/** Set a new state for this machine */
	void SetState(TState NewState)
	{
		CurrentState = NewState;
	}

	/** Tick this state machine with the given time limit. Will continuously enumerate the machine until TimeLimit is reached */
	void Tick(const FTimeLimit& TimeLimit)
	{
		while (!TimeLimit.Exceeded())
		{
			const auto& State = Nodes[CurrentState];
			TOptional<TState> NewState = State.Endpoint(TimeLimit);
			if (NewState.IsSet())
			{
				CurrentState = NewState.GetValue();
			}
			else if (State.Type == EStateMachineNode::CallOnce)
			{
				break;
			}
		}
	}

private:

	/** The current state of this machine */
	TState CurrentState;

private:

	struct FStateMachineNode
	{
		FStateMachineNode(EStateMachineNode InType, const FunctionType& InEndpoint) : Endpoint(InEndpoint), Type(InType) {}

		/** The function endpoint for this node */
		FunctionType Endpoint;

		/** Whether this endpoint should be called multiple times in a frame, or just once */
		EStateMachineNode Type;
	};

	/** A map of enum value -> callback information */
	TMap<TState, FStateMachineNode> Nodes;
};

/** Enum and value to specify the current state of our processing */
enum class ECurrentState
{
	Idle, PendingResponse, ProcessAdditions, SavePackages, ProcessModifications, ProcessDeletions
};
uint32 GetTypeHash(ECurrentState State)
{
	return (int32)State;
}

/* Deals with auto reimporting of objects when the objects file on disk is modified*/
class FAutoReimportManager : public FTickableEditorObject, public FGCObject, public TSharedFromThis<FAutoReimportManager>
{
public:

	FAutoReimportManager();
	void Destroy();

private:

	/** FTickableEditorObject interface*/
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

	/** FGCObject interface*/
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

private:

	/** Called when a new asset path has been mounted */
	void OnContentPathMounted(const FString& InAssetPath, const FString& FileSystemPath);

	/** Called when a new asset path has been dismounted */
	void OnContentPathDismounted(const FString& InAssetPath, const FString& FileSystemPath);

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
	
	/** Get the number of unprocessed changes that are not part of the current processing operation */
	int32 GetNumUnprocessedChanges() const;

	/** Get the progress text to display on the context notification */
	FText GetProgressText() const;

private:

	/** A state machine holding information about the current state of the manager */
	FStateMachine<ECurrentState> StateMachine;

private:

	/** Idle processing */
	TOptional<ECurrentState> Idle(const FTimeLimit& Limit);
	
	/** Process any remaining pending additions we have */
	TOptional<ECurrentState> ProcessAdditions(const FTimeLimit& TimeLimit);

	/** Save any packages that were created inside ProcessAdditions */
	TOptional<ECurrentState> SavePackages();

	/** Process any remaining pending modifications we have */
	TOptional<ECurrentState> ProcessModifications(const FTimeLimit& TimeLimit);

	/** Process any remaining pending deletions we have */
	TOptional<ECurrentState> ProcessDeletions();

	/** Wait for a user's input. Just updates the progress text for now */
	TOptional<ECurrentState> PendingResponse();

	/** Cleanup an operation that just processed some changes */
	void Cleanup();

private:

	/** Feedback context that can selectively override the global context to consume progress events for saving of assets */
	TSharedPtr<FReimportFeedbackContext> FeedbackContextOverride;

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
	: StateMachine(ECurrentState::Idle)
	, bIsProcessingChanges(false)
	, StartProcessingDelay(0.5)
{
	// @todo: arodham: update this when modules are reloaded or new factory types are available?
	SupportedExtensions = GetAllFactoryExtensions();

	auto* Settings = GetMutableDefault<UEditorLoadingSavingSettings>();
	Settings->OnSettingChanged().AddRaw(this, &FAutoReimportManager::HandleLoadingSavingSettingChanged);

	FPackageName::OnContentPathMounted().AddRaw(this, &FAutoReimportManager::OnContentPathMounted);
	FPackageName::OnContentPathDismounted().AddRaw(this, &FAutoReimportManager::OnContentPathDismounted);

	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	if (!MessageLogModule.IsRegisteredLogListing("AssetReimport"))
	{
		MessageLogModule.RegisterLogListing("AssetReimport", LOCTEXT("AssetReimportLabel", "Asset Reimport"));
	}

	// Only set this up for content directories if the user has this enabled
	if (Settings->bMonitorContentDirectories)
	{
		SetUpMountedContentDirectories();
	}

	// We always monitor user directories if they are set
	SetUpUserDirectories();

	StateMachine.Add(ECurrentState::Idle,					EStateMachineNode::CallOnce,	[this](const FTimeLimit& T){ return this->Idle(T);					});
	StateMachine.Add(ECurrentState::PendingResponse,		EStateMachineNode::CallOnce,	[this](const FTimeLimit& T){ return this->PendingResponse();		});
	StateMachine.Add(ECurrentState::ProcessAdditions,		EStateMachineNode::CallMany,	[this](const FTimeLimit& T){ return this->ProcessAdditions(T);		});
	StateMachine.Add(ECurrentState::SavePackages,			EStateMachineNode::CallOnce,	[this](const FTimeLimit& T){ return this->SavePackages();			});
	StateMachine.Add(ECurrentState::ProcessModifications,	EStateMachineNode::CallMany,	[this](const FTimeLimit& T){ return this->ProcessModifications(T);	});
	StateMachine.Add(ECurrentState::ProcessDeletions,		EStateMachineNode::CallMany,	[this](const FTimeLimit& T){ return this->ProcessDeletions();		});
}

void FAutoReimportManager::Destroy()
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

FText FAutoReimportManager::GetProgressText() const
{
	FFormatOrderedArguments Args;
	{
		auto Pred = [](const FContentDirectoryMonitor& Monitor, int32 Total){ return Total + Monitor.GetWorkProgress(); };
		const int32 Progress = Utils::Reduce(MountedDirectoryMonitors, Pred, 0) + Utils::Reduce(UserDirectoryMonitors, Pred, 0);

		Args.Add(Progress);
	}

	{
		auto Pred = [](const FContentDirectoryMonitor& Monitor, int32 Total){ return Total + Monitor.GetTotalWork(); };
		const int32 Total = Utils::Reduce(MountedDirectoryMonitors, Pred, 0) + Utils::Reduce(UserDirectoryMonitors, Pred, 0);

		Args.Add(Total);
	}

	return FText::Format(LOCTEXT("ProcessingChanges", "Processing outstanding content changes ({0} of {1})..."), Args);
}

TOptional<ECurrentState> FAutoReimportManager::ProcessAdditions(const FTimeLimit& TimeLimit)
{
	FeedbackContextOverride->GetContent()->SetMainText(GetProgressText());

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
		Monitor.ProcessAdditions(PackagesToSave, TimeLimit, Factories, *FeedbackContextOverride);
		yield TOptional<ECurrentState>();
	}

	for (auto& Monitor : UserDirectoryMonitors)
	{
		Monitor.ProcessAdditions(PackagesToSave, TimeLimit, Factories, *FeedbackContextOverride);
		yield TOptional<ECurrentState>();
	}

	return ECurrentState::SavePackages;
}

TOptional<ECurrentState> FAutoReimportManager::SavePackages()
{
	// We don't override the context specifically when saving packages so the user gets proper feedback
	//TGuardValue<FFeedbackContext*> ScopedContextOverride(GWarn, FeedbackContextOverride.Get());

	FeedbackContextOverride->GetContent()->SetMainText(GetProgressText());

	if (PackagesToSave.Num() > 0)
	{
		const bool bAlreadyCheckedOut = false;
		const bool bCheckDirty = false;
		const bool bPromptToSave = false;
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave, nullptr, bAlreadyCheckedOut);

		PackagesToSave.Empty();
	}

	return ECurrentState::ProcessModifications;
}

TOptional<ECurrentState> FAutoReimportManager::ProcessModifications(const FTimeLimit& TimeLimit)
{
	// Override the global feedback context while we do this to avoid popping up dialogs
	TGuardValue<FFeedbackContext*> ScopedContextOverride(GWarn, FeedbackContextOverride.Get());

	FeedbackContextOverride->GetContent()->SetMainText(GetProgressText());

	const IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	for (auto& Monitor : MountedDirectoryMonitors)
	{
		Monitor.ProcessModifications(Registry, TimeLimit, *FeedbackContextOverride);
		yield TOptional<ECurrentState>();
	}

	for (auto& Monitor : UserDirectoryMonitors)
	{
		Monitor.ProcessModifications(Registry, TimeLimit, *FeedbackContextOverride);
		yield TOptional<ECurrentState>();
	}

	return ECurrentState::ProcessDeletions;
}

TOptional<ECurrentState> FAutoReimportManager::ProcessDeletions()
{
	const IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FeedbackContextOverride->GetContent()->SetMainText(GetProgressText());

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

	Cleanup();
	return ECurrentState::Idle;
}

TOptional<ECurrentState> FAutoReimportManager::PendingResponse()
{
	FeedbackContextOverride->GetContent()->SetMainText(GetProgressText());
	return TOptional<ECurrentState>();
}

TOptional<ECurrentState> FAutoReimportManager::Idle(const FTimeLimit& TimeLimit)
{
	for (auto& Monitor : MountedDirectoryMonitors)
	{
		Monitor.Tick(TimeLimit);
		yield TOptional<ECurrentState>();
	}
	for (auto& Monitor : UserDirectoryMonitors)
	{
		Monitor.Tick(TimeLimit);
		yield TOptional<ECurrentState>();
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
				// Create a new feedback context override
				FeedbackContextOverride = MakeShareable(new FReimportFeedbackContext);
				FeedbackContextOverride->Initialize(SNew(SReimportFeedback, GetProgressText()));

				// Deal with changes to the file system second
				for (auto& Monitor : MountedDirectoryMonitors)
				{
					Monitor.StartProcessing();
				}
				for (auto& Monitor : UserDirectoryMonitors)
				{
					Monitor.StartProcessing();
				}

				return ECurrentState::ProcessAdditions;
			}
		}
	}

	return TOptional<ECurrentState>();
}

void FAutoReimportManager::Cleanup()
{
	CachedNumUnprocessedChanges = 0;

	FeedbackContextOverride->Destroy();
	FeedbackContextOverride = nullptr;
}

void FAutoReimportManager::Tick(float DeltaTime)
{
	if (FeedbackContextOverride.IsValid())
	{
		FeedbackContextOverride->Tick();
	}
	
	// Never spend more than a 60fps frame doing this work (meaning we shouldn't drop below 30fps), we can do more if we're throttling CPU usage (ie editor running in background)
	const FTimeLimit TimeLimit(GEditor->ShouldThrottleCPUUsage() ? 1 / 6.f : 1.f / 60.f);
	StateMachine.Tick(TimeLimit);
}

TStatId FAutoReimportManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FAutoReimportManager, STATGROUP_Tickables);
}

void FAutoReimportManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (auto* Package : PackagesToSave)
	{
		Collector.AddReferencedObject(Package);
	}
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
	Implementation = MakeShareable(new FAutoReimportManager);
}

void UAutoReimportManager::BeginDestroy()
{
	Super::BeginDestroy();
	if (Implementation.IsValid())
	{
		Implementation->Destroy();
		Implementation = nullptr;
	}
}


#undef CONDITIONAL_YIELD
#undef LOCTEXT_NAMESPACE

