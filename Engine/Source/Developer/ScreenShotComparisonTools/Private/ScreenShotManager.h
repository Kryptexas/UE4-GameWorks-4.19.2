// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenShotManager.cpp: Implements the FScreenShotManager class.
=============================================================================*/

#pragma once

#include "ImageComparer.h"
#include "Async/Async.h"

class FScreenshotComparisons;

/**
 * Implements the ScreenShotManager that contains screen shot data.
 */
class FScreenShotManager : public IScreenShotManager 
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InMessageBus - The message bus to use.
	 */
	FScreenShotManager( const IMessageBusRef& InMessageBus );

public:

	/**
	* Create some dummy data to test the UI
	*/
	void CreateData();

public:

	//~ Begin IScreenShotManager Interface

	virtual void GenerateLists() override;

	virtual TArray< TSharedPtr<FString> >& GetCachedPlatfomList() override;

	virtual TArray<IScreenShotDataPtr>& GetLists() override;

	virtual TArray< TSharedPtr<FScreenShotDataItem> >& GetScreenshotResults() override;

	virtual void RegisterScreenShotUpdate(const FOnScreenFilterChanged& InDelegate ) override;

	virtual void SetFilter( TSharedPtr< ScreenShotFilterCollection > InFilter ) override;

	virtual void SetDisplayEveryNthScreenshot(int32 NewNth) override;

	virtual TFuture<void> CompareScreensotsAsync() override;

	virtual TFuture<void> ExportScreensotsAsync(FString ExportPath) override;

	virtual TSharedPtr<FComparisonResults> ImportScreensots(FString ImportPath) override;

	//~ End IScreenShotManager Interface

private:
	FString GetDefaultExportDirectory() const;
	FString GetComparisonResultsJsonFilename() const;

	void CompareScreensots();
	void SaveComparisonResults(const FComparisonResults& Results) const;
	bool ExportComparisonResults(FString RootExportFolder);
	void CopyDirectory(const FString& DestDir, const FString& SrcDir);

	TSharedRef<FScreenshotComparisons> GenerateFileList() const;

	// Handles FAutomationWorkerScreenImage messages.
	void HandleScreenShotMessage( const FAutomationWorkerScreenImage& Message, const IMessageContextRef& Context );

private:
	FString ScreenshotApprovedFolder;
	FString ScreenshotIncomingFolder;
	FString ScreenshotDeltaFolder;
	FString ScreenshotResultsFolder;

	// Holds the list of active platforms
	TArray<TSharedPtr<FString> > CachedPlatformList;

	// Holds the messaging endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds the array of created screen shot data items
	TArray< TSharedPtr<FScreenShotDataItem> > ScreenShotDataArray;

	// Holds the root of the screen shot tree
	TSharedPtr< IScreenShotData > ScreenShotRoot;

	// Holds a delegate to be invoked when the screen shot filter has changed.
	FOnScreenFilterChanged ScreenFilterChangedDelegate;
};
