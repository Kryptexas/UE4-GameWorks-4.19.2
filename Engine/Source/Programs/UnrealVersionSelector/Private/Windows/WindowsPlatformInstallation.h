// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../UnrealVersionSelector.h"
#include "../GenericPlatform/GenericPlatformInstallation.h"

struct FWindowsPlatformInstallation : FGenericPlatformInstallation
{
	/// Get the path to the installed build of the version selector
	static bool GetLauncherVersionSelector(FString &OutFileName);

	// Engine installations
	static bool RegisterEngineInstallation(const FString &Id, const FString &RootDirName);
	static bool UnregisterEngineInstallation(const FString &Id);
	static void EnumerateEngineInstallations(TMap<FString, FString> &OutInstallations);

	// Test whether a given engine root directory is a source build
	static bool IsSourceDistribution(const FString &EngineRootDir);

	// Launches the editor application
	static bool LaunchEditor(const FString &RootDirName, const FString &Arguments);

	// Generate project files by running UBT
	static bool GenerateProjectFiles(const FString &RootDirName, const FString &Arguments);

	// Check whether the current settings are up to date
	static bool VerifyShellIntegration();

	// Update the installation settings
	static bool UpdateShellIntegration();
};

