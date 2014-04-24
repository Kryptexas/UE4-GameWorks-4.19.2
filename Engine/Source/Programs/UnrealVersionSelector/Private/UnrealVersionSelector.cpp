#include "UnrealVersionSelector.h"
#include "RequiredProgramMainCPPInclude.h"
#include "DesktopPlatformModule.h"
#include "PlatformInstallation.h"
#include "FileAssociation.h"

IMPLEMENT_APPLICATION(UnrealVersionSelector, "UnrealVersionSelector")

FString FullExecutableName()
{
	return FString(FPlatformProcess::BaseDir()) / FString(FPlatformProcess::ExecutableName(false));
}

bool RegisterCurrentEngineDirectory()
{
	// Get the current engine directory.
	FString EngineRootDir = FPlatformMisc::RootDir();

	// Get the canonical version selector path
	FString VersionSelectorFileName;
	if (!FPlatformInstallation::GetLauncherVersionSelector(VersionSelectorFileName))
	{
		VersionSelectorFileName = FString(FPlatformProcess::BaseDir()) / FString(FPlatformProcess::ExecutableName(false));
	}

	// Launch the installed version selector as elevated to register this directory
	FString Arguments = FString::Printf(TEXT("/register \"%s\""), *EngineRootDir);
	return FPlatformProcess::ExecElevatedProcess(*VersionSelectorFileName, *Arguments, NULL);
}

void EnumerateInvalidInstallations(TArray<FString> &OutIds)
{
	// Enumerate all the engine installations
	TMap<FString, FString> Installations;
	FPlatformInstallation::EnumerateEngineInstallations(Installations);

	// Check each one of them has a valid engine path
	for (TMap<FString, FString>::TConstIterator Iter(Installations); Iter; ++Iter)
	{
		FString RootDir = Iter.Value();
		if (!FPlatformInstallation::IsValidEngineRootDir(RootDir))
		{
			OutIds.Add(Iter.Key());
		}
	}
}

bool VerifySettings()
{
	// Enumerate all the invalid installations
	TArray<FString> InvalidInstallationIds;
	EnumerateInvalidInstallations(InvalidInstallationIds);

	// If the current settings are valid, we don't need to do anything
	if (InvalidInstallationIds.Num() == 0 && FPlatformInstallation::VerifyShellIntegration())
	{
		return true;
	}

	// Otherwise relaunch as administrator
	return FPlatformProcess::ExecElevatedProcess(*FullExecutableName(), TEXT("/update"), NULL);
}

bool UpdateSettings()
{
	// Enumerate all the invalid installations
	TArray<FString> InvalidInstallationIds;
	EnumerateInvalidInstallations(InvalidInstallationIds);

	// Remove those installations
	for (int32 Idx = 0; Idx < InvalidInstallationIds.Num(); Idx++)
	{
		if (!FPlatformInstallation::UnregisterEngineInstallation(InvalidInstallationIds[Idx]))
		{
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Couldn't update installation settings."), TEXT("Error"));
			return false;
		}
	}

	// Update everything
	if (!FPlatformInstallation::UpdateShellIntegration())
	{
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Couldn't update installation settings."), TEXT("Error"));
		return false;
	}
	return true;
}

bool RegisterEngineDirectory(const FString &EngineId, const FString &EngineDir)
{
	// Add the installation by name
	if (!FPlatformInstallation::RegisterEngineInstallation(EngineId, EngineDir))
	{
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Couldn't add engine installation."), TEXT("Error"));
		return false;
	}

	// Add an installation, then call update
	return UpdateSettings();
}

bool RegisterEngineDirectory(const FString &EngineDir)
{
	// Get any existing tag name or register a new one
	FString EngineId;
	if (!GetEngineIdFromRootDir(EngineDir, EngineId))
	{
		EngineId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensInBraces);
	}

	// Add an installation, then call update
	return RegisterEngineDirectory(EngineId, EngineDir);
}

bool SelectEngineAssociation(const FString &ProjectFileName, const FString &EngineId)
{
	// Just write the ID into the file
	if (!SetEngineIdForProject(ProjectFileName, EngineId))
	{
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Couldn't set association for project. Check the file is writeable."), TEXT("Error"));
		return false;
	}
	return true;
}

bool BrowseForEngineAssociation(const FString &ProjectFileName)
{
	// Get the currently bound engine directory for the project
	FString EngineRootDir;
	GetEngineRootDirForProject(ProjectFileName, EngineRootDir);

	// Browse for a new directory
	FString NewEngineRootDir;
	if (!FDesktopPlatformModule::Get()->OpenDirectoryDialog(NULL, TEXT("Select the Unreal Engine installation to use for this project"), EngineRootDir, NewEngineRootDir))
	{
		return false;
	}

	// Check it's a valid directory
	if (!FPlatformInstallation::NormalizeEngineRootDir(NewEngineRootDir))
	{
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("The selected directory is not a valid engine installation."), TEXT("Error"));
		return false;
	}

	// Check that it's a registered engine directory
	FString EngineId;
	if (!GetEngineIdFromRootDir(NewEngineRootDir, EngineId))
	{
		int32 ExitCode;
		if (!FPlatformProcess::ExecElevatedProcess(*FullExecutableName(), *FString::Printf(TEXT("/register \"%s\""), *NewEngineRootDir), &ExitCode) || ExitCode != 0)
		{
			return false;
		}
		if (!GetEngineIdFromRootDir(NewEngineRootDir, EngineId))
		{
			return false;
		}
	}

	// Set the file association
	return SelectEngineAssociation(ProjectFileName, EngineId);
}

