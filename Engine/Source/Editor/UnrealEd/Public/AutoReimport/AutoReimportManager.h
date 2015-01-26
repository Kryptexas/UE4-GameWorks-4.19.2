// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FileCache.h"

#include "AutoReimportManager.generated.h"

class IAssetRegistry;

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

	/** Tick this directory monitor. Will potentially process changed files */
	void Tick(const IAssetRegistry& Registry, const FWorkLimiter& Limiter);

	/** Destroy this monitor including its cache */
	void Destroy();

	/** Get the directory that this monitor applies to */
	const FString& GetDirectory() const { return Cache.GetDirectory(); }

	/** Get the mounted content path that this monitor applies to */
	const FString& GetMountedContentPath() const { return MountedContentPath; }

private:

	/** Process the outstanding changes that we have cached */
	void ProcessOutstandingChanges(const IAssetRegistry& Registry, const FWorkLimiter& Limiter);

private:

	/** The file cache that monitors and reflects the content directory. */
	FFileCache Cache;

	/** The mounted content path for this monitor (eg /Engine/) */
	FString MountedContentPath;

	/** A list of file system changes that are due to be processed */
	TArray<FUpdateCacheTransaction> OutstandingChanges;

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

	/** Cached string of extensions that are supported by all available factories */
	FString SupportedExtensions;

	/** Array of directory monitors for automatically managed mounted content paths */
	TArray<FContentDirectoryMonitor> MountedDirectoryMonitors;

	/** Additional external directories to monitor for changes specified by the user */
	TArray<FContentDirectoryMonitor> UserDirectoryMonitors;

	/** Set when we are processing changes to avoid reentrant logic for asset deletions etc */
	bool bIsProcessingChanges;
};
