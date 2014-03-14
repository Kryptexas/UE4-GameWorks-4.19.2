// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Async task for gathering asset data from from the file list in FAssetRegistry
 */
class FAssetDataGatherer : public FRunnable
{
public:
	/** Constructor */
	FAssetDataGatherer(const TArray<FString>& Paths, bool bInIsSynchronous);

	// FRunnable implementation
	virtual bool Init() OVERRIDE;
	virtual uint32 Run() OVERRIDE;
	virtual void Stop() OVERRIDE;
	virtual void Exit() OVERRIDE;

	/** Signals to end the thread and waits for it to close before returning */
	void EnsureCompletion();

	/** Gets search results from the data gatherer */
	bool GetAndTrimSearchResults(TArray<FBackgroundAssetData*>& OutAssetResults, TArray<FString>& OutPathResults, TArray<FPackageDependencyData>& OutDependencyResults, TArray<double>& OutSearchTimes, int32& OutNumFilesToSearch);

	/** Adds a root path to the search queue. Only works when searching asynchronously */
	void AddPathToSearch(const FString& Path);

	/** Adds specific files to the search queue. Only works when searching asynchronously */
	void AddFilesToSearch(const TArray<FString>& Files);

private:
	
	/** This function is run on the gathering thread, unless synchronous */
	void DiscoverFilesToSearch();

	/** Returns true if this package file has only valid characters and can exist in a content root */
	bool IsValidPackageFileToRead(const FString& Filename) const;

	/**
	 * Reads FAssetData information out of a file
	 *
	 * @param AssetFilename the name of the file to read
	 * @param AssetDataList the FBackgroundAssetData for every asset found in the file
	 * @param DependencyData the FPackageDependencyData for every asset found in the file
	 *
	 * @return true if the file was successfully read
	 */
	bool ReadAssetFile(const FString& AssetFilename, TArray<FBackgroundAssetData*>& AssetDataList, FPackageDependencyData& DependencyData) const;

private:
	/** A critical section to protect data transfer to the main thread */
	FCriticalSection WorkerThreadCriticalSection;

	/** List of files that need to be processed by the search. It is not threadsafe to directly access this array */
	TArray<FString> FilesToSearch;

	/** > 0 if we've been asked to abort work in progress at the next opportunity */
	FThreadSafeCounter StopTaskCounter;

	/** True if this gather request is synchronous (i.e, IsRunningCommandlet()) */
	bool bIsSynchronous;

	/** The current search start time */
	double SearchStartTime;

	/** The input base paths in which to discover assets and paths */
	// IMPORTANT: This variable may be modified by from a different thread via a call to AddPathToSearch(), so access 
	//            to this array should be handled very carefully.
	TArray<FString> PathsToSearch;

	/** The asset data gathered from the searched files */
	TArray<FBackgroundAssetData*> AssetResults;

	/** Dependency data for scanned packages */
	TArray<FPackageDependencyData> DependencyResults;

	/** All the search times since the last main thread tick */
	TArray<double> SearchTimes;

	/** The paths found during the search */
	TArray<FString> DiscoveredPaths;

	/** True if dependency data should be gathered */
	bool bGatherDependsData;

	/** Set of characters that are invalid in packages, thus files containing them can not be loaded. */
	TSet<TCHAR> InvalidAssetFileCharacters;

public:
	/** Thread to run the cleanup FRunnable on */
	FRunnableThread* Thread;
};