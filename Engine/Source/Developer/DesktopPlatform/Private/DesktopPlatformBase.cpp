// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DesktopPlatformPrivatePCH.h"
#include "DesktopPlatformBase.h"
#include "UProjectInfo.h"
#include "EngineVersion.h"
#include "ModuleManager.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "DesktopPlatform"

FDesktopPlatformBase::FDesktopPlatformBase()
{
	LauncherInstallationTimestamp = FDateTime::MinValue();
}

FString FDesktopPlatformBase::GetEngineDescription(const FString& Identifier)
{
	// Official release versions just have a version number
	if(IsStockEngineRelease(Identifier))
	{
		return Identifier;
	}

	// Otherwise get the path
	FString RootDir;
	if(!GetEngineRootDirFromIdentifier(Identifier, RootDir))
	{
		return FString();
	}

	// Convert it to a platform directory
	FString PlatformRootDir = RootDir;
	FPaths::MakePlatformFilename(PlatformRootDir);

	// Perforce build
	if (IsSourceDistribution(RootDir))
	{
		return FString::Printf(TEXT("Source build at %s"), *PlatformRootDir);
	}
	else
	{
		return FString::Printf(TEXT("Binary build at %s"), *PlatformRootDir);
	}
}

FString FDesktopPlatformBase::GetCurrentEngineIdentifier()
{
	if(CurrentEngineIdentifier.Len() == 0 && !GetEngineIdentifierFromRootDir(FPlatformMisc::RootDir(), CurrentEngineIdentifier))
	{
		CurrentEngineIdentifier.Empty();
	}
	return CurrentEngineIdentifier;
}

void FDesktopPlatformBase::EnumerateLauncherEngineInstallations(TMap<FString, FString> &OutInstallations)
{
	// Cache the launcher install list if necessary
	ReadLauncherInstallationList();

	// We've got a list of launcher installations. Filter it by the engine installations.
	for(TMap<FString, FString>::TConstIterator Iter(LauncherInstallationList); Iter; ++Iter)
	{
		FString AppName = Iter.Key();
		if(AppName.RemoveFromStart(TEXT("UE_"), ESearchCase::CaseSensitive))
		{
			OutInstallations.Add(AppName, Iter.Value());
		}
	}
}

void FDesktopPlatformBase::EnumerateLauncherSampleInstallations(TArray<FString> &OutInstallations)
{
	// Cache the launcher install list if necessary
	ReadLauncherInstallationList();

	// We've got a list of launcher installations. Filter it by the engine installations.
	for(TMap<FString, FString>::TConstIterator Iter(LauncherInstallationList); Iter; ++Iter)
	{
		FString AppName = Iter.Key();
		if(!AppName.StartsWith(TEXT("UE_"), ESearchCase::CaseSensitive))
		{
			OutInstallations.Add(Iter.Value());
		}
	}
}

void FDesktopPlatformBase::EnumerateLauncherSampleProjects(TArray<FString> &OutFileNames)
{
	// Enumerate all the sample installation directories
	TArray<FString> LauncherSampleDirectories;
	EnumerateLauncherSampleInstallations(LauncherSampleDirectories);

	// Find all the project files within them
	for(int32 Idx = 0; Idx < LauncherSampleDirectories.Num(); Idx++)
	{
		TArray<FString> FileNames;
		IFileManager::Get().FindFiles(FileNames, *(LauncherSampleDirectories[Idx] / TEXT("*.uproject")), true, false);
		OutFileNames.Append(FileNames);
	}
}

bool FDesktopPlatformBase::GetEngineRootDirFromIdentifier(const FString &Identifier, FString &OutRootDir)
{
	// Get all the installations
	TMap<FString, FString> Installations;
	EnumerateEngineInstallations(Installations);

	// Find the one with the right identifier
	for (TMap<FString, FString>::TConstIterator Iter(Installations); Iter; ++Iter)
	{
		if (Iter->Key == Identifier)
		{
			OutRootDir = Iter->Value;
			return true;
		}
	}
	return false;
}

