// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenShotManager.cpp: Implements the FScreenShotManager class.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "ImageComparer.h"
#include "Interfaces/IScreenShotManager.h"
#include "MessageEndpoint.h"

DECLARE_LOG_CATEGORY_EXTERN(LogScreenShotManager, Log, All);

class FScreenshotComparisons;
struct FScreenShotDataItem;

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
	FScreenShotManager();

public:

	//~ Begin IScreenShotManager Interface

	virtual TFuture<FImageComparisonResult> CompareScreenshotAsync(FString RelativeImagePath) override;

	virtual TFuture<FScreenshotExportResults> ExportComparisonResultsAsync(FString ExportPath = TEXT("")) override;

	virtual bool OpenComparisonReports(FString ImportPath, TArray<FComparisonReport>& OutReports) override;

	virtual FString GetLocalUnapprovedFolder() const override;

	virtual FString GetLocalApprovedFolder() const override;

	virtual FString GetLocalComparisonFolder() const override;

	//~ End IScreenShotManager Interface

private:
	FString GetDefaultExportDirectory() const;

	FImageComparisonResult CompareScreenshot(FString Existing);
	FScreenshotExportResults ExportComparisonResults(FString RootExportFolder);
	void CopyDirectory(const FString& DestDir, const FString& SrcDir);

	void BuildFallbackPlatformsListFromConfig();

private:
	FString ScreenshotApprovedFolder;
	FString ScreenshotUnapprovedFolder;
	FString ScreenshotDeltaFolder;
	FString ScreenshotResultsFolder;
	FString ComparisonResultsFolder;
	TMap<FString, FString> FallbackPlatforms;
};
