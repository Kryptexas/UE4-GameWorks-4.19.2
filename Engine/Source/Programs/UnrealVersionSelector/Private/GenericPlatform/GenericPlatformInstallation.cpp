// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../UnrealVersionSelector.h"
#include "GenericPlatformInstallation.h"
#include "../PlatformInstallation.h"
#include "Runtime/Launch/Resources/Version.h"

bool FGenericPlatformInstallation::GetLauncherVersionSelector(FString &OutFileName)
{
	FConfigFile ConfigFile;
	ConfigFile.Read(GetConfigFilePath());

	FString InstallationPath;
	if (!ConfigFile.GetString(TEXT("Launcher"), TEXT("InstalledPath"), InstallationPath))
	{
		return false;
	}

	FString LauncherFileName = InstallationPath / TEXT("Engine/Binaries/Win64/UnrealVersionSelector.exe");
	if (IFileManager::Get().FileSize(*LauncherFileName) < 0)
	{
		return false;
	}

	return true;
}

bool FGenericPlatformInstallation::RegisterEngineInstallation(const FString &Id, const FString &RootDirName)
{
	FString NormalizedRootDirName = RootDirName;
	FPaths::NormalizeDirectoryName(NormalizedRootDirName);

	FConfigFile ConfigFile;
	ConfigFile.Read(GetConfigFilePath());

	FConfigSection &Section = ConfigFile.FindOrAdd(TEXT("Installations"));
	Section.AddUnique(*Id, NormalizedRootDirName);
	ConfigFile.Dirty = true;
	return ConfigFile.Write(GetConfigFilePath());
}

bool FGenericPlatformInstallation::UnregisterEngineInstallation(const FString &Id)
{
	FConfigFile ConfigFile;
	ConfigFile.Read(GetConfigFilePath());

	FConfigSection &Section = ConfigFile.FindOrAdd(TEXT("Installations"));
	Section.Remove(*Id);
	ConfigFile.Dirty = true;
	return ConfigFile.Write(GetConfigFilePath());
}

bool FGenericPlatformInstallation::IsSourceDistribution(const FString &EngineRootDir)
{
	// Check for the existence of a SourceBuild.txt file
	FString SourceBuildPath = EngineRootDir / TEXT("Engine\\Build\\SourceDistribution.txt");
	return (IFileManager::Get().FileSize(*SourceBuildPath) >= 0);
}

bool FGenericPlatformInstallation::IsPerforceBuild(const FString &EngineRootDir)
{
	// Check for the existence of a SourceBuild.txt file
	FString PerforceBuildPath = EngineRootDir / TEXT("Engine\\Build\\PerforceBuild.txt");
	return (IFileManager::Get().FileSize(*PerforceBuildPath) >= 0);
}

bool FGenericPlatformInstallation::IsValidEngineRootDir(const FString &RootDir)
{
	// Check if the engine binaries directory exists
	FString EngineBinariesDirName = RootDir / TEXT("Engine/Binaries");
	return IFileManager::Get().DirectoryExists(*EngineBinariesDirName);
}

bool FGenericPlatformInstallation::NormalizeEngineRootDir(FString &RootDir)
{
	// Canonicalize the engine path and remove the last backslash.
	FString NormalizedRootDir = RootDir;
	FPaths::NormalizeDirectoryName(NormalizedRootDir);

	// Check if it's valid
	if (IsValidEngineRootDir(NormalizedRootDir))
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
	if (IsValidEngineRootDir(NormalizedRootDir))
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

bool FGenericPlatformInstallation::VerifyShellIntegration()
{
	// No shell integration by default
	return true;
}

bool FGenericPlatformInstallation::UpdateShellIntegration()
{
	// No shell integration by default
	return true;
}

FString FGenericPlatformInstallation::GetConfigFilePath()
{
	return FString(FPlatformProcess::ApplicationSettingsDir()) / FString(EPIC_PRODUCT_IDENTIFIER) / FString(TEXT("Install.ini"));
}

