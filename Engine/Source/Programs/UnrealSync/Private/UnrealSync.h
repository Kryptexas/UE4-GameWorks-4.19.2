// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "CoreUObject.h"
#include "P4DataCache.h"

/**
 * Main program function.
 */
void RunUnrealSync(const TCHAR* CommandLine);

/**
 * Helper class with functions used to sync engine.
 */
class FUnrealSync
{
public:
	/* On data refresh event delegate. */
	DECLARE_DELEGATE(FOnDataRefresh);

	/* On sync finished event delegate. */
	DECLARE_DELEGATE_OneParam(FOnSyncFinished, bool);
	/* On sync log chunk read event delegate. */
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnSyncProgress, const FString&);

	/**
	 * Gets latest label for given game name.
	 *
	 * @param GameName Current game name.
	 *
	 * @returns Latest label for current game.
	 */
	static FString GetLatestLabelForGame(const FString& GameName);

	/**
	 * Gets promoted labels for given game.
	 *
	 * @param GameName Current game name.
	 *
	 * @returns Array of promoted labels for given game.
	 */
	static TSharedPtr<TArray<FString> > GetPromotedLabelsForGame(const FString& GameName);

	/**
	 * Gets promotable labels for given game since last promoted.
	 *
	 * @param GameName Current game name.
	 *
	 * @returns Array of promotable labels for given game since last promoted.
	 */
	static TSharedPtr<TArray<FString> > GetPromotableLabelsForGame(const FString& GameName);

	/**
	 * Gets all labels.
	 *
	 * @returns Array of all labels names.
	 */
	static TSharedPtr<TArray<FString> > GetAllLabels();

	/**
	 * Gets possible game names.
	 *
	 * @returns Array of possible game names.
	 */
	static TSharedPtr<TArray<FString> > GetPossibleGameNames();

	/**
	 * Gets shared promotable display name.
	 *
	 * @returns Shared promotable display name.
	 */
	static const FString& GetSharedPromotableDisplayName();

	/**
	 * Registers event that will be trigger when data is refreshed.
	 *
	 * @param OnDataRefresh Delegate to call when event happens.
	 */
	static void RegisterOnDataRefresh(FOnDataRefresh OnDataRefresh);

	/**
	 * Start async loading of the P4 label data in case user wants it.
	 */
	static void StartLoadingData();

	/**
	 * Tells that labels names are currently being loaded.
	 *
	 * @returns True if labels names are currently being loaded. False otherwise.
	 */
	static bool IsLoadingInProgress()
	{
		return LoaderThread.IsValid() && LoaderThread->IsInProgress();
	}

	/**
	 * Terminates background P4 data loading process.
	 */
	static void TerminateLoadingProcess();

	/**
	 * Method to receive p4 data loading finished event.
	 *
	 * @param Data Loaded data.
	 */
	static void OnP4DataLoadingFinished(TSharedPtr<FP4DataCache> Data);

	/**
	 * Tells if has valid P4 data loaded.
	 *
	 * @returns If P4 data was loaded.
	 */
	static bool HasValidData();

	/**
	 * Tells if loading has finished.
	 *
	 * @returns True if loading has finished. False otherwise.
	 */
	static bool LoadingFinished();

	/**
	 * Gets labels from the loaded cache.
	 *
	 * @returns Array of loaded labels.
	 */
	static const TArray<FP4Label>& GetLabels() { return Data->GetLabels(); }

	/**
	 * Launches UAT UnrealSync command with given command line and options.
	 *
	 * @param bArtist Perform artist sync?
	 * @param bPreview Perform preview sync?
	 * @param CommandLine Command line to run.
	 * @param OnSyncFinished Delegate to run when syncing is finished.
	 * @param OnSyncProgress Delegate to run when syncing has made progress.
	 */
	static void LaunchSync(bool bArtist, bool bPreview, const FString& CommandLine, FOnSyncFinished OnSyncFinished, FOnSyncProgress OnSyncProgress);

private:
	/* Tells if loading has finished. */
	static bool bLoadingFinished;

	/* Data refresh event. */
	static FOnDataRefresh OnDataRefresh;

	/* Cached data ptr. */
	static TSharedPtr<FP4DataCache> Data;

	/* Background loading process monitoring thread. */
	static TSharedPtr<FP4DataLoader> LoaderThread;
};
