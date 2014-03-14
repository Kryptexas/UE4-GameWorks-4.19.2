// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherBuildCommands.h: Declares the FLauncherBuildCommands.
=============================================================================*/

#pragma once


/**
 * Implements the launcher command arguments for building a game
 */
class FLauncherBuildGameCommand
	: public FLauncherUATCommand
{
public:
	FLauncherBuildGameCommand( const ITargetPlatform& InTargetPlatform )
		: TargetPlatform(InTargetPlatform)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherBuildGameTaskName", "Building game").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherBuildGameTaskDesc", "Build game for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		CommandLine += FString::Printf(TEXT(" -build -skipcook"));

		return CommandLine;
	}

private:

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;
};


/**
 * Implements the launcher command arguments for building a server
 */
class FLauncherBuildServerCommand
	: public FLauncherUATCommand
{
public:
	FLauncherBuildServerCommand( const ITargetPlatform& InTargetPlatform )
		: TargetPlatform(InTargetPlatform)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherBuildServerTaskName", "Building server").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherBuildServerTaskDesc", "Build game for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		FString Platform = TEXT("Win64");
		if (TargetPlatform.PlatformName() == TEXT("LinuxServer"))
		{
			Platform = TEXT("Linux");
		}
		CommandLine += FString::Printf(TEXT(" -build -server -noclient -serverplatform=%s -skipcook"),
			*Platform);

		return CommandLine;
	}

private:

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;
};
