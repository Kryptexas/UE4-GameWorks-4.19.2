// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FileCache.h"

#include "AutoReimportManager.generated.h"

class IAssetRegistry;
class IAssetTools;

/** Feedback context that overrides GWarn for import operations to prevent popup spam */
struct FReimportFeedbackContext : public FFeedbackContext
{
	/** Constructor - created when we are processing changes */
	FReimportFeedbackContext();
	/** Destructor - called when we are done processing changes */
	~FReimportFeedbackContext();

	/** Update the text on the notification */
	void UpdateText(int32 Progress, int32 Total);

private:
	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) {}

private:
	/** The notification that is shown when the context is active */
	TSharedPtr<SNotificationItem> Notification;
};

/** Class responsible for watching a specific content directory for changes */
class FContentDirectoryMonitor
{
public:

	/**
	 * Constructor.
	 * @param InDirectory			Content directory path to monitor. Assumed to be absolute.
	 * @param InSupportedExtensions A string containing semi-colon separated extensions to monitor.
	 * @param InMountedContentPath	(optional) Mounted content path (eg /Engine/, /Game/) to which InDirectory maps.
	 */
	FContentDirectoryMonitor(const FString& InDirectory, const FString& InSupportedExtensions, const FString& InMountedContentPath = FString());

	/** Tick this monitor's cache to give it a chance to finish scanning for files */
	void Tick(const FTimeLimit& TimeLimit);

	/** Start processing any outstanding changes this monitor is aware of */
	void StartProcessing();

	/** Extract the files we need to import from our outstanding changes (happens first)*/ 
	void ProcessAdditions(TArray<UPackage*>& OutPackagesToSave, const FTimeLimit& TimeLimit, const TMap<FString, TArray<UFactory*>>& InFactoriesByExtension);

	/** Process the outstanding changes that we have cached */
	void ProcessModifications(const IAssetRegistry& Registry, const FTimeLimit& TimeLimit, FReimportFeedbackContext& Context);

	/** Extract the assets we need to delete from our outstanding changes (happens last) */ 
	void ExtractAssetsToDelete(const IAssetRegistry& Registry, TArray<FAssetData>& OutAssetsToDelete);

	/** Destroy this monitor including its cache */
	void Destroy();

	/** Get the directory that this monitor applies to */
	const FString& GetDirectory() const { return Cache.GetDirectory(); }

public:

	/** Get the number of outstanding changes that we potentially have to process (when not already processing) */
	int32 GetNumUnprocessedChanges() const { return Cache.GetNumOutstandingChanges(); }

	/** Get the total amount of work this monitor has to perform in the current processing operation */
	int32 GetTotalWork() const { return TotalWork; }

	/** Get the total amount of work this monitor has performed in the current processing operation */
	int32 GetWorkProgress() const { return WorkProgress; }

private:

	/** The file cache that monitors and reflects the content directory. */
	FFileCache Cache;

	/** The mounted content path for this monitor (eg /Game/) */
	FString MountedContentPath;

	/** A list of file system changes that are due to be processed */
	TArray<FUpdateCacheTransaction> AddedFiles, ModifiedFiles, DeletedFiles;

	/** The number of changes we've processed out of the OutstandingChanges array */
	int32 TotalWork, WorkProgress;

	/** The last time we attempted to save the cache file */
	double LastSaveTime;

	/** The interval between potential re-saves of the cache file */
	static const int32 ResaveIntervalS = 60;
};

/* Deals with auto reimporting of objects when the objects file on disk is modified*/
UCLASS(config=Editor, transient)
class UNREALED_API UAutoReimportManager : public UObject, public FTickableEditorObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Initialize during engine startup */
	void Initialize();

private:

	/** FTickableEditorObject interface*/
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

private:

	/** UObject Interface */
	virtual void BeginDestroy() override;

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
