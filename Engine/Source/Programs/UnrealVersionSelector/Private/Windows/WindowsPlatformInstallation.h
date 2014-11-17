// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../UnrealVersionSelector.h"
#include "../GenericPlatform/GenericPlatformInstallation.h"

struct FWindowsPlatformInstallation : FGenericPlatformInstallation
{
	// Register an engine installation
	static bool RegisterEngineInstallation(const FString &RootDirName, FString &OutIdentifier);

	// Launches the editor application
	static bool LaunchEditor(const FString &RootDirName, const FString &Arguments);

	// Generate project files by running UBT
	static bool GenerateProjectFiles(const FString &RootDirName, const FString &Arguments);

	// Select an engine installation
	static bool SelectEngineInstallation(FString &Identifier);
};

