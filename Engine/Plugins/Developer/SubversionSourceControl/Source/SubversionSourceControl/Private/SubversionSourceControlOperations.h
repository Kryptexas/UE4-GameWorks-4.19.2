// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISubversionSourceControlWorker.h"
#include "SubversionSourceControlUtils.h"

class FSubversionConnectWorker : public ISubversionSourceControlWorker
{
public:
	// ISubversionSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FSubversionSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;
};

class FSubversionCheckOutWorker : public ISubversionSourceControlWorker
{
public:
	// ISubversionSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FSubversionSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Temporary states for results */
	TArray<FSubversionSourceControlState> OutStates;
};

class FSubversionCheckInWorker : public ISubversionSourceControlWorker
{
public:
	// ISubversionSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FSubversionSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Temporary states for results */
	TArray<FSubversionSourceControlState> OutStates;
};

class FSubversionMarkForAddWorker : public ISubversionSourceControlWorker
{
public:
	// ISubversionSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FSubversionSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Temporary states for results */
	TArray<FSubversionSourceControlState> OutStates;
};

class FSubversionDeleteWorker : public ISubversionSourceControlWorker
{
public:
	// ISubversionSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FSubversionSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to Subversion state */
	TArray<FSubversionSourceControlState> OutStates;
};

class FSubversionRevertWorker : public ISubversionSourceControlWorker
{
public:
	// ISubversionSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FSubversionSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to Subversion state */
	TArray<FSubversionSourceControlState> OutStates;
};

class FSubversionSyncWorker : public ISubversionSourceControlWorker
{
public:
	// ISubversionSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FSubversionSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Map of filenames to Subversion state */
	TArray<FSubversionSourceControlState> OutStates;
};

class FSubversionUpdateStatusWorker : public ISubversionSourceControlWorker
{
public:
	// ISubversionSourceControlWorker interface
	virtual FName GetName() const OVERRIDE;
	virtual bool Execute(class FSubversionSourceControlCommand& InCommand) OVERRIDE;
	virtual bool UpdateStates() const OVERRIDE;

public:
	/** Temporary states for results */
	TArray<FSubversionSourceControlState> OutStates;

	/** Map of filenames to history */
	SubversionSourceControlUtils::FHistoryOutput OutHistory;
};
