// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPerforceSourceControlWorker.h"
#include "PerforceSourceControlState.h"

class FPerforceConnectWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;
};

class FPerforceCheckOutWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to perforce state */
	TMap<FString, EPerforceState::Type> OutResults;
};

class FPerforceCheckInWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to perforce state */
	TMap<FString, EPerforceState::Type> OutResults;
};

class FPerforceMarkForAddWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to perforce state */
	TMap<FString, EPerforceState::Type> OutResults;
};

class FPerforceDeleteWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to perforce state */
	TMap<FString, EPerforceState::Type> OutResults;
};

class FPerforceRevertWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to perforce state */
	TMap<FString, EPerforceState::Type> OutResults;
};

class FPerforceSyncWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to perforce state */
	TMap<FString, EPerforceState::Type> OutResults;
};

class FPerforceUpdateStatusWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Temporary states for results */
	TArray<FPerforceSourceControlState> OutStates;

	/** Map of filename->state */
	TMap<FString, EPerforceState::Type> OutStateMap;

	/** Map of filenames to history */
	typedef TMap<FString, TArray< TSharedRef<FPerforceSourceControlRevision, ESPMode::ThreadSafe> > > FHistoryMap;
	FHistoryMap OutHistory;

	/** Map of filenames to modified flag */
	TArray<FString> OutModifiedFiles;
};

class FPerforceGetWorkspacesWorker : public IPerforceSourceControlWorker
{
public:
	// IPerforceSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FPerforceSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;
};