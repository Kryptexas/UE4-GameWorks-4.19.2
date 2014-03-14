// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FDirectoryWatcherWindows : public IDirectoryWatcher
{
public:
	FDirectoryWatcherWindows();
	virtual ~FDirectoryWatcherWindows();

	virtual bool RegisterDirectoryChangedCallback (const FString& Directory, const FDirectoryChanged& InDelegate) OVERRIDE;
	virtual bool UnregisterDirectoryChangedCallback (const FString& Directory, const FDirectoryChanged& InDelegate) OVERRIDE;
	virtual void Tick (float DeltaSeconds) OVERRIDE;

	/** Map of directory paths to requests */
	TMap<FString, FDirectoryWatchRequestWindows*> RequestMap;
	TArray<FDirectoryWatchRequestWindows*> RequestsPendingDelete;

	/** A count of FDirectoryWatchRequestWindows created to ensure they are cleaned up on shutdown */
	int32 NumRequests;
};

typedef FDirectoryWatcherWindows FDirectoryWatcher;
