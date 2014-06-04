// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "CoreUObject.h"

/**
 * P4 environment class to connect with P4 executable.
 */
class FP4Env
{
public:
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnP4MadeProgress, const FString&)

	/**
	 * Initializes P4 environment from command line.
	 *
	 * @param CommandLine Command line to initialize from.
	 *
	 * @returns True if succeeded. False otherwise.
	 */
	static bool Init(const TCHAR* CommandLine);

	/**
	 * Initializes P4 environment.
	 *
	 * @param Port P4 port to connect to.
	 * @param User P4 user to connect.
	 * @param Client P4 workspace name.
	 *
	 * @returns True if succeeded. False otherwise.
	 */
	static bool Init(const FString& Port, const FString& User, const FString& Client);



	/**
	 * Gets instance of the P4 environment.
	 *
	 * @returns P4 environment object.
	 */
	static FP4Env& Get();

	/**
	 * Runs P4 process and allows communication through progress delegate.
	 *
	 * @param CommandLine Command line to run P4 process with.
	 * @param OnP4MadeProgress Progress delegate function.
	 *
	 * @returns True if succeeded. False otherwise.
	 */
	static bool RunP4Progress(const FString& CommandLine, const FOnP4MadeProgress& OnP4MadeProgress);

	/**
	 * Runs P4 process and catches output made by this process.
	 *
	 * @param CommandLine Command line to run P4 process with.
	 * @param Output Catched output.
	 *
	 * @returns True if succeeded. False otherwise.
	 */
	static bool RunP4Output(const FString& CommandLine, FString& Output);

	/**
	 * Runs P4 process.
	 *
	 * @param CommandLine Command line to run P4 process with.
	 *
	 * @returns True if succeeded. False otherwise.
	 */
	static bool RunP4(const FString& CommandLine);



	/**
	 * Gets P4 user.
	 *
	 * @returns P4 user.
	 */
	const FString& GetUser() const;

	/**
	 * Gets P4 port.
	 *
	 * @returns P4 port.
	 */
	const FString& GetPort() const;

	/**
	 * Gets P4 workspace name.
	 *
	 * @returns P4 workspace name.
	 */
	const FString& GetClient() const;

	/**
	 * Gets current P4 branch.
	 *
	 * @returns Current P4 branch.
	 */
	const FString& GetBranch() const;

private:
	/**
	 * Constructor
	 *
	 * @param Port P4 port.
	 * @param User P4 user.
	 * @param Client P4 workspace name.
	 * @param Branch Current P4 branch.
	 */
	FP4Env(const FString& Port, const FString& User, const FString& Client, const FString& Branch);

	/**
	 * Parses param either from command line or environment variables.
	 * Command line has priority.
	 *
	 * @param CommandLine Command line to parse.
	 * @param ParamName Param name to parse.
	 *
	 * @returns Parsed value.
	 */
	static FString GetParam(const TCHAR* CommandLine, const FString& ParamName);

	/* Singleton instance pointer. */
	static TSharedPtr<FP4Env> Env;

	/* P4 port. */
	FString Port;

	/* P4 user. */
	FString User;

	/* P4 workspace name. */
	FString Client;

	/* P4 current branch. */
	FString Branch;
};
