// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceControlProvider.h"
#include "IPerforceSourceControlWorker.h"
#include "PerforceSourceControlState.h"

DECLARE_DELEGATE_RetVal(FPerforceSourceControlWorkerRef, FGetPerforceSourceControlWorker)

class FPerforceSourceControlProvider : public ISourceControlProvider
{
public:
	/** Constructor */
	FPerforceSourceControlProvider()
		: bServerAvailable(false)
		, PersistentConnection(NULL)
#if PLATFORM_WINDOWS
		, P4apiHandle(NULL)
#endif
	{
	}

	/* ISourceControlProvider implementation */
	virtual void Init(bool bForceConnection = true) OVERRIDE;
	virtual void Close() OVERRIDE;
	virtual FString GetStatusText() const OVERRIDE;
	virtual bool IsEnabled() const OVERRIDE;
	virtual bool IsAvailable() const OVERRIDE;
	virtual const FName& GetName(void) const OVERRIDE;
	virtual ECommandResult::Type GetState( const TArray<FString>& InFiles, TArray< TSharedRef<ISourceControlState, ESPMode::ThreadSafe> >& OutState, EStateCacheUsage::Type InStateCacheUsage ) OVERRIDE;
	virtual void RegisterSourceControlStateChanged( const FSourceControlStateChanged::FDelegate& SourceControlStateChanged ) OVERRIDE;
	virtual void UnregisterSourceControlStateChanged( const FSourceControlStateChanged::FDelegate& SourceControlStateChanged ) OVERRIDE;
	virtual ECommandResult::Type Execute( const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation, const TArray<FString>& InFiles, EConcurrency::Type InConcurrency = EConcurrency::Synchronous, const FSourceControlOperationComplete& InOperationCompleteDelegate = FSourceControlOperationComplete() ) OVERRIDE;
	virtual bool CanCancelOperation( const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation ) const OVERRIDE;
	virtual void CancelOperation( const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe>& InOperation ) OVERRIDE;
	virtual bool UsesLocalReadOnlyState() const OVERRIDE;
	virtual void Tick() OVERRIDE;
	virtual TArray< TSharedRef<class ISourceControlLabel> > GetLabels( const FString& InMatchingSpec ) const OVERRIDE;
	virtual TSharedRef<class SWidget> MakeSettingsWidget() const OVERRIDE;

	/**
	 * Register a worker with the provider.
	 * This is used internally so the provider can maintain a map of all available operations.
	 */
	void RegisterWorker( const FName& InName, const FGetPerforceSourceControlWorker& InDelegate );

	/**
	 * Gets a list of client spec names from the source control provider
	 *
	 * @param	InServerName		Name of server
	 * @param	InUserName			User name
	 * @param	OutWorkspaceList	List of client spec name strings
	 * @param	OutErrorMessages	List of any error messages that may have occurred
	 */
	void GetWorkspaceList(const FString& InServerName, const FString& InUserName, TArray<FString>& OutWorkspaceList, TArray<FString>& OutErrorMessages);

	/** Get the P4 port we will use for connections */
	const FString& GetPort() const;

	/** Get the P4 user we will use for connections */
	const FString& GetUser() const;

	/** Get the P4 client spec we will use for connections */
	const FString& GetClientSpec() const;

	/** Get the P4 ticket we will use for connections */
	const FString& GetTicket() const;

	/** Helper function used to update state cache */
	TSharedRef<FPerforceSourceControlState, ESPMode::ThreadSafe> GetStateInternal(const FString& InFilename);

	/**
	 * Connects to the source control server if the persistent connection is not already established.
	 *
	 * @return true if the connection is established or became established and false if the connection failed.
	 */
	bool EstablishPersistentConnection();

	/** Get the persistent connection, if any */
	class FPerforceConnection* GetPersistentConnection()
	{
		return PersistentConnection;
	}

private:

	/** Helper function used to create a worker for a particular operation */
	TSharedPtr<class IPerforceSourceControlWorker, ESPMode::ThreadSafe> CreateWorker(const FName& InOperationName) const;

	/** 
	 * Logs any messages that a command needs to output.
	 */
	void OutputCommandMessages(const class FPerforceSourceControlCommand& InCommand) const;

	/**
	 * Loads user/SCC information from the INI file.
	 */
	void ParseCommandLineSettings(bool bForceConnection);

	/**
	 * Load the p4api dll's
	 */
	void LoadLibraries();

	/**
	 * Unload the perforce dll's
	 */
	void UnloadLibraries();

	/**
	 * Helper function for running command 'synchronously'.
	 * This really doesn't execute synchronously; rather it adds the command to the queue & does not return until
	 * the command is completed.
	 */
	ECommandResult::Type ExecuteSynchronousCommand(class FPerforceSourceControlCommand& InCommand, const FText& Task, bool bSuppressResponseMsg);

	/**
	 * Run a command synchronously or asynchronously.
	 */
	ECommandResult::Type IssueCommand(class FPerforceSourceControlCommand& InCommand, const bool bSynchronous);

private:
#if PLATFORM_WINDOWS
	/** A handle for the p4api module */
	HMODULE P4apiHandle;
#endif

	/** The ticket we use for login. */
	FString Ticket;

	/** Indicates if source control integration is available or not. */
	bool bServerAvailable;

	/** A pointer to the persistent P4 connection for synchronous operations */
	class FPerforceConnection* PersistentConnection;

	/** State cache */
	TMap<FString, TSharedRef<class FPerforceSourceControlState, ESPMode::ThreadSafe> > StateCache;

	/** The currently registered source control operations */
	TMap<FName, FGetPerforceSourceControlWorker> WorkersMap;

	/** Queue for commands given by the main thread */
	TArray < FPerforceSourceControlCommand* > CommandQueue;

	/** For notifying when the source control states in the cache have changed */
	FSourceControlStateChanged OnSourceControlStateChanged;
};