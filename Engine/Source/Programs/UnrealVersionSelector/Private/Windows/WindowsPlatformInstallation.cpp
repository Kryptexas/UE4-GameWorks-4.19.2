// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../UnrealVersionSelector.h"
#include "WindowsPlatformInstallation.h"
#include "WindowsRegistry.h"
#include "Runtime/Core/Public/Misc/EngineVersion.h"
#include "AllowWindowsPlatformTypes.h"
#include <Shlwapi.h>
#include <Shellapi.h>

struct FEngineLabelSortPredicate
{
	bool operator()(const FString &A, const FString &B) const
	{
		// Return true if A < B
		int32 VerA = ParseReleaseVersion(A);
		int32 VerB = ParseReleaseVersion(B);
		if (VerA == VerB)
		{
			return StrCmpLogicalW(*A, *B) < 0;
		}
		else
		{
			return VerA > VerB;
		}
	}
};

FString GetInstallationDescription(const FString &Id, const FString &RootDir)
{
	// Official release versions just have a version number
	if (Id.Len() > 0 && Id[0] != '{')
	{
		return Id;
	}

	// Otherwise get the path
	FString PlatformRootDir = RootDir;
	FPaths::MakePlatformFilename(PlatformRootDir);

	// Perforce build
	if (FPlatformInstallation::IsSourceDistribution(RootDir))
	{
		if (FPlatformInstallation::IsPerforceBuild(RootDir))
		{
			return FString::Printf(TEXT("Perforce build at %s"), *PlatformRootDir);
		}
		else
		{
			return FString::Printf(TEXT("GitHub build at %s"), *PlatformRootDir);
		}
	}
	else
	{
		return FString::Printf(TEXT("%s"), *PlatformRootDir);
	}
}