bool FDesktopPlatformBase::GetEngineIdentifierFromRootDir(const FString &RootDir, FString &OutIdentifier)
{
	// Get all the installations
	TMap<FString, FString> Installations;
	EnumerateEngineInstallations(Installations);

	// Normalize the root directory
	FString NormalizedRootDir = RootDir;
	FPaths::NormalizeDirectoryName(NormalizedRootDir);

	// Find the label for the given directory
	for (TMap<FString, FString>::TConstIterator Iter(Installations); Iter; ++Iter)
	{
		if (Iter->Value == NormalizedRootDir)
		{
			OutIdentifier = Iter->Key;
			return true;
		}
	}

	// Otherwise just try to add it
	return RegisterEngineInstallation(RootDir, OutIdentifier);
}

bool FDesktopPlatformBase::GetDefaultEngineIdentifier(FString &OutId)
{
	TMap<FString, FString> Installations;
	EnumerateEngineInstallations(Installations);

	bool bRes = false;
	if (Installations.Num() > 0)
	{
		// Default to the first install
		TMap<FString, FString>::TConstIterator Iter(Installations);
		OutId = Iter.Key();
		++Iter;

		// Try to find the most preferred install
		for(; Iter; ++Iter)
		{
			if(IsPreferredEngineIdentifier(Iter.Key(), OutId))
			{
				OutId = Iter.Key();
			}
		}
	}
	return bRes;
}

bool FDesktopPlatformBase::GetDefaultEngineRootDir(FString &OutDirName)
{
	FString Identifier;
	return GetDefaultEngineIdentifier(Identifier) && GetEngineRootDirFromIdentifier(Identifier, OutDirName);
}

bool FDesktopPlatformBase::IsPreferredEngineIdentifier(const FString &Identifier, const FString &OtherIdentifier)
{
	int32 Version = ParseReleaseVersion(Identifier);
	int32 OtherVersion = ParseReleaseVersion(OtherIdentifier);

	if(Version != OtherVersion)
	{
		return Version > OtherVersion;
	}
	else
	{
		return Identifier > OtherIdentifier;
	}
}

bool FDesktopPlatformBase::IsStockEngineRelease(const FString &Identifier)
{
	return Identifier.Len() > 0 && FChar::IsDigit(Identifier[0]);
}

bool FDesktopPlatformBase::IsSourceDistribution(const FString &EngineRootDir)
{
	// Check for the existence of a SourceBuild.txt file
	FString SourceBuildPath = EngineRootDir / TEXT("Engine/Build/SourceDistribution.txt");
	return (IFileManager::Get().FileSize(*SourceBuildPath) >= 0);
}

bool FDesktopPlatformBase::IsPerforceBuild(const FString &EngineRootDir)
{
	// Check for the existence of a SourceBuild.txt file
	FString PerforceBuildPath = EngineRootDir / TEXT("Engine/Build/PerforceBuild.txt");
	return (IFileManager::Get().FileSize(*PerforceBuildPath) >= 0);
}

bool FDesktopPlatformBase::IsValidRootDirectory(const FString &RootDir)
{
	FString EngineBinariesDirName = RootDir / TEXT("Engine/Binaries");
	FPaths::NormalizeDirectoryName(EngineBinariesDirName);
	return IFileManager::Get().DirectoryExists(*EngineBinariesDirName);
}

bool FDesktopPlatformBase::SetEngineIdentifierForProject(const FString &ProjectFileName, const FString &InIdentifier)
{
	// Load the project file
	TSharedPtr<FJsonObject> ProjectFile = LoadProjectFile(ProjectFileName);
	if (!ProjectFile.IsValid())
	{
		return false;
	}

	// Check if the project is a non-foreign project of the given engine installation. If so, blank the identifier 
	// string to allow portability between source control databases. GetEngineIdentifierForProject will translate
	// the association back into a local identifier on other machines or syncs.
	FString Identifier = InIdentifier;
	if(Identifier.Len() > 0)
	{
		FString RootDir;
		if(GetEngineRootDirFromIdentifier(Identifier, RootDir))
		{
			const FUProjectDictionary &Dictionary = GetCachedProjectDictionary(RootDir);
			if(!Dictionary.IsForeignProject(ProjectFileName))
			{
				Identifier.Empty();
			}
		}
	}

	// Set the association on the project and save it
	ProjectFile->SetStringField(TEXT("EngineAssociation"), Identifier);
	return SaveProjectFile(ProjectFileName, ProjectFile);
}

