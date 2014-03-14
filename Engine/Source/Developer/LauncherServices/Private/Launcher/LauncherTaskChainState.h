// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherTaskChainState.h: Declares the FLauncherTaskChainState class.
=============================================================================*/

#pragma once


/**
 * Structure for passing launcher task chain state data.
 */
struct FLauncherTaskChainState
{
	/**
	 * Holds the path to the binaries directory (for deployment).
	 */
	FString BinariesDirectory;

	/**
	 * Holds the path to the content directory (for deployment).
	 */
	FString ContentDirectory;

	/**
	 * Holds the identifier of the deployed application.
	 */
	FString DeployedAppId;

	/**
	 * Holds the list of IP addresses and port numbers at which the file server is reachable.
	 */
	FString FileServerAddressListString;

	/**
	 * Holds a pointer to the launcher profile.
	 */
	ILauncherProfilePtr Profile;

	/**
	 * Holds the session identifier.
	 */
	FGuid SessionId;
};