void GetRequiredRegistrySettings(TIndirectArray<FRegistryRootedKey> &RootedKeys)
{
	// Get the path to the executable
	FString ExecutableFileName = FString(FPlatformProcess::BaseDir()) / FString(FPlatformProcess::ExecutableName(false));
	FPaths::MakePlatformFilename(ExecutableFileName);
	FString QuotedExecutableFileName = FString(TEXT("\"")) + ExecutableFileName + FString(TEXT("\""));

	// HKLM\SOFTWARE\Classes\.uproject
	FRegistryRootedKey *RootExtensionKey = new FRegistryRootedKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\.uproject"));
	RootExtensionKey->Key = new FRegistryKey();
	RootExtensionKey->Key->SetValue(TEXT(""), TEXT("Unreal.ProjectFile"));
	RootedKeys.AddRawItem(RootExtensionKey);

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile
	FRegistryRootedKey *RootFileTypeKey = new FRegistryRootedKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Unreal.ProjectFile"));
	RootFileTypeKey->Key = new FRegistryKey();
	RootFileTypeKey->Key->SetValue(TEXT(""), TEXT("Unreal Engine Project File"));
	RootFileTypeKey->Key->FindOrAddKey(L"DefaultIcon")->SetValue(TEXT(""), QuotedExecutableFileName);
	RootedKeys.AddRawItem(RootFileTypeKey);

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\shell
	FRegistryKey *ShellKey = RootFileTypeKey->Key->FindOrAddKey(TEXT("shell"));

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\shell\open
	FRegistryKey *ShellOpenKey = ShellKey->FindOrAddKey(TEXT("open"));
	ShellOpenKey->SetValue(L"", L"Open");
	ShellOpenKey->FindOrAddKey(L"command")->SetValue(L"", QuotedExecutableFileName + FString(TEXT(" /editor \"%1\"")));

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\shell\run
	FRegistryKey *ShellRunKey = ShellKey->FindOrAddKey(TEXT("run"));
	ShellRunKey->SetValue(TEXT(""), TEXT("Launch game"));
	ShellRunKey->SetValue(TEXT("Icon"), QuotedExecutableFileName);
	ShellRunKey->FindOrAddKey(L"command")->SetValue(TEXT(""), QuotedExecutableFileName + FString(TEXT(" /game \"%1\"")));

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\shell\rungenproj
	FRegistryKey *ShellProjectKey = ShellKey->FindOrAddKey(TEXT("rungenproj"));
	ShellProjectKey->SetValue(L"", L"Generate Visual Studio project files");
	ShellProjectKey->SetValue(L"Icon", QuotedExecutableFileName);
	ShellProjectKey->FindOrAddKey(L"command")->SetValue(TEXT(""), QuotedExecutableFileName + FString(TEXT(" /projectfiles \"%1\"")));

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\shell\switchversion
	FRegistryKey *ShellVersionKey = ShellKey->FindOrAddKey(TEXT("switchversion"));
	ShellVersionKey->SetValue(TEXT(""), TEXT("Switch engine version"));
	ShellVersionKey->SetValue(TEXT("ExtendedSubCommandsKey"), TEXT("Unreal.ProjectFile\\switchversion"));
	ShellVersionKey->SetValue(TEXT("Icon"), QuotedExecutableFileName);
	ShellVersionKey->SetValue(TEXT("MUIVerb"), TEXT("Switch Unreal Engine version"));

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\switchversion
	FRegistryKey *SwitchKey = RootFileTypeKey->Key->FindOrAddKey(TEXT("switchversion"));

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\switchversion\shell
	FRegistryKey *SwitchShellKey = SwitchKey->FindOrAddKey(TEXT("shell"));

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\switchversion\shell\<Label>
	TMap<FString, FString> Installations;
	FDesktopPlatformModule::Get()->EnumerateEngineInstallations(Installations);

	if (Installations.Num() > 0)
	{
		// Add the default option
		FRegistryKey *DefaultKey = SwitchShellKey->FindOrAddKey(L"000");
		DefaultKey->SetValue(L"MUIVerb", L"Default");
		DefaultKey->FindOrAddKey(L"command")->SetValue(L"", QuotedExecutableFileName + FString(TEXT(" /associate \"%1\" \"\"")));

		// Build a list of installation names
		TMap<FString, FString> InstallationNames;
		for (TMap<FString, FString>::TConstIterator Iter(Installations); Iter; ++Iter)
		{
			const FString &Id = Iter->Key;
			FString Description = GetInstallationDescription(Id, Iter.Value());
			InstallationNames.Add(Description, Id);
		}
		InstallationNames.KeySort(FEngineLabelSortPredicate());

		// Add the installation options
		INT Idx = 1;
		for (TMap<FString, FString>::TConstIterator Iter(InstallationNames); Iter; ++Iter)
		{
			wchar_t KeyName[100];
			wsprintf(KeyName, L"%03d", Idx);

			FRegistryKey *VersionKey = SwitchShellKey->FindOrAddKey(KeyName);
			VersionKey->SetValue(TEXT("MUIVerb"), Iter.Key());
			VersionKey->FindOrAddKey(TEXT("command"))->SetValue(TEXT(""), QuotedExecutableFileName + FString::Printf(TEXT(" /associate \"%%1\" \"%s\""), *Iter.Value()));
			if(Idx++ == 1) VersionKey->SetValue(L"CommandFlags", 0x20);
		}
	}

	// HKLM\SOFTWARE\Classes\Unreal.ProjectFile\switchversion\shell\browse
	FRegistryKey *BrowseShellKey = SwitchShellKey->FindOrAddKey(TEXT("browse"));
	BrowseShellKey->SetValue(TEXT("MUIVerb"), TEXT("Browse..."));
	BrowseShellKey->FindOrAddKey(TEXT("command"))->SetValue(L"", QuotedExecutableFileName + FString(TEXT(" /browse \"%1\"")));
	if (Installations.Num() > 0)
	{
		BrowseShellKey->SetValue(TEXT("CommandFlags"), 0x20);
	}
}