bool FDesktopPlatformBase::GetEngineIdentifierForProject(const FString &ProjectFileName, FString &OutIdentifier)
{
	OutIdentifier.Empty();

	// Load the project file
	TSharedPtr<FJsonObject> ProjectFile = LoadProjectFile(ProjectFileName);
	if(!ProjectFile.IsValid())
	{
		return false;
	}

	// Try to read the identifier from it
	TSharedPtr<FJsonValue> Value = ProjectFile->TryGetField(TEXT("EngineAssociation"));
	if(Value.IsValid() && Value->Type == EJson::String)
	{
		OutIdentifier = Value->AsString();
		if(OutIdentifier.Len() > 0)
		{
			return true;
		}
	}

	// Otherwise scan up through the directory hierarchy to find an installation
	FString ParentDir = FPaths::GetPath(ProjectFileName);
	FPaths::NormalizeDirectoryName(ParentDir);

	// Keep going until we reach the root
	int32 SeparatorIdx;
	while(ParentDir.FindLastChar(TEXT('/'), SeparatorIdx))
	{
		ParentDir.RemoveAt(SeparatorIdx, ParentDir.Len() - SeparatorIdx);
		if(IsValidRootDirectory(ParentDir) && GetEngineIdentifierFromRootDir(ParentDir, OutIdentifier))
		{
			return true;
		}
	}

	// Otherwise check the engine version string for 4.0, in case this project existed before the engine association stuff went in
	FString EngineVersionString = ProjectFile->GetStringField(TEXT("EngineVersion"));
	if(EngineVersionString.Len() > 0)
	{
		FEngineVersion EngineVersion;
		if(FEngineVersion::Parse(EngineVersionString, EngineVersion) && EngineVersion.IsPromotedBuild() && EngineVersion.ToString(EVersionComponent::Minor) == TEXT("4.0"))
		{
			OutIdentifier = TEXT("4.0");
			return true;
		}
	}

	return false;
}

bool FDesktopPlatformBase::OpenProject(const FString& ProjectFileName)
{
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*ProjectFileName);
	return true;
}

bool FDesktopPlatformBase::CleanGameProject(const FString &ProjectDir, FFeedbackContext* Warn)
{
	// Begin a task
	Warn->BeginSlowTask(LOCTEXT("CleaningProject", "Removing stale build products..."), true);

	// Enumerate all the files
	TArray<FString> FileNames;
	TArray<FString> DirectoryNames;
	GetProjectBuildProducts(ProjectDir, FileNames, DirectoryNames);

	// Remove all the files
	for(int32 Idx = 0; Idx < FileNames.Num(); Idx++)
	{
		// Remove the file
		if(!IFileManager::Get().Delete(*FileNames[Idx]))
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("ERROR: Couldn't delete file '%s'"), *FileNames[Idx]);
			return false;
		}

		// Update the progress
		Warn->UpdateProgress(Idx, FileNames.Num() + DirectoryNames.Num());
	}

	// Remove all the directories
	for(int32 Idx = 0; Idx < DirectoryNames.Num(); Idx++)
	{
		// Remove the directory
		if(!IFileManager::Get().DeleteDirectory(*DirectoryNames[Idx], false, true))
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("ERROR: Couldn't delete directory '%s'"), *DirectoryNames[Idx]);
			return false;
		}

		// Update the progress
		Warn->UpdateProgress(Idx + FileNames.Num(), FileNames.Num() + DirectoryNames.Num());
	}

	// End the task
	Warn->EndSlowTask();
	return true;
}

bool FDesktopPlatformBase::CompileGameProject(const FString& RootDir, const FString& ProjectFileName, FFeedbackContext* Warn)
{
	// Get the target name
	FString Arguments = FString::Printf(TEXT("%sEditor %s %s"), *FPaths::GetBaseFilename(ProjectFileName), FModuleManager::Get().GetUBTConfiguration(), FPlatformMisc::GetUBTPlatform());

	// Append the project name if it's a foreign project
	if ( !ProjectFileName.IsEmpty() )
	{
		FUProjectDictionary ProjectDictionary(RootDir);
		if(ProjectDictionary.IsForeignProject(ProjectFileName))
		{
			Arguments += FString::Printf(TEXT(" -project=\"%s\""), *IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*ProjectFileName));
		}
	}

	// Append the Rocket flag
	if(!IsSourceDistribution(RootDir))
	{
		Arguments += TEXT(" -rocket");
	}

	// Append any other options
	Arguments += " -editorrecompile -progress";

	// Run UBT
	return RunUnrealBuildTool(LOCTEXT("CompilingProject", "Compiling project..."), RootDir, Arguments, Warn);
}