bool GetValidatedEngineRootDir(const FString &ProjectFileName, FString &OutRootDir)
{
	// Get the engine directory for this project
	if (!GetEngineRootDirForProject(ProjectFileName, OutRootDir))
	{
		// Try to set an association
		if (!BrowseForEngineAssociation(ProjectFileName))
		{
			return false;
		}

		// See if it's valid now
		if (!GetEngineRootDirForProject(ProjectFileName, OutRootDir))
		{
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Error retrieving project root directory"), TEXT("Error"));
			return false;
		}
	}
	return true;
}

bool LaunchEditor(const FString &ProjectFileName, const FString &Arguments)
{
	// Get the engine root directory
	FString RootDir;
	if (!GetValidatedEngineRootDir(ProjectFileName, RootDir))
	{
		return false;
	}

	// Launch the editor
	if (!FPlatformInstallation::LaunchEditor(RootDir, FString::Printf(TEXT("\"%s\" %s"), *ProjectFileName, *Arguments)))
	{
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Failed to launch editor"), TEXT("Error"));
		return false;
	}

	return true;
}

bool GenerateProjectFiles(const FString &ProjectFileName)
{
	// Get the engine root directory
	FString RootDir;
	if (!GetValidatedEngineRootDir(ProjectFileName, RootDir))
	{
		return false;
	}

	// Build the argument list
	FString Arguments = FString::Printf(TEXT("\"%s\" -game"), *ProjectFileName);
	if (FPlatformInstallation::IsSourceDistribution(RootDir))
	{
		Arguments += TEXT(" -engine");
	}

	// Launch the editor
	if (!FPlatformInstallation::LaunchEditor(RootDir, FString::Printf(TEXT("\"%s\" %s"), *ProjectFileName, *Arguments)))
	{
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Failed to launch editor"), TEXT("Error"));
		return false;
	}

	return true;
}

int Main(const TArray<FString> &Arguments)
{
	bool bRes = false;
	if (Arguments.Num() == 0)
	{
		// Add the current directory to the list of installations
		bRes = RegisterCurrentEngineDirectory();
	}
	else if (Arguments.Num() == 1 && Arguments[0] == TEXT("/verify"))
	{
		// Verify all the settings are correct.
		bRes = VerifySettings();
	}
	else if (Arguments.Num() == 1 && Arguments[0] == TEXT("/update"))
	{
		// Update all the settings.
		bRes = UpdateSettings();
	}
	else if (Arguments.Num() == 2 && Arguments[0] == TEXT("/register"))
	{
		// Register the given directory as an engine directory
		bRes = RegisterEngineDirectory(Arguments[1]);
	}
	else if (Arguments.Num() == 3 && Arguments[0] == TEXT("/register"))
	{
		// Register an id corresponding to a given directory
		bRes = RegisterEngineDirectory(Arguments[1], Arguments[2]);
	}
	else if (Arguments.Num() == 3 && Arguments[0] == TEXT("/associate"))
	{
		// Associate with an engine label
		bRes = SelectEngineAssociation(Arguments[1], Arguments[2]);
	}
	else if (Arguments.Num() == 2 && Arguments[0] == TEXT("/browse"))
	{
		// Associate the project with a user selected engine
		bRes = BrowseForEngineAssociation(Arguments[1]);
	}
	else if (Arguments.Num() == 2 && Arguments[0] == TEXT("/editor"))
	{
		// Open a project with the editor
		bRes = LaunchEditor(Arguments[1], L"");
	}
	else if (Arguments.Num() == 2 && Arguments[0] == TEXT("/game"))
	{
		// Play a game using the editor executable
		bRes = LaunchEditor(Arguments[1], L"/game");
	}
	else if (Arguments.Num() == 2 && Arguments[0] == TEXT("/generateprojectfiles"))
	{
		// Generate Visual Studio project files
		bRes = GenerateProjectFiles(Arguments[1]);
	}
	else
	{
		// Invalid command line
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Invalid command line"), NULL);
	}
	return bRes ? 0 : 1;
}

#if PLATFORM_WINDOWS

	#include "AllowWindowsPlatformTypes.h"
	#include <Shellapi.h>

	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int ShowCmd)
	{
		int ArgC;
		LPWSTR *ArgV = ::CommandLineToArgvW(GetCommandLine(), &ArgC);

		FCommandLine::Set(TEXT(""));

		TArray<FString> Arguments;
		for (int Idx = 1; Idx < ArgC; Idx++)
		{
			Arguments.Add(ArgV[Idx]);
		}

		return Main(Arguments);
	}

	#include "HideWindowsPlatformTypes.h"

#else

	int main(int ArgC, const char *ArgV[])
	{
		FCommandLine::Set(TEXT(""));
		
		TArray<FString> Arguments;
		for (int Idx = 1; Idx < ArgC; Idx++)
		{
			Arguments.Add(ArgV[Idx]);
		}
		
		return Main(Arguments);
	}

#endif