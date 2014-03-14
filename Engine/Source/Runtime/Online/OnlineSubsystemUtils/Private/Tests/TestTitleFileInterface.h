// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineTitleFileInterface.h"

class FTestTitleFileInterface
{
public:
	void Test();

	/**
	 * Sets the subsystem name to test
	 *
	 * @param InSubsystemName the subsystem to test
	 */
	FTestTitleFileInterface(const FString& InSubsystemName);

private:
	void FinishTest();

	/** The subsystem that was requested to be tested or the default if empty */
	const FString SubsystemName;

	/** The online interface to use for testing */
	IOnlineTitleFilePtr OnlineTitleFile;

	/** Title file list enumeration complete delegate */
	FOnEnumerateFilesCompleteDelegate OnEnumerateFilesCompleteDelegate;
	/** Title file download complete delegate */
	FOnReadFileCompleteDelegate OnReadFileCompleteDelegate;

	void OnEnumerateFilesComplete(bool bSuccess);

	void OnReadFileComplete(bool bSuccess, const FString& Filename);

	/** Async file reads still pending completion */
	int32 NumPendingFileReads;
};