bool FDesktopPlatformBase::GenerateProjectFiles(const FString& RootDir, const FString& ProjectFileName, FFeedbackContext* Warn)
{
#if PLATFORM_MAC
	FString Arguments = TEXT("-xcodeprojectfile");
#else
	FString Arguments = TEXT("-projectfiles");
#endif

	// Build the arguments to pass to UBT
	if ( !ProjectFileName.IsEmpty() )
	{
		// Figure out whether it's a foreign project
		const FUProjectDictionary &ProjectDictionary = GetCachedProjectDictionary(RootDir);
		if(ProjectDictionary.IsForeignProject(ProjectFileName))
		{
			Arguments += FString::Printf(TEXT(" -project=\"%s\""), *IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*ProjectFileName));

			// Always include game source
			Arguments += " -game";

			// Determine whether or not to include engine source
			if(IsSourceDistribution(RootDir))
			{
				Arguments += " -engine";
			}
			else
			{
				Arguments += " -rocket";
			}
		}
	}
	Arguments += " -progress";

	// Run UnrealBuildTool
	Warn->BeginSlowTask(LOCTEXT("GeneratingProjectFiles", "Generating project files..."), true, true);
	bool bRes = RunUnrealBuildTool(LOCTEXT("GeneratingProjectFiles", "Generating project files..."), RootDir, Arguments, Warn);
	Warn->EndSlowTask();
	return bRes;
}

FString FDesktopPlatformBase::GetDefaultProjectCreationPath()
{
	// My Documents
	const FString DefaultProjectSubFolder = TEXT("Unreal Projects");
	return FString(FPlatformProcess::UserDir()) + DefaultProjectSubFolder;
}

void FDesktopPlatformBase::ReadLauncherInstallationList()
{
	FString InstalledListFile = FString(FPlatformProcess::ApplicationSettingsDir()) / TEXT("UnrealEngineLauncher/LauncherInstalled.dat");

	// If the file does not exist, manually check for the 4.0 or 4.1 manifest
	FDateTime NewListTimestamp = IFileManager::Get().GetTimeStamp(*InstalledListFile);
	if(NewListTimestamp == FDateTime::MinValue())
	{
		if(LauncherInstallationList.Num() == 0)
		{
			CheckForLauncherEngineInstallation(TEXT("40003"), TEXT("UE_4.0"), LauncherInstallationList);
			CheckForLauncherEngineInstallation(TEXT("1040003"), TEXT("UE_4.1"), LauncherInstallationList);
		}
	}
	else if(NewListTimestamp != LauncherInstallationTimestamp)
	{
		// Read the installation manifest
		FString InstalledText;
		if (FFileHelper::LoadFileToString(InstalledText, *InstalledListFile))
		{
			// Deserialize the object
			TSharedPtr< FJsonObject > RootObject;
			TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(InstalledText);
			if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
			{
				// Parse the list of installations
				TArray< TSharedPtr<FJsonValue> > InstallationList = RootObject->GetArrayField(TEXT("InstallationList"));
				for(int32 Idx = 0; Idx < InstallationList.Num(); Idx++)
				{
					TSharedPtr<FJsonObject> InstallationItem = InstallationList[Idx]->AsObject();

					FString AppName = InstallationItem->GetStringField(TEXT("AppName"));
					FString InstallLocation = InstallationItem->GetStringField(TEXT("InstallLocation"));
					if(AppName.Len() > 0 && InstallLocation.Len() > 0)
					{
						FPaths::NormalizeDirectoryName(InstallLocation);
						LauncherInstallationList.Add(AppName, InstallLocation);
					}
				}
			}
			LauncherInstallationTimestamp = NewListTimestamp;
		}
	}
}

