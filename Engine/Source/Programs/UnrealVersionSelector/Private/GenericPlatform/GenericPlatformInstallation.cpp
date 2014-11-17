// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../UnrealVersionSelector.h"
#include "GenericPlatformInstallation.h"
#include "../PlatformInstallation.h"
#include "Runtime/Launch/Resources/Version.h"

bool FGenericPlatformInstallation::RegisterEngineInstallation(const FString &Id, const FString &RootDirName)
{
	// No default implementation
	return true;
}

bool FGenericPlatformInstallation::NormalizeEngineRootDir(FString &RootDir)
{
	// Canonicalize the engine path and remove the last backslash.
	FString NormalizedRootDir = RootDir;
	FPaths::NormalizeDirectoryName(NormalizedRootDir);

	// Check if it's valid
	if (FDesktopPlatformModule::Get()->IsValidRootDirectory(NormalizedRootDir))
	{
		RootDir = NormalizedRootDir;
		return true;
	}

	// Otherwise try to accept directories underneath the root
	if (!NormalizedRootDir.RemoveFromEnd(TEXT("/Engine")))
	{
		if (!NormalizedRootDir.RemoveFromEnd(TEXT("/Engine/Binaries")))
		{
			NormalizedRootDir.RemoveFromEnd(FString(TEXT("/Engine/Binaries/")) / FPlatformProcess::GetBinariesSubdirectory());
		}
	}

	// Check if the engine binaries directory exists
	if (FDesktopPlatformModule::Get()->IsValidRootDirectory(NormalizedRootDir))
	{
		RootDir = NormalizedRootDir;
		return true;
	}

	return false;
}

bool FGenericPlatformInstallation::LaunchEditor(const FString &RootDirName, const FString &Arguments)
{
	// No default implementation
	return false;
}

bool FGenericPlatformInstallation::GenerateProjectFiles(const FString &RootDirName, const FString &Arguments)
{
	// No default implementation
	return false;
}

bool FGenericPlatformInstallation::SelectEngineInstallation(FString &Identifier)
{
	// No default implementation
	return false;
}
