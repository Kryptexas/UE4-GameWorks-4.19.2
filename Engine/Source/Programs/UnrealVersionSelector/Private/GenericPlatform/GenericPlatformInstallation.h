// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../UnrealVersionSelector.h"

struct FGenericPlatformInstallation
{
	/// Get the path to the installed build of the version selector
	static bool GetLauncherVersionSelector(FString &OutFileName);

	// Register an engine installation
	static bool RegisterEngineInstallation(const FString &Id, const FString &RootDirName);

	// Unregister an engine installation
	static bool UnregisterEngineInstallation(const FString &Id);

	// Test whether a given engine root directory is a source build
	static bool IsSourceDistribution(const FString &EngineRootDir);

	// Tests whether an engine directory contains a Perforce build
	static bool IsPerforceBuild(const FString &EngineRootDir);

	// Validate and normalize an engine root directory name
	static bool IsValidEngineRootDir(const FString &RootDir);

	// Validate and normalize an engine root directory name
	static bool NormalizeEngineRootDir(FString &RootDir);

	// Launches the editor application
	static bool LaunchEditor(const FString &RootDirName, const FString &Arguments);

	// Generate project files by running UBT
	static bool GenerateProjectFiles(const FString &RootDirName, const FString &Arguments);

	// Check whether the current settings are up to date
	static bool VerifyShellIntegration();

	// Update the installation settings
	static bool UpdateShellIntegration();

private:
	// Returns the config file for this 
	static FString GetConfigFilePath();

	// Returns the config file for this 
	static FConfigFile *GetConfigFile();
};