void FDesktopPlatformBase::CheckForLauncherEngineInstallation(const FString &AppId, const FString &Identifier, TMap<FString, FString> &OutInstallations)
{
	FString ManifestText;
	FString ManifestFileName = FString(FPlatformProcess::ApplicationSettingsDir()) / FString::Printf(TEXT("UnrealEngineLauncher/Data/Manifests/%s.manifest"), *AppId);
	if (FFileHelper::LoadFileToString(ManifestText, *ManifestFileName))
	{
		TSharedPtr< FJsonObject > RootObject;
		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(ManifestText);
		if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
		{
			TSharedPtr<FJsonObject> CustomFieldsObject = RootObject->GetObjectField(TEXT("CustomFields"));
			if (CustomFieldsObject.IsValid())
			{
				FString InstallLocation = CustomFieldsObject->GetStringField("InstallLocation");
				if (InstallLocation.Len() > 0)
				{
					OutInstallations.Add(Identifier, InstallLocation);
				}
			}
		}
	}
}

int32 FDesktopPlatformBase::ParseReleaseVersion(const FString &Version)
{
	TCHAR *End;

	uint64 Major = FCString::Strtoui64(*Version, &End, 10);
	if (Major >= MAX_int16 || *(End++) != '.')
	{
		return INDEX_NONE;
	}

	uint64 Minor = FCString::Strtoui64(End, &End, 10);
	if (Minor >= MAX_int16 || *End != 0)
	{
		return INDEX_NONE;
	}

	return (Major << 16) + Minor;
}

TSharedPtr<FJsonObject> FDesktopPlatformBase::LoadProjectFile(const FString &FileName)
{
	FString FileContents;

	if (!FFileHelper::LoadFileToString(FileContents, *FileName))
	{
		return TSharedPtr<FJsonObject>(NULL);
	}

	TSharedPtr< FJsonObject > JsonObject;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(FileContents);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return TSharedPtr<FJsonObject>(NULL);
	}

	return JsonObject;
}

bool FDesktopPlatformBase::SaveProjectFile(const FString &FileName, TSharedPtr<FJsonObject> Object)
{
	FString FileContents;

	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&FileContents);
	if (!FJsonSerializer::Serialize(Object.ToSharedRef(), Writer))
	{
		return false;
	}

	if (!FFileHelper::SaveStringToFile(FileContents, *FileName))
	{
		return false;
	}

	return true;
}

const FUProjectDictionary &FDesktopPlatformBase::GetCachedProjectDictionary(const FString& RootDir)
{
	FString NormalizedRootDir = RootDir;
	FPaths::NormalizeDirectoryName(NormalizedRootDir);

	FUProjectDictionary *Dictionary = CachedProjectDictionaries.Find(NormalizedRootDir);
	if(Dictionary == NULL)
	{
		Dictionary = &CachedProjectDictionaries.Add(RootDir, FUProjectDictionary(RootDir));
	}
	return *Dictionary;
}

void FDesktopPlatformBase::GetProjectBuildProducts(const FString& ProjectDir, TArray<FString> &OutFileNames, TArray<FString> &OutDirectoryNames)
{
	FString NormalizedProjectDir = ProjectDir;
	FPaths::NormalizeDirectoryName(NormalizedProjectDir);

	// Find all the build roots
	TArray<FString> BuildRootDirectories;
	BuildRootDirectories.Add(NormalizedProjectDir);

	// Add all the plugin directories
	TArray<FString> PluginFileNames;
	IFileManager::Get().FindFilesRecursive(PluginFileNames, *(NormalizedProjectDir / TEXT("Plugins")), TEXT("*.uplugin"), true, false);
	for(int32 Idx = 0; Idx < PluginFileNames.Num(); Idx++)
	{
		BuildRootDirectories.Add(FPaths::GetPath(PluginFileNames[Idx]));
	}

	// Add all the intermediate directories
	for(int32 Idx = 0; Idx < BuildRootDirectories.Num(); Idx++)
	{
		OutDirectoryNames.Add(BuildRootDirectories[Idx] / TEXT("Intermediate"));
	}

	// Add the files in the cleaned directories to the output list
	for(int32 Idx = 0; Idx < OutDirectoryNames.Num(); Idx++)
	{
		IFileManager::Get().FindFilesRecursive(OutFileNames, *OutDirectoryNames[Idx], TEXT("*"), true, false, false);
	}
}

