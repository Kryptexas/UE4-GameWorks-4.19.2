// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FP4Label
{
public:
	FP4Label(const FString& Name, const FDateTime& Date);

	const FString& GetName() const;
	const FDateTime& GetDate() const;

private:
	FString Name;
	FDateTime Date;
};

/**
 * Class for storing cached P4 data.
 */
class FP4DataCache
{
public:
	/**
	 * Loads P4 data from UnrealSyncList log.
	 *
	 * @returns True if succeeded. False otherwise.
	 */
	bool LoadFromLog(const FString& UnrealSyncListLog);

	/**
	 * Gets labels stored in this cache object.
	 *
	 * @returns Reference to array of labels.
	 */
	const TArray<FP4Label>& GetLabels();

private:
	/* Array object with labels. */
	TArray<FP4Label> Labels;
};

/**
 * Class to store info about thread running and controling P4 data loading process.
 */
class FP4DataLoader : public FRunnable
{
public:
	/* Delagate for loading finished event. */
	DECLARE_DELEGATE_OneParam(FOnLoadingFinished, TSharedPtr<FP4DataCache>);

	/**
	 * Constructor
	 *
	 * @param OnLoadingFinished Delegate to run when loading finished.
	 */
	FP4DataLoader(FOnLoadingFinished OnLoadingFinished);

	virtual ~FP4DataLoader();

	/**
	 * Main function to run for this thread.
	 *
	 * @returns Exit code.
	 */
	uint32 Run() OVERRIDE;

	/**
	 * Signals that the loading process should be terminated.
	 */
	void Terminate();

private:
	/* Delegate to run when loading is finished. */
	FOnLoadingFinished OnLoadingFinished;

	/* Handle for thread object. */
	FRunnableThread *Thread;

	/* Signals if the loading process should be terminated. */
	bool bTerminate;
};

