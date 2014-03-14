// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceControlProvider.h"
#include "PerforceSourceControlCommand.h"

/** A map containing result of running Perforce command */
class FP4Record : public TMap<FString, FString >
{
public:

	virtual ~FP4Record() {}

	FString operator () (const FString& Key) const
	{
		const FString* Found = Find(Key);
		return Found ? *Found : TEXT("");
	}
};

typedef TArray<FP4Record> FP4RecordSet;

class FPerforceConnection
{
public:
	//This constructor is strictly for internal questions to perforce (get client spec list, etc)
	FPerforceConnection(const FString& InServerName, const FString& InUserName, const FString& InWorkspaceName, const FString& InTicket);
	/** API Specific close of source control project*/
	~FPerforceConnection();

	/** 
	 * Attempts to automatically detect the workspace to use based on the working directory
	 */
	static bool AutoDetectWorkspace(const FString& InPortName, const FString& InUserName, FString& OutWorkspaceName, const FString& InTicket);

	/**
	 * Static function in charge of making sure the specified connection is valid or requests that data from the user via dialog
	 * @param InOutPortName - Port name in the inifile.  Out value is the port name from the connection dialog
	 * @param InOutUserName - User name in the inifile.  Out value is the user name from the connection dialog
	 * @param InOutWorkspaceName - Workspace name in the inifile.  Out value is the client spec from the connection dialog
	 * @param InTicket - The ticket to use as a password when talking to Perforce.
	 * @return - true if the connection, whether via dialog or otherwise, is valid.  False if source control should be disabled
	 */
	static bool EnsureValidConnection(FString& InOutServerName, FString& InOutUserName, FString& InOutWorkspaceName, const FString& InTicket);

	/**
	 * Get List of ClientSpecs
	 * @param InUserName		The username who should own the workspaces in the list
	 * @param InOnIsCancelled	Delegate called to check for operation cancellation.
	 * @param OutWorkspaceList	The workspace list output.
	 * @param OutErrorMessages	Any error messages output.
	 * @return - List of client spec names
	 */
	bool GetWorkspaceList(const FString& InUserName, FOnIsCancelled InOnIsCancelled, TArray<FString>& OutWorkspaceList, TArray<FString>& OutErrorMessages);

	/** Returns true if connection is currently active */
	bool IsValidConnection();

	/** If the connection is valid, disconnect from the server */
	void Disconnect();

	/**
	 * Runs internal perforce command, catches exceptions, returns results
	 */
	bool RunCommand(const FString& InCommand, const TArray<FString>& InParameters, FP4RecordSet& OutRecordSet, TArray<FString>&  OutErrorMessage, FOnIsCancelled InIsCancelled, bool& OutConnectionDropped)
	{
		const bool bStandardDebugOutput=true;
		const bool bAllowRetry=true;
		return RunCommand(InCommand, InParameters, OutRecordSet, OutErrorMessage, InIsCancelled, OutConnectionDropped, bStandardDebugOutput, bAllowRetry);
	}

	/**
	 * Runs internal perforce command, catches exceptions, returns results
	 */
	bool RunCommand(const FString& InCommand, const TArray<FString>& InParameters, FP4RecordSet& OutRecordSet, TArray<FString>& OutErrorMessage, FOnIsCancelled InIsCancelled, bool& OutConnectionDropped, const bool bInStandardDebugOutput, const bool bInAllowRetry);

	/**
	 * Creates a changelist with the specified description
	 */
	int32 CreatePendingChangelist(const FString &Description, FOnIsCancelled InIsCancelled, TArray<FString>& OutErrorMessages);

	/**
	 * Submits the specified changelist
	 */
	bool SubmitChangelist(int32 ChangeList, FOnIsCancelled InIsCancelled, TArray<FString>& OutErrorMessages);

	/**
	 * Make a valid connection if possible
	 * @param InServerName - Server name for the perforce connection
	 * @param InUserName - User for the perforce connection
	 * @param InWorkspaceName - Client Spec name for the perforce connection
	 * @param InTicket - The ticket to use in lieu of a login
	 * @param bInExceptOnWarnings - If true, perforce will except on warnings
	 */
	void EstablishConnection(const FString& InServerName, const FString& InUserName, const FString& InWorkspaceName, const FString& InTicket);

public:
	/** Perforce API client object */
	ClientApi P4Client;

	/** The current root for the client workspace */
	FString ClientRoot;

	/** true if connection was successfully established */
	bool bEstablishedConnection;

	/** Is this a connection to a unicode server? */ 
	bool bIsUnicode;
};

/**
 * Connection that is used within specific scope
 */
class FScopedPerforceConnection
{
public:
	/** 
	 * Constructor - establish a connection.
	 * The concurrency of the passed in command determines whether the persistent connection is 
	 * used or another connection is established (connections cannot safely be used across different
	 * threads).
	 */
	FScopedPerforceConnection( const class FPerforceSourceControlCommand& InCommand );

	/** 
	 * Constructor - establish a connection.
	 * The concurrency passed in determines whether the persistent connection is used or another 
	 * connection is established (connections cannot safely be used across different threads).
	 */
	FScopedPerforceConnection( EConcurrency::Type InConcurrency, const FString& InPort, const FString& InUserName, const FString& InClientSpec, const FString& InTicket );

	/**
	 * Destructor - disconnect if this is a temporary connection
	 */
	~FScopedPerforceConnection();

	/** Get the connection wrapped by this class */
	FPerforceConnection& GetConnection() const
	{
		return *Connection;
	}

	/** Check if this connection is valid */
	bool IsValid() const
	{
		return Connection != NULL;
	}

private:
	/** Set up the connection */
	void Initialize( const FString& InPort, const FString& InUserName, const FString& InClientSpec, const FString& InTicket );

private:
	/** The perforce connection we are using */
	FPerforceConnection* Connection;

	/** The concurrency of this connection */
	EConcurrency::Type Concurrency;
};