bool FDesktopPlatformBase::EnumerateProjectsKnownByEngine(const FString &Identifier, bool bIncludeNativeProjects, TArray<FString> &OutProjectFileNames)
{
	// Get the engine root directory
	FString RootDir;
	if(!GetEngineRootDirFromIdentifier(Identifier, RootDir))
	{
		return false;
	}

	// Get the path to the game agnostic settings
	FString UserDir;
	if(IsStockEngineRelease(Identifier))
	{
		UserDir = FPaths::Combine(FPlatformProcess::UserSettingsDir(), *FString(EPIC_PRODUCT_IDENTIFIER), *Identifier);
	}
	else
	{
		UserDir = FPaths::Combine(*RootDir, TEXT("Engine"));
	}

	// Get the game agnostic config dir
	FString GameAgnosticConfigDir = UserDir / TEXT("Saved/Config") / ANSI_TO_TCHAR(FPlatformProperties::PlatformName());

	// Find all the created project directories. Start with the default project creation path.
	TArray<FString> SearchDirectories;
	SearchDirectories.AddUnique(GetDefaultProjectCreationPath());

	// Load the config file
	FConfigFile GameAgnosticConfig;
	FConfigCacheIni::LoadExternalIniFile(GameAgnosticConfig, TEXT("EditorGameAgnostic"), NULL, *GameAgnosticConfigDir, false);

	// Find the editor game-agnostic settings
	FConfigSection* Section = GameAgnosticConfig.Find(TEXT("/Script/UnrealEd.EditorGameAgnosticSettings"));
	if(Section != NULL)
	{
		// Add in every path that the user has ever created a project file. This is to catch new projects showing up in the user's project folders
		TArray<FString> AdditionalDirectories;
		Section->MultiFind(TEXT("CreatedProjectPaths"), AdditionalDirectories);
		for(int Idx = 0; Idx < AdditionalDirectories.Num(); Idx++)
		{
			FPaths::NormalizeDirectoryName(AdditionalDirectories[Idx]);
			SearchDirectories.AddUnique(AdditionalDirectories[Idx]);
		}

		// Also add in all the recently opened projects
		TArray<FString> RecentlyOpenedFiles;
		Section->MultiFind(TEXT("RecentlyOpenedProjectFiles"), RecentlyOpenedFiles);
		for(int Idx = 0; Idx < RecentlyOpenedFiles.Num(); Idx++)
		{
			FPaths::NormalizeFilename(RecentlyOpenedFiles[Idx]);
			OutProjectFileNames.AddUnique(RecentlyOpenedFiles[Idx]);
		}		
	}

	// Find all the other projects that are in the search directories
	for(int Idx = 0; Idx < SearchDirectories.Num(); Idx++)
	{
		TArray<FString> ProjectFolders;
		IFileManager::Get().FindFiles(ProjectFolders, *(SearchDirectories[Idx] / TEXT("*")), false, true);

		for(int32 FolderIdx = 0; FolderIdx < ProjectFolders.Num(); FolderIdx++)
		{
			TArray<FString> ProjectFiles;
			IFileManager::Get().FindFiles(ProjectFiles, *(SearchDirectories[Idx] / ProjectFolders[FolderIdx] / TEXT("*.uproject")), true, false);

			for(int32 FileIdx = 0; FileIdx < ProjectFiles.Num(); FileIdx++)
			{
				OutProjectFileNames.AddUnique(SearchDirectories[Idx] / ProjectFolders[FolderIdx] / ProjectFiles[FileIdx]);
			}
		}
	}

	// Find all the native projects, and either add or remove them from the list depending on whether we want native projects
	const FUProjectDictionary &Dictionary = GetCachedProjectDictionary(RootDir);
	if(bIncludeNativeProjects)
	{
		TArray<FString> NativeProjectPaths = Dictionary.GetProjectPaths();
		for(int Idx = 0; Idx < NativeProjectPaths.Num(); Idx++)
		{
			if(!NativeProjectPaths[Idx].Contains(TEXT("/Templates/")))
			{
				OutProjectFileNames.AddUnique(NativeProjectPaths[Idx]);
			}
		}
	}
	else
	{
		TArray<FString> NativeProjectPaths = Dictionary.GetProjectPaths();
		for(int Idx = 0; Idx < NativeProjectPaths.Num(); Idx++)
		{
			OutProjectFileNames.Remove(NativeProjectPaths[Idx]);
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