bool FWindowsPlatformInstallation::GetLauncherVersionSelector(FString &OutFileName)
{
	// Get the launcher installed path
	TCHAR InstallDir[MAX_PATH];
	DWORD InstallDirLen = sizeof(InstallDir);
	if (RegGetValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\EpicGames\\Unreal Engine"), TEXT("INSTALLDIR"), RRF_RT_REG_SZ, NULL, InstallDir, &InstallDirLen) != ERROR_SUCCESS)
	{
		return false;
	}

	// Normalize the directory
	FString NormalizedInstallDir = InstallDir;
	FPaths::NormalizeDirectoryName(NormalizedInstallDir);

	// Check the version selector executable exists
	FString LauncherFileName = NormalizedInstallDir / TEXT("Launcher/Engine/Binaries/Win64/UnrealVersionSelector.exe");
	if (IFileManager::Get().FileSize(*LauncherFileName) < 0)
	{
		return false;
	}

	OutFileName = LauncherFileName;
	return true;
}

bool FWindowsPlatformInstallation::RegisterEngineInstallation(const FString &Id, const FString &RootDirName)
{
	FString NormalizedRootDirName = RootDirName;
	FPaths::NormalizeDirectoryName(NormalizedRootDirName);

	FRegistryRootedKey RootKey(HKEY_LOCAL_MACHINE, FString::Printf(TEXT("SOFTWARE\\EpicGames\\Unreal Engine\\%s"), *Id));
	RootKey.Key = new FRegistryKey();
	RootKey.Key->SetValue(TEXT("InstalledDirectory"), NormalizedRootDirName);
	return RootKey.Write(false);
}

bool FWindowsPlatformInstallation::UnregisterEngineInstallation(const FString &Id)
{
	FRegistryRootedKey RootKey(HKEY_LOCAL_MACHINE, FString::Printf(TEXT("SOFTWARE\\EpicGames\\Unreal Engine\\%s"), *Id));
	return RootKey.Write(true);
}

bool FWindowsPlatformInstallation::IsSourceDistribution(const FString &EngineRootDir)
{
	// Check for the existence of a GenerateProjectFiles.bat file. This allows compatibility with the GitHub 4.0 release.
	FString GenerateProjectFilesPath = EngineRootDir / TEXT("GenerateProjectFiles.bat");
	if (IFileManager::Get().FileSize(*GenerateProjectFilesPath) >= 0)
	{
		return true;
	}

	// Otherwise use the default test
	return FGenericPlatformInstallation::IsSourceDistribution(EngineRootDir);
}

bool FWindowsPlatformInstallation::LaunchEditor(const FString &RootDirName, const FString &Arguments)
{
	FString EditorFileName = RootDirName / TEXT("Engine/Binaries/Win64/UE4Editor.exe");
	return FPlatformProcess::ExecProcess(*EditorFileName, *Arguments, NULL, NULL, NULL);
}

bool FWindowsPlatformInstallation::GenerateProjectFiles(const FString &RootDirName, const FString &Arguments)
{
	FString AllArguments = FString::Printf(TEXT("/c \"\"%s\" %s\""), *(RootDirName / TEXT("Engine/Build/BatchFiles/GenerateProjectFiles.bat")), *Arguments);
	return FPlatformProcess::ExecProcess(TEXT("cmd.exe"), *AllArguments, NULL, NULL, NULL);
}

bool FWindowsPlatformInstallation::VerifyShellIntegration()
{
	TIndirectArray<FRegistryRootedKey> Keys;
	GetRequiredRegistrySettings(Keys);

	for (int32 Idx = 0; Idx < Keys.Num(); Idx++)
	{
		if (!Keys[Idx].IsUpToDate(true))
		{
			return false;
		}
	}

	return true;
}

bool FWindowsPlatformInstallation::UpdateShellIntegration()
{
	TIndirectArray<FRegistryRootedKey> Keys;
	GetRequiredRegistrySettings(Keys);

	for (int32 Idx = 0; Idx < Keys.Num(); Idx++)
	{
		if (!Keys[Idx].Write(true))
		{
			return false;
		}
	}

	return true;
}

#include "HideWindowsPlatformTypes.h"
