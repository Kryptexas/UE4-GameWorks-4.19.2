// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GameProjectGenerationPrivatePCH.h"
#include "UnrealEdMisc.h"
#include "ISourceControlModule.h"
#include "MainFrame.h"

#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "EngineAnalytics.h"

#define LOCTEXT_NAMESPACE "GameProjectUtils"

#define MAX_PROJECT_PATH_BUFFER_SPACE 130 // Leave a reasonable buffer of additional characters to account for files created in the content directory during or after project generation
#define MAX_PROJECT_NAME_LENGTH 22 // Enforce a reasonable project name length so the path is not too long for PLATFORM_MAX_FILEPATH_LENGTH
checkAtCompileTime(PLATFORM_MAX_FILEPATH_LENGTH - MAX_PROJECT_PATH_BUFFER_SPACE > 0, filesystem_path_shorter_than_project_creation_buffer_space);

#define MAX_CLASS_NAME_LENGTH 32 // Enforce a reasonable class name length so the path is not too long for PLATFORM_MAX_FILEPATH_LENGTH

TWeakPtr<SNotificationItem> GameProjectUtils::UpdateGameProjectNotification = NULL;

bool GameProjectUtils::IsValidProjectFileForCreation(const FString& ProjectFile, FText& OutFailReason)
{
	const FString BaseProjectFile = FPaths::GetBaseFilename(ProjectFile);
	if ( FPaths::GetPath(ProjectFile).IsEmpty() )
	{
		OutFailReason = LOCTEXT( "NoProjectPath", "You must specify a path." );
		return false;
	}

	if ( BaseProjectFile.IsEmpty() )
	{
		OutFailReason = LOCTEXT( "NoProjectName", "You must specify a project name." );
		return false;
	}

	if ( BaseProjectFile.Contains(TEXT(" ")) )
	{
		OutFailReason = LOCTEXT( "ProjectNameContainsSpace", "Project names may not contain a space." );
		return false;
	}

	if ( !FChar::IsAlpha(BaseProjectFile[0]) )
	{
		OutFailReason = LOCTEXT( "ProjectNameMustBeginWithACharacter", "Project names must begin with an alphabetic character." );
		return false;
	}

	if ( BaseProjectFile.Len() > MAX_PROJECT_NAME_LENGTH )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("MaxProjectNameLength"), MAX_PROJECT_NAME_LENGTH );
		OutFailReason = FText::Format( LOCTEXT( "ProjectNameTooLong", "Project names must not be longer than {MaxProjectNameLength} characters." ), Args );
		return false;
	}

	const int32 MaxProjectPathLength = PLATFORM_MAX_FILEPATH_LENGTH - MAX_PROJECT_PATH_BUFFER_SPACE;
	if ( FPaths::GetBaseFilename(ProjectFile, false).Len() > MaxProjectPathLength )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("MaxProjectPathLength"), MaxProjectPathLength );
		OutFailReason = FText::Format( LOCTEXT( "ProjectPathTooLong", "A projects path must not be longer than {MaxProjectPathLength} characters." ), Args );
		return false;
	}

	if ( FPaths::GetExtension(ProjectFile) != IProjectManager::GetProjectFileExtension() )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ProjectFileExtension"), FText::FromString( IProjectManager::GetProjectFileExtension() ) );
		OutFailReason = FText::Format( LOCTEXT( "InvalidProjectFileExtension", "File extension is not {ProjectFileExtension}" ), Args );
		return false;
	}

	FString IllegalNameCharacters;
	if ( !NameContainsOnlyLegalCharacters(BaseProjectFile, IllegalNameCharacters) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("IllegalNameCharacters"), FText::FromString( IllegalNameCharacters ) );
		OutFailReason = FText::Format( LOCTEXT( "ProjectNameContainsIllegalCharacters", "Project names may not contain the following characters: {IllegalNameCharacters}" ), Args );
		return false;
	}

	FString IllegalPathCharacters;
	if ( !ProjectPathContainsOnlyLegalCharacters(FPaths::GetPath(ProjectFile), IllegalPathCharacters) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("IllegalPathCharacters"), FText::FromString( IllegalPathCharacters ) );
		OutFailReason = FText::Format( LOCTEXT( "ProjectPathContainsIllegalCharacters", "Project names may not contain the following characters: {IllegalNameCharacters}" ), Args );
		return false;
	}

	if ( ProjectFileExists(ProjectFile) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ProjectFile"), FText::FromString( ProjectFile ) );
		OutFailReason = FText::Format( LOCTEXT( "ProjectFileAlreadyExists", "{ProjectFile} already exists." ), Args );
		return false;
	}

	if ( FPaths::ConvertRelativePathToFull(FPaths::GetPath(ProjectFile)).StartsWith( FPaths::ConvertRelativePathToFull(FPaths::EngineDir())) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ProjectFile"), FText::FromString( ProjectFile ) );
		OutFailReason = FText::Format( LOCTEXT( "ProjectFileCannotBeUnderEngineFolder", "{ProjectFile} cannot be saved under the Engine folder.  Create the project in a different directory." ), Args );
		return false;
	}

	if ( AnyProjectFilesExistInFolder(FPaths::GetPath(ProjectFile)) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ProjectFileExtension"), FText::FromString( IProjectManager::GetProjectFileExtension() ) );
		Args.Add( TEXT("ProjectFilePath"), FText::FromString( FPaths::GetPath(ProjectFile) ) );
		OutFailReason = FText::Format( LOCTEXT( "AProjectFileAlreadyExistsAtLoction", "Another .{ProjectFileExtension} file already exists in {ProjectFilePath}" ), Args );
		return false;
	}

	return true;
}

bool GameProjectUtils::OpenProject(const FString& ProjectFile, FText& OutFailReason)
{
	if ( ProjectFile.IsEmpty() )
	{
		OutFailReason = LOCTEXT( "NoProjectFileSpecified", "You must specify a project file." );
		return false;
	}

	const FString BaseProjectFile = FPaths::GetBaseFilename(ProjectFile);
	if ( BaseProjectFile.Contains(TEXT(" ")) )
	{
		OutFailReason = LOCTEXT( "ProjectNameContainsSpace", "Project names may not contain a space." );
		return false;
	}

	if ( !FChar::IsAlpha(BaseProjectFile[0]) )
	{
		OutFailReason = LOCTEXT( "ProjectNameMustBeginWithACharacter", "Project names must begin with an alphabetic character." );
		return false;
	}

	if ( BaseProjectFile.Len() > MAX_PROJECT_NAME_LENGTH )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("MaxProjectNameLength"), MAX_PROJECT_NAME_LENGTH );
		OutFailReason = FText::Format( LOCTEXT( "ProjectNameTooLong", "Project names must not be longer than {MaxProjectNameLength} characters." ), Args );
		return false;
	}

	const int32 MaxProjectPathLength = PLATFORM_MAX_FILEPATH_LENGTH - MAX_PROJECT_PATH_BUFFER_SPACE;
	if ( FPaths::GetBaseFilename(ProjectFile, false).Len() > MaxProjectPathLength )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("MaxProjectPathLength"), MaxProjectPathLength );
		OutFailReason = FText::Format( LOCTEXT( "ProjectPathTooLong", "A projects path must not be longer than {MaxProjectPathLength} characters." ), Args );
		return false;
	}

	if ( FPaths::GetExtension(ProjectFile) != IProjectManager::GetProjectFileExtension() )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ProjectFileExtension"), FText::FromString( IProjectManager::GetProjectFileExtension() ) );
		OutFailReason = FText::Format( LOCTEXT( "InvalidProjectFileExtension", "File extension is not {ProjectFileExtension}" ), Args );
		return false;
	}

	FString IllegalNameCharacters;
	if ( !NameContainsOnlyLegalCharacters(BaseProjectFile, IllegalNameCharacters) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("IllegalNameCharacters"), FText::FromString( IllegalNameCharacters ) );
		OutFailReason = FText::Format( LOCTEXT( "ProjectNameContainsIllegalCharacters", "Project names may not contain the following characters: {IllegalNameCharacters}" ), Args );
		return false;
	}

	FString IllegalPathCharacters;
	if ( !ProjectPathContainsOnlyLegalCharacters(FPaths::GetPath(ProjectFile), IllegalPathCharacters) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("IllegalPathCharacters"), FText::FromString( IllegalPathCharacters ) );
		OutFailReason = FText::Format( LOCTEXT( "ProjectPathContainsIllegalCharacters", "A projects path may not contain the following characters: {IllegalPathCharacters}" ), Args );
		return false;
	}

	if ( !ProjectFileExists(ProjectFile) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ProjectFile"), FText::FromString( ProjectFile ) );
		OutFailReason = FText::Format( LOCTEXT( "ProjectFileDoesNotExist", "{ProjectFile} does not exist." ), Args );
		return false;
	}

	FUnrealEdMisc::Get().SwitchProject(ProjectFile, false);

	return true;
}

bool GameProjectUtils::OpenCodeIDE(const FString& ProjectFile, FText& OutFailReason)
{
	if ( ProjectFile.IsEmpty() )
	{
		OutFailReason = LOCTEXT( "NoProjectFileSpecified", "You must specify a project file." );
		return false;
	}

	bool bIsInRootFolder = false;
	if ( !FRocketSupport::IsRocket() )
	{
		// If we are in the UE4 root, just open the UE4.sln file, otherwise open the generated one.
		FString AbsoluteProjectParentFolder = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::GetPath(FPaths::GetPath(ProjectFile)));
		FString AbsoluteRootPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::RootDir());

		if ( AbsoluteProjectParentFolder.Right(1) != TEXT("/") )
		{
			AbsoluteProjectParentFolder += TEXT("/");
		}

		if ( AbsoluteRootPath.Right(1) != TEXT("/") )
		{
			AbsoluteRootPath += TEXT("/");
		}
		
		bIsInRootFolder = (AbsoluteProjectParentFolder == AbsoluteRootPath);
	}

	FString SolutionFolder;
	FString SolutionFilenameWithoutExtension;
	if ( bIsInRootFolder )
	{
		SolutionFolder = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::RootDir());
		SolutionFilenameWithoutExtension = TEXT("UE4");
	}
	else
	{
		SolutionFolder = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::GetPath(ProjectFile));
		SolutionFilenameWithoutExtension = FPaths::GetBaseFilename(ProjectFile);
	}

	FString CodeSolutionFile;
#if PLATFORM_WINDOWS
	CodeSolutionFile = SolutionFilenameWithoutExtension + TEXT(".sln");
#elif PLATFORM_MAC
	CodeSolutionFile = SolutionFilenameWithoutExtension + TEXT(".xcodeproj");
#else
	OutFailReason = LOCTEXT( "OpenCodeIDE_UnknownPlatform", "could not open the code editing IDE. The operating system is unknown." ).ToString();
	return false;
#endif

	const FString FullPath = FPaths::Combine(*SolutionFolder, *CodeSolutionFile);

#if PLATFORM_MAC
	if ( IFileManager::Get().DirectoryExists(*FullPath) )
#else
	if ( FPaths::FileExists(FullPath) )
#endif
	{
		FPlatformProcess::LaunchFileInDefaultExternalApplication( *FullPath );
		return true;
	}
	else
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("Path"), FText::FromString( FullPath ) );
		OutFailReason = FText::Format( LOCTEXT( "OpenCodeIDE_MissingFile", "Could not edit the code editing IDE. {Path} could not be found." ), Args );
		return false;
	}
}

void GameProjectUtils::GetStarterContentFiles(TArray<FString>& OutFilenames)
{
	FString const SrcFolder = FPaths::StarterContentDir();
	FString const ContentFolder = SrcFolder / TEXT("Content");
	FString const DDCFolder = SrcFolder / TEXT("DerivedDataCache");

	// only copying /Content and /DerivedDataCache
	IFileManager::Get().FindFilesRecursive(OutFilenames, *ContentFolder, TEXT("*"), /*Files=*/true, /*Directories=*/false);
	IFileManager::Get().FindFilesRecursive(OutFilenames, *DDCFolder, TEXT("*"), /*Files=*/true, /*Directories=*/false, /*bClearFilenames=*/false);
}

bool GameProjectUtils::CopyStarterContent(const FString& DestProjectFolder, FText& OutFailReason)
{
	FString const SrcFolder = FPaths::StarterContentDir();

	TArray<FString> FilesToCopy;
	GetStarterContentFiles(FilesToCopy);

	TArray<FString> CreatedFiles;
	for (FString SrcFilename : FilesToCopy)
	{
		// Update the slow task dialog
		const bool bAllowNewSlowTask = false;
		FFormatNamedArguments Args;
		Args.Add(TEXT("SrcFilename"), FText::FromString(FPaths::GetCleanFilename(SrcFilename)));
		FStatusMessageContext SlowTaskMessage(FText::Format(LOCTEXT("CreatingProjectStatus_CopyingFile", "Copying File {SrcFilename}..."), Args), bAllowNewSlowTask);

		FString FileRelPath = FPaths::GetPath(SrcFilename);
		FPaths::MakePathRelativeTo(FileRelPath, *SrcFolder);

		// Perform the copy. For file collisions, leave existing file.
		const FString DestFilename = DestProjectFolder + TEXT("/") + FileRelPath + TEXT("/") + FPaths::GetCleanFilename(SrcFilename);
		if (!FPaths::FileExists(DestFilename))
		{
			if (IFileManager::Get().Copy(*DestFilename, *SrcFilename, false) == COPY_OK)
			{
				CreatedFiles.Add(DestFilename);
			}
			else
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("SrcFilename"), FText::FromString(SrcFilename));
				Args.Add(TEXT("DestFilename"), FText::FromString(DestFilename));
				OutFailReason = FText::Format(LOCTEXT("FailedToCopyFile", "Failed to copy \"{SrcFilename}\" to \"{DestFilename}\"."), Args);
				DeleteCreatedFiles(DestProjectFolder, CreatedFiles);
				return false;
			}
		}
	}

	return true;
}


bool GameProjectUtils::CreateProject(const FString& NewProjectFile, const FString& TemplateFile, bool bShouldGenerateCode, bool bCopyStarterContent, FText& OutFailReason)
{
	if ( !IsValidProjectFileForCreation(NewProjectFile, OutFailReason) )
	{
		return false;
	}

	const bool bAllowNewSlowTask = true;
	FStatusMessageContext SlowTaskMessage( LOCTEXT( "CreatingProjectStatus", "Creating project..." ), bAllowNewSlowTask );

	bool bProjectCreationSuccessful = false;
	FString TemplateName;
	if ( TemplateFile.IsEmpty() )
	{
		bProjectCreationSuccessful = GenerateProjectFromScratch(NewProjectFile, bShouldGenerateCode, bCopyStarterContent, OutFailReason);
		TemplateName = bShouldGenerateCode ? TEXT("Basic Code") : TEXT("Blank");
	}
	else
	{
		bProjectCreationSuccessful = CreateProjectFromTemplate(NewProjectFile, TemplateFile, bShouldGenerateCode, bCopyStarterContent, OutFailReason);
		TemplateName = FPaths::GetBaseFilename(TemplateFile);
	}

	if( FEngineAnalytics::IsAvailable() )
	{
		TArray<FAnalyticsEventAttribute> EventAttributes;
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("Template"), TemplateName));
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("ProjectType"), bShouldGenerateCode ? TEXT("C++ Code") : TEXT("Content Only")));
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("Outcome"), bProjectCreationSuccessful ? TEXT("Successful") : TEXT("Failed")));

		FEngineAnalytics::GetProvider().RecordEvent( TEXT( "Editor.NewProject.ProjectCreated" ), EventAttributes );
	}

	return bProjectCreationSuccessful;
}

bool GameProjectUtils::BuildGameBinaries(const FString& ProjectFilename, FText& OutFailReason)
{
	const bool bAllowNewSlowTask = true;
	FStatusMessageContext SlowTaskMessage( LOCTEXT( "BuildingProjectStatus", "Building project..." ), bAllowNewSlowTask );

	// Compile the *editor* for the project
	if ( FModuleManager::Get().CompileGameProjectEditor(ProjectFilename, *GLog) )
	{
		return true;
	}

	FFormatNamedArguments Args;
	Args.Add( TEXT("ProjectFilename"), FText::FromString( ProjectFilename ) );
	OutFailReason = FText::Format( LOCTEXT("FailedToCompileNewProject", "Failed to compile {ProjectFileName}."), Args );
	return false;
}

void GameProjectUtils::CheckForOutOfDateGameProjectFile()
{
	const FString& LoadedProjectFilePath = FPaths::IsProjectFilePathSet() ? FPaths::GetProjectFilePath() : FString();
	if ( !LoadedProjectFilePath.IsEmpty() )
	{
		FProjectStatus ProjectStatus;
		if ( IProjectManager::Get().QueryStatusForProject(LoadedProjectFilePath, ProjectStatus) )
		{
			if ( !ProjectStatus.bUpToDate )
			{
				// For the time being, if it's writable, just overwrite it with the latest info. If we need to do anything potentially destructive, we can change this logic for this in future builds.
				bool bHaveUpdatedProject = false;
				if (!FPlatformFileManager::Get().GetPlatformFile().IsReadOnly(*LoadedProjectFilePath))
				{
					FText FailureReason;
					if (IProjectManager::Get().UpdateLoadedProjectFileToCurrent(NULL, FailureReason))
					{
						bHaveUpdatedProject = true;
					}
					else
					{
						UE_LOG(LogGameProjectGeneration, Error, TEXT("Failed to auto-update project file %s"), *LoadedProjectFilePath, *FailureReason.ToString());
					}
				}

				// Otherwise, fall back to prompting
				if (!bHaveUpdatedProject)
				{
					const FText UpdateProjectText = LOCTEXT("UpdateProjectFilePrompt", "The project file you have loaded is not up to date. Would you like to update it now?");
					const FText UpdateProjectConfirmText = LOCTEXT("UpdateProjectFileConfirm", "Update");
					const FText UpdateProjectCancelText = LOCTEXT("UpdateProjectFileCancel", "Not Now");

					FNotificationInfo Info(UpdateProjectText);
					Info.bFireAndForget = false;
					Info.bUseLargeFont = false;
					Info.bUseThrobber = false;
					Info.bUseSuccessFailIcons = false;
					Info.FadeOutDuration = 3.f;
					Info.ButtonDetails.Add(FNotificationButtonInfo(UpdateProjectConfirmText, FText(), FSimpleDelegate::CreateStatic(&GameProjectUtils::OnUpdateProjectConfirm)));
					Info.ButtonDetails.Add(FNotificationButtonInfo(UpdateProjectCancelText, FText(), FSimpleDelegate::CreateStatic(&GameProjectUtils::OnUpdateProjectCancel)));

					if (UpdateGameProjectNotification.IsValid())
					{
						UpdateGameProjectNotification.Pin()->ExpireAndFadeout();
						UpdateGameProjectNotification.Reset();
					}

					UpdateGameProjectNotification = FSlateNotificationManager::Get().AddNotification(Info);

					if (UpdateGameProjectNotification.IsValid())
					{
						UpdateGameProjectNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
					}
				}
			}
		}
	}
}

bool GameProjectUtils::UpdateGameProject()
{
	const FString& ProjectFilename = FPaths::IsProjectFilePathSet() ? FPaths::GetProjectFilePath() : FString();
	if ( !ProjectFilename.IsEmpty() )
	{
		FProjectStatus ProjectStatus;
		if ( IProjectManager::Get().QueryStatusForProject(ProjectFilename, ProjectStatus) )
		{
			if ( ProjectStatus.bUpToDate )
			{
				// The project was already up to date.
				UE_LOG(LogGameProjectGeneration, Log, TEXT("%s is already up to date."), *ProjectFilename );
			}
			else
			{
				FText FailReason;
				bool bWasCheckedOut = false;
				if ( !UpdateGameProjectFile(ProjectFilename, NULL, bWasCheckedOut, FailReason) )
				{
					// The user chose to update, but the update failed. Notify the user.
					UE_LOG(LogGameProjectGeneration, Error, TEXT("%s failed to update. %s"), *ProjectFilename, *FailReason.ToString() );
					return false;
				}

				// The project was updated successfully.
				UE_LOG(LogGameProjectGeneration, Log, TEXT("%s was successfully updated."), *ProjectFilename );
			}
		}
	}

	return true;
}

void GameProjectUtils::OpenAddCodeToProjectDialog()
{
	TSharedRef<SWindow> AddCodeWindow =
		SNew(SWindow)
		.Title(LOCTEXT( "AddCodeWindowHeader", "Add Code"))
		.ClientSize( FVector2D(1280, 720) )
		.SizingRule( ESizingRule::FixedSize )
		.SupportsMinimize(false) .SupportsMaximize(false);

	AddCodeWindow->SetContent( SNew(SNewClassDialog) );

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	if (MainFrameModule.GetParentWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(AddCodeWindow, MainFrameModule.GetParentWindow().ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(AddCodeWindow);
	}
}

bool GameProjectUtils::IsValidClassNameForCreation(const FString& NewClassName, FText& OutFailReason)
{
	if ( NewClassName.IsEmpty() )
	{
		OutFailReason = LOCTEXT( "NoClassName", "You must specify a class name." );
		return false;
	}

	if ( NewClassName.Contains(TEXT(" ")) )
	{
		OutFailReason = LOCTEXT( "ClassNameContainsSpace", "Your class name may not contain a space." );
		return false;
	}

	if ( !FChar::IsAlpha(NewClassName[0]) )
	{
		OutFailReason = LOCTEXT( "ClassNameMustBeginWithACharacter", "Your class name must begin with an alphabetic character." );
		return false;
	}

	if ( NewClassName.Len() > MAX_CLASS_NAME_LENGTH )
	{
		OutFailReason = FText::Format( LOCTEXT( "ClassNameTooLong", "The class name must not be longer than {0} characters." ), FText::AsNumber(MAX_CLASS_NAME_LENGTH) );
		return false;
	}

	FString IllegalNameCharacters;
	if ( !NameContainsOnlyLegalCharacters(NewClassName, IllegalNameCharacters) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("IllegalNameCharacters"), FText::FromString( IllegalNameCharacters ) );
		OutFailReason = FText::Format( LOCTEXT( "ClassNameContainsIllegalCharacters", "The class name may not contain the following characters: {IllegalNameCharacters}" ), Args );
		return false;
	}

	// Look for a duplicate class in memory
	for ( TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt )
	{
		if ( ClassIt->GetName() == NewClassName )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("NewClassName"), FText::FromString( NewClassName ) );
			OutFailReason = FText::Format( LOCTEXT("ClassNameAlreadyExists", "The name {NewClassName} is already used by another class."), Args );
			return false;
		}
	}

	// Look for a duplicate class on disk in their project
	TArray<FString> Filenames;
	IFileManager::Get().FindFilesRecursive(Filenames, *FPaths::GameSourceDir(), TEXT("*.h"), true, false, false);
	for ( auto FileIt = Filenames.CreateConstIterator(); FileIt; ++FileIt )
	{
		const FString& File = *FileIt;
		if ( NewClassName == FPaths::GetBaseFilename(File) )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("NewClassName"), FText::FromString( NewClassName ) );
			OutFailReason = FText::Format( LOCTEXT("ClassNameAlreadyExists", "The name {NewClassName} is already used by another class."), Args );
			return false;
		}
	}

	return true;
}

bool GameProjectUtils::AddCodeToProject(const FString& NewClassName, const UClass* ParentClass, FString& OutHeaderFilePath, FString& OutCppFilePath, FText& OutFailReason)
{
	const bool bAddCodeSuccessful = AddCodeToProject_Internal(NewClassName, ParentClass, OutHeaderFilePath, OutCppFilePath, OutFailReason);

	if( FEngineAnalytics::IsAvailable() )
	{
		TArray<FAnalyticsEventAttribute> EventAttributes;
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("ParentClass"), ParentClass ? ParentClass->GetName() : TEXT("None")));
		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("Outcome"), bAddCodeSuccessful ? TEXT("Successful") : TEXT("Failed")));

		FEngineAnalytics::GetProvider().RecordEvent( TEXT( "Editor.AddCodeToProject.CodeAdded" ), EventAttributes );
	}

	return bAddCodeSuccessful;
}

UTemplateProjectDefs* GameProjectUtils::LoadTemplateDefs(const FString& ProjectDirectory)
{
	UTemplateProjectDefs* TemplateDefs = NULL;

	const FString TemplateDefsIniFilename = ProjectDirectory / TEXT("Config") / GetTemplateDefsFilename();
	if ( FPlatformFileManager::Get().GetPlatformFile().FileExists(*TemplateDefsIniFilename) )
	{
		TemplateDefs = ConstructObject<UTemplateProjectDefs>(UTemplateProjectDefs::StaticClass());
		TemplateDefs->LoadConfig(UTemplateProjectDefs::StaticClass(), *TemplateDefsIniFilename);
	}

	return TemplateDefs;
}

FString GameProjectUtils::GetDefaultProjectCreationPath()
{
	// My Documents
	const FString DefaultProjectSubFolder = TEXT("Unreal Projects");
	return FString(FPlatformProcess::UserDir()) + DefaultProjectSubFolder;
}

bool GameProjectUtils::GenerateProjectFromScratch(const FString& NewProjectFile, bool bShouldGenerateCode, bool bCopyStarterContent, FText& OutFailReason)
{
	const FString NewProjectFolder = FPaths::GetPath(NewProjectFile);
	const FString NewProjectName = FPaths::GetBaseFilename(NewProjectFile);
	TArray<FString> CreatedFiles;

	// Generate config files
	if (!GenerateConfigFiles(NewProjectFolder, NewProjectName, bShouldGenerateCode, bCopyStarterContent, CreatedFiles, OutFailReason))
	{
		DeleteCreatedFiles(NewProjectFolder, CreatedFiles);
		return false;
	}

	// Make the Content folder
	const FString ContentFolder = NewProjectFolder / TEXT("Content");
	if ( !IFileManager::Get().MakeDirectory(*ContentFolder) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ContentFolder"), FText::FromString( ContentFolder ) );
		OutFailReason = FText::Format( LOCTEXT("FailedToCreateContentFolder", "Failed to create the content folder {ContentFolder}"), Args );
		DeleteCreatedFiles(NewProjectFolder, CreatedFiles);
		return false;
	}

	TArray<FString> StartupModuleNames;
	if ( bShouldGenerateCode )
	{
		// Generate basic source code files
		if ( !GenerateBasicSourceCode(NewProjectFolder / TEXT("Source"), NewProjectName, StartupModuleNames, CreatedFiles, OutFailReason) )
		{
			DeleteCreatedFiles(NewProjectFolder, CreatedFiles);
			return false;
		}

		// Generate game framework source code files
		if ( !GenerateGameFrameworkSourceCode(NewProjectFolder / TEXT("Source"), NewProjectName, CreatedFiles, OutFailReason) )
		{
			DeleteCreatedFiles(NewProjectFolder, CreatedFiles);
			return false;
		}
	}

	// Generate the project file
	{
		FText LocalFailReason;
		if ( IProjectManager::Get().GenerateNewProjectFile(NewProjectFile, StartupModuleNames, LocalFailReason) )
		{
			CreatedFiles.Add(NewProjectFile);
		}
		else
		{
			OutFailReason = LocalFailReason;
			DeleteCreatedFiles(NewProjectFolder, CreatedFiles);
			return false;
		}
	}

	if ( bShouldGenerateCode )
	{
		// Generate project files
		if ( !GenerateCodeProjectFiles(NewProjectFile, OutFailReason) )
		{
			DeleteGeneratedProjectFiles(NewProjectFile);
			DeleteCreatedFiles(NewProjectFolder, CreatedFiles);
			return false;
		}
	}

	if (bCopyStarterContent)
	{
		// Copy the starter content
		if ( !CopyStarterContent(NewProjectFolder, OutFailReason) )
		{
			DeleteGeneratedProjectFiles(NewProjectFile);
			DeleteCreatedFiles(NewProjectFolder, CreatedFiles);
			return false;
		}
	}

	UE_LOG(LogGameProjectGeneration, Log, TEXT("Created new project with %d files (plus project files)"), CreatedFiles.Num());
	return true;
}

struct FConfigValue
{
	FString ConfigFile;
	FString ConfigSection;
	FString ConfigKey;
	FString ConfigValue;
	bool bShouldReplaceExistingValue;

	FConfigValue(const FString& InFile, const FString& InSection, const FString& InKey, const FString& InValue, bool InShouldReplaceExistingValue)
		: ConfigFile(InFile)
		, ConfigSection(InSection)
		, ConfigKey(InKey)
		, ConfigValue(InValue)
		, bShouldReplaceExistingValue(InShouldReplaceExistingValue)
	{}
};

bool GameProjectUtils::CreateProjectFromTemplate(const FString& NewProjectFile, const FString& TemplateFile, bool bShouldGenerateCode, bool bCopyStarterContent, FText& OutFailReason)
{
	const FString ProjectName = FPaths::GetBaseFilename(NewProjectFile);
	const FString TemplateName = FPaths::GetBaseFilename(TemplateFile);
	const FString SrcFolder = FPaths::GetPath(TemplateFile);
	const FString DestFolder = FPaths::GetPath(NewProjectFile);

	if ( !FPlatformFileManager::Get().GetPlatformFile().FileExists(*TemplateFile) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("TemplateFile"), FText::FromString( TemplateFile ) );
		OutFailReason = FText::Format( LOCTEXT("InvalidTemplate_MissingProject", "Template project \"{TemplateFile}\" does not exist."), Args );
		return false;
	}

	UTemplateProjectDefs* TemplateDefs = LoadTemplateDefs(SrcFolder);
	if ( TemplateDefs == NULL )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("TemplateFile"), FText::FromString( FPaths::GetBaseFilename(TemplateFile) ) );
		Args.Add( TEXT("TemplateDefinesFile"), FText::FromString( GetTemplateDefsFilename() ) );
		OutFailReason = FText::Format( LOCTEXT("InvalidTemplate_MissingDefs", "Template project \"{TemplateFile}\" does not have definitions file: '{TemplateDefinesFile}'."), Args );
		return false;
	}

	// Fix up the replacement strings using the specified project name
	TemplateDefs->FixupStrings(TemplateName, ProjectName);

	// Form a list of all extensions we care about
	TSet<FString> ReplacementsInFilesExtensions;
	for ( auto ReplacementIt = TemplateDefs->ReplacementsInFiles.CreateConstIterator(); ReplacementIt; ++ReplacementIt )
	{
		ReplacementsInFilesExtensions.Append((*ReplacementIt).Extensions);
	}

	// Keep a list of created files so we can delete them if project creation fails
	TArray<FString> CreatedFiles;

	// Discover and copy all files in the src folder to the destination, excluding a few files and folders
	TArray<FString> FilesToCopy;
	TArray<FString> FilesThatNeedContentsReplaced;
	TMap<FString, FString> ClassRenames;
	IFileManager::Get().FindFilesRecursive(FilesToCopy, *SrcFolder, TEXT("*"), /*Files=*/true, /*Directories=*/false);
	for ( auto FileIt = FilesToCopy.CreateConstIterator(); FileIt; ++FileIt )
	{
		const FString SrcFilename = (*FileIt);

		// Get the file path, relative to the src folder
		const FString SrcFileSubpath = SrcFilename.RightChop(SrcFolder.Len() + 1);

		// Skip any files that were configured to be ignored
		bool bThisFileIsIgnored = false;
		for ( auto IgnoreIt = TemplateDefs->FilesToIgnore.CreateConstIterator(); IgnoreIt; ++IgnoreIt )
		{
			if ( SrcFileSubpath == *IgnoreIt )
			{
				// This file was marked as "ignored"
				bThisFileIsIgnored = true;
				break;
			}
		}

		if ( bThisFileIsIgnored )
		{
			// This file was marked as "ignored"
			continue;
		}

		// Skip any folders that were configured to be ignored
		bool bThisFolderIsIgnored = false;
		for ( auto IgnoreIt = TemplateDefs->FoldersToIgnore.CreateConstIterator(); IgnoreIt; ++IgnoreIt )
		{
			if ( SrcFileSubpath.StartsWith((*IgnoreIt) + TEXT("/") ) )
			{
				// This folder was marked as "ignored"
				bThisFolderIsIgnored = true;
				break;
			}
		}

		if ( bThisFolderIsIgnored )
		{
			// This folder was marked as "ignored"
			continue;
		}

		// Update the slow task dialog
		const bool bAllowNewSlowTask = false;
		FFormatNamedArguments Args;
		Args.Add( TEXT("SrcFilename"), FText::FromString( FPaths::GetCleanFilename(SrcFilename) ) );
		FStatusMessageContext SlowTaskMessage( FText::Format( LOCTEXT( "CreatingProjectStatus_CopyingFile", "Copying File {SrcFilename}..." ), Args ), bAllowNewSlowTask );

		// Retarget any folders that were chosen to be renamed by choosing a new destination subpath now
		FString DestFileSubpathWithoutFilename = FPaths::GetPath(SrcFileSubpath) + TEXT("/");
		for ( auto RenameIt = TemplateDefs->FolderRenames.CreateConstIterator(); RenameIt; ++RenameIt )
		{
			const FTemplateFolderRename& FolderRename = *RenameIt;
			if ( SrcFileSubpath.StartsWith(FolderRename.From + TEXT("/")) )
			{
				// This was a file in a renamed folder. Retarget to the new location
				DestFileSubpathWithoutFilename = FolderRename.To / DestFileSubpathWithoutFilename.RightChop( FolderRename.From.Len() );
			}
		}

		// Retarget any files that were chosen to have parts of their names replaced here
		FString DestBaseFilename = FPaths::GetBaseFilename(SrcFileSubpath);
		const FString FileExtension = FPaths::GetExtension(SrcFileSubpath);
		for ( auto ReplacementIt = TemplateDefs->FilenameReplacements.CreateConstIterator(); ReplacementIt; ++ReplacementIt )
		{
			const FTemplateReplacement& Replacement = *ReplacementIt;
			if ( Replacement.Extensions.Contains( FileExtension ) )
			{
				// This file matched a filename replacement extension, apply it now
				DestBaseFilename = DestBaseFilename.Replace(*Replacement.From, *Replacement.To, Replacement.bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase);
			}
		}

		// Perform the copy
		const FString DestFilename = DestFolder / DestFileSubpathWithoutFilename + DestBaseFilename + TEXT(".") + FileExtension;
		if ( IFileManager::Get().Copy(*DestFilename, *SrcFilename) == COPY_OK )
		{
			CreatedFiles.Add(DestFilename);

			if ( ReplacementsInFilesExtensions.Contains(FileExtension) )
			{
				FilesThatNeedContentsReplaced.Add(DestFilename);
			}

			if ( FileExtension == TEXT("h")															// A header file
				&& FPaths::GetBaseFilename(SrcFilename) != FPaths::GetBaseFilename(DestFilename))	// Whose name changed
			{
				FString FileContents;
				if( ensure( FFileHelper::LoadFileToString( FileContents, *DestFilename ) ) )
				{
					// @todo uht: Checking file contents to see if this is a UObject class.  Sort of fragile here.
					if( FileContents.Contains( TEXT( ".generated.h\"" ), ESearchCase::IgnoreCase ) )
					{
						// Looks like a UObject header!
						ClassRenames.Add(FPaths::GetBaseFilename(SrcFilename), FPaths::GetBaseFilename(DestFilename));
					}
				}
			}
		}
		else
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("SrcFilename"), FText::FromString( SrcFilename ) );
			Args.Add( TEXT("DestFilename"), FText::FromString( DestFilename ) );
			OutFailReason = FText::Format( LOCTEXT("FailedToCopyFile", "Failed to copy \"{SrcFilename}\" to \"{DestFilename}\"."), Args );
			DeleteCreatedFiles(DestFolder, CreatedFiles);
			return false;
		}
	}

	// Open all files with the specified extensions and replace text
	for ( auto FileIt = FilesThatNeedContentsReplaced.CreateConstIterator(); FileIt; ++FileIt )
	{
		const FString FileToFix = *FileIt;
		bool bSuccessfullyProcessed = false;

		FString FileContents;
		if ( FFileHelper::LoadFileToString(FileContents, *FileToFix) )
		{
			for ( auto ReplacementIt = TemplateDefs->ReplacementsInFiles.CreateConstIterator(); ReplacementIt; ++ReplacementIt )
			{
				const FTemplateReplacement& Replacement = *ReplacementIt;
				if ( Replacement.Extensions.Contains( FPaths::GetExtension(FileToFix) ) )
				{
					FileContents = FileContents.Replace(*Replacement.From, *Replacement.To, Replacement.bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase);
				}
			}

			if ( FFileHelper::SaveStringToFile(FileContents, *FileToFix) )
			{
				bSuccessfullyProcessed = true;
			}
		}

		if ( !bSuccessfullyProcessed )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("FileToFix"), FText::FromString( FileToFix ) );
			OutFailReason = FText::Format( LOCTEXT("FailedToFixUpFile", "Failed to process file \"{FileToFix}\"."), Args );
			DeleteCreatedFiles(DestFolder, CreatedFiles);
			return false;
		}
	}

	// Fixup specific ini values
	TArray<FConfigValue> ConfigValuesToSet;
	const FString ActiveGameNameRedirectsValue_LongName = FString::Printf(TEXT("(OldGameName=\"/Script/%s\",NewGameName=\"/Script/%s\")"), *TemplateName, *ProjectName);
	const FString ActiveGameNameRedirectsValue_ShortName = FString::Printf(TEXT("(OldGameName=\"%s\",NewGameName=\"/Script/%s\")"), *TemplateName, *ProjectName);
	new (ConfigValuesToSet) FConfigValue(TEXT("DefaultEngine.ini"), TEXT("/Script/Engine.Engine"), TEXT("+ActiveGameNameRedirects"), *ActiveGameNameRedirectsValue_LongName, /*InShouldReplaceExistingValue=*/false);
	new (ConfigValuesToSet) FConfigValue(TEXT("DefaultEngine.ini"), TEXT("/Script/Engine.Engine"), TEXT("+ActiveGameNameRedirects"), *ActiveGameNameRedirectsValue_ShortName, /*InShouldReplaceExistingValue=*/false);
	new (ConfigValuesToSet) FConfigValue(TEXT("DefaultGame.ini"), TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectID"), FGuid::NewGuid().ToString(), /*InShouldReplaceExistingValue=*/true);

	// Add all classname fixups
	for ( auto RenameIt = ClassRenames.CreateConstIterator(); RenameIt; ++RenameIt )
	{
		const FString ClassRedirectString = FString::Printf(TEXT("(OldClassName=\"%s\",NewClassName=\"%s\")"), *RenameIt.Key(), *RenameIt.Value());
		new (ConfigValuesToSet) FConfigValue(TEXT("DefaultEngine.ini"), TEXT("/Script/Engine.Engine"), TEXT("+ActiveClassRedirects"), *ClassRedirectString, /*InShouldReplaceExistingValue=*/false);
	}

	// Fix all specified config values
	for ( auto ConfigIt = ConfigValuesToSet.CreateConstIterator(); ConfigIt; ++ConfigIt )
	{
		const FConfigValue& ConfigValue = *ConfigIt;
		const FString IniFilename = DestFolder / TEXT("Config") / ConfigValue.ConfigFile;
		bool bSuccessfullyProcessed = false;

		TArray<FString> FileLines;
		if ( FFileHelper::LoadANSITextFileToStrings(*IniFilename, &IFileManager::Get(), FileLines) )
		{
			FString FileOutput;
			const FString TargetSection = ConfigValue.ConfigSection;
			FString CurSection;
			bool bFoundTargetKey = false;
			for ( auto LineIt = FileLines.CreateConstIterator(); LineIt; ++LineIt )
			{
				FString Line = *LineIt;
				Line.Trim().TrimTrailing();

				bool bShouldExcludeLineFromOutput = false;

				// If we not yet found the target key parse each line looking for it
				if ( !bFoundTargetKey )
				{
					// Check for an empty line. No work needs to be done on these lines
					if ( Line.Len() == 0 )
					{

					}
					// Comment lines start with ";". Skip these lines entirely.
					else if ( Line.StartsWith(TEXT(";")) )
					{
						
					}
					// If this is a section line, update the section
					else if ( Line.StartsWith(TEXT("[")) )
					{
						// If we are entering a new section and we have not yet found our key in the target section, add it to the end of the section
						if ( CurSection == TargetSection )
						{
							FileOutput += ConfigValue.ConfigKey + TEXT("=") + ConfigValue.ConfigValue + LINE_TERMINATOR + LINE_TERMINATOR;
							bFoundTargetKey = true;
						}

						// Update the current section
						CurSection = Line.Mid(1, Line.Len() - 2);
					}
					// This is possibly an actual key/value pair
					else if ( CurSection == TargetSection )
					{
						// Key value pairs contain an equals sign
						const int32 EqualsIdx = Line.Find(TEXT("="));
						if ( EqualsIdx != INDEX_NONE )
						{
							// Determine the key and see if it is the target key
							const FString Key = Line.Left(EqualsIdx);
							if ( Key == ConfigValue.ConfigKey )
							{
								// Found the target key, add it to the output and skip the current line if the target value is supposed to replace
								FileOutput += ConfigValue.ConfigKey + TEXT("=") + ConfigValue.ConfigValue + LINE_TERMINATOR;
								bShouldExcludeLineFromOutput = ConfigValue.bShouldReplaceExistingValue;
								bFoundTargetKey = true;
							}
						}
					}
				}

				// Unless we replaced the key, add this line to the output
				if ( !bShouldExcludeLineFromOutput )
				{
					FileOutput += Line;
					if ( LineIt.GetIndex() < FileLines.Num() - 1 )
					{
						// Add a line terminator on every line except the last
						FileOutput += LINE_TERMINATOR;
					}
				}
			}

			// If the key did not exist, add it here
			if ( !bFoundTargetKey )
			{
				// If we did not end in the correct section, add the section to the bottom of the file
				if ( CurSection != TargetSection )
				{
					FileOutput += LINE_TERMINATOR;
					FileOutput += LINE_TERMINATOR;
					FileOutput += FString::Printf(TEXT("[%s]"), *TargetSection) + LINE_TERMINATOR;
				}

				// Add the key/value here
				FileOutput += ConfigValue.ConfigKey + TEXT("=") + ConfigValue.ConfigValue + LINE_TERMINATOR;
			}

			if ( FFileHelper::SaveStringToFile(FileOutput, *IniFilename) )
			{
				bSuccessfullyProcessed = true;
			}
		}

		if ( !bSuccessfullyProcessed )
		{
			OutFailReason = LOCTEXT("FailedToFixUpDefaultEngine", "Failed to process file DefaultEngine.ini");
			DeleteCreatedFiles(DestFolder, CreatedFiles);
			return false;
		}
	}

	// Generate the project file
	{
		FText LocalFailReason;
		if ( IProjectManager::Get().DuplicateProjectFile(TemplateFile, NewProjectFile, LocalFailReason) )
		{
			CreatedFiles.Add(NewProjectFile);
		}
		else
		{
			OutFailReason = LocalFailReason;
			DeleteCreatedFiles(DestFolder, CreatedFiles);
			return false;
		}
	}

	if ( bShouldGenerateCode )
	{
		// resource folder
		const FString GameModuleSourcePath = DestFolder / TEXT("Source") / ProjectName;
		if (GenerateGameResourceFiles(GameModuleSourcePath, ProjectName, CreatedFiles, OutFailReason) == false)
		{
			DeleteCreatedFiles(DestFolder, CreatedFiles);
			return false;
		}

		// Generate project files
		if ( !GenerateCodeProjectFiles(NewProjectFile, OutFailReason) )
		{
			DeleteGeneratedProjectFiles(NewProjectFile);
			DeleteCreatedFiles(DestFolder, CreatedFiles);
			return false;
		}
	}

	if (bCopyStarterContent)
	{
		// Copy the starter content
		if ( !CopyStarterContent(DestFolder, OutFailReason) )
		{
			DeleteGeneratedProjectFiles(NewProjectFile);
			DeleteCreatedFiles(DestFolder, CreatedFiles);
			return false;
		}
	}

	return true;
}

FString GameProjectUtils::GetTemplateDefsFilename()
{
	return TEXT("TemplateDefs.ini");
}

bool GameProjectUtils::NameContainsOnlyLegalCharacters(const FString& TestName, FString& OutIllegalCharacters)
{
	bool bContainsIllegalCharacters = false;

	// Only allow alphanumeric characters in the project name
	bool bFoundAlphaNumericChar = false;
	for ( int32 CharIdx = 0 ; CharIdx < TestName.Len() ; ++CharIdx )
	{
		const FString& Char = TestName.Mid( CharIdx, 1 );
		if ( !FChar::IsAlnum(Char[0]) && Char != TEXT("_") )
		{
			if ( !OutIllegalCharacters.Contains( Char ) )
			{
				OutIllegalCharacters += Char;
			}

			bContainsIllegalCharacters = true;
		}
	}

	return !bContainsIllegalCharacters;
}

bool GameProjectUtils::ProjectPathContainsOnlyLegalCharacters(const FString& ProjectFilePath, FString& OutIllegalCharacters)
{
	bool bContainsIllegalCharacters = false;

	// set of characters not allowed in paths (Windows limitations)
	FString AllIllegalCharacters = TEXT("<>\"|?*");

	// Add lower-32 characters (illegal on windows)
	for ( TCHAR IllegalChar = 0; IllegalChar < 32; IllegalChar++ )
	{
		AllIllegalCharacters += IllegalChar;
	}

	// @ characters are not legal. Used for revision/label specifiers in P4/SVN.
	AllIllegalCharacters += TEXT("@");					
	// # characters are not legal. Used for revision specifiers in P4/SVN.
	AllIllegalCharacters += TEXT("#");					
	// ^ characters are not legal. While the filesystem wont complain about this character, visual studio will.
	AllIllegalCharacters += TEXT("^");					

#if PLATFORM_MAC
	// : characters are illegal on Mac OSX (but we need to allow them on Windows!)
	AllIllegalCharacters += TEXT(":");
#endif

	for ( int32 CharIdx = 0; CharIdx < AllIllegalCharacters.Len() ; ++CharIdx )
	{
		const FString& Char = AllIllegalCharacters.Mid( CharIdx, 1 );

		if ( ProjectFilePath.Contains( Char ) )
		{
			if ( !OutIllegalCharacters.Contains( Char ) )
			{
				OutIllegalCharacters += Char;
			}

			bContainsIllegalCharacters = true;
		}
	}

	return !bContainsIllegalCharacters;
}

bool GameProjectUtils::ProjectFileExists(const FString& ProjectFile)
{
	return FPlatformFileManager::Get().GetPlatformFile().FileExists(*ProjectFile);
}

bool GameProjectUtils::AnyProjectFilesExistInFolder(const FString& Path)
{
	TArray<FString> ExistingFiles;
	const FString Wildcard = FString::Printf(TEXT("%s/*.%s"), *Path, *IProjectManager::GetProjectFileExtension());
	IFileManager::Get().FindFiles(ExistingFiles, *Wildcard, /*Files=*/true, /*Directories=*/false);

	return ExistingFiles.Num() > 0;
}

bool GameProjectUtils::CleanupIsEnabled()
{
	// Clean up files when running Rocket (unless otherwise specified on the command line)
	return FParse::Param(FCommandLine::Get(), TEXT("norocketcleanup")) == false;
}

void GameProjectUtils::DeleteCreatedFiles(const FString& RootFolder, const TArray<FString>& CreatedFiles)
{
	if (CleanupIsEnabled())
	{
		for ( auto FileToDeleteIt = CreatedFiles.CreateConstIterator(); FileToDeleteIt; ++FileToDeleteIt )
		{
			IFileManager::Get().Delete(**FileToDeleteIt);
		}

		// If the project folder is empty after deleting all the files we created, delete the directory as well
		TArray<FString> RemainingFiles;
		IFileManager::Get().FindFilesRecursive(RemainingFiles, *RootFolder, TEXT("*.*"), /*Files=*/true, /*Directories=*/false);
		if ( RemainingFiles.Num() == 0 )
		{
			IFileManager::Get().DeleteDirectory(*RootFolder, /*RequireExists=*/false, /*Tree=*/true);
		}
	}
}

void GameProjectUtils::DeleteGeneratedProjectFiles(const FString& NewProjectFile)
{
	if (CleanupIsEnabled())
	{
		const FString NewProjectFolder = FPaths::GetPath(NewProjectFile);
		const FString NewProjectName = FPaths::GetBaseFilename(NewProjectFile);

		// Since it is hard to tell which files were created from the code project file generation process, just delete the entire ProjectFiles folder.
		const FString IntermediateProjectFileFolder = NewProjectFolder / TEXT("Intermediate") / TEXT("ProjectFiles");
		IFileManager::Get().DeleteDirectory(*IntermediateProjectFileFolder, /*RequireExists=*/false, /*Tree=*/true);

		// Delete the solution file
		const FString SolutionFileName = NewProjectFolder / NewProjectName + TEXT(".sln");
		IFileManager::Get().Delete( *SolutionFileName );
	}
}

void GameProjectUtils::DeleteGeneratedBuildFiles(const FString& NewProjectFolder)
{
	if (CleanupIsEnabled())
	{
		// Since it is hard to tell which files were created from the build process, just delete the entire Binaries and Build folders.
		const FString BinariesFolder = NewProjectFolder / TEXT("Binaries");
		const FString BuildFolder    = NewProjectFolder / TEXT("Intermediate") / TEXT("Build");
		IFileManager::Get().DeleteDirectory(*BinariesFolder, /*RequireExists=*/false, /*Tree=*/true);
		IFileManager::Get().DeleteDirectory(*BuildFolder, /*RequireExists=*/false, /*Tree=*/true);
	}
}

bool GameProjectUtils::GenerateConfigFiles(const FString& NewProjectPath, const FString& NewProjectName, bool bShouldGenerateCode, bool bCopyStarterContent, TArray<FString>& OutCreatedFiles, FText& OutFailReason)
{
	FString ProjectConfigPath = NewProjectPath / TEXT("Config");

	// DefaultEngine.ini
	{
		const FString DefaultEngineIniFilename = ProjectConfigPath / TEXT("DefaultEngine.ini");
		FString FileContents;

		FileContents += TEXT("[URL]") LINE_TERMINATOR;
		FileContents += FString::Printf(TEXT("GameName=%s") LINE_TERMINATOR, *NewProjectName);
		FileContents += LINE_TERMINATOR;

		if (bCopyStarterContent)
		{
			// for generated/blank projects with starter content, set startup map to be the starter content map
			// otherwise, we leave it to be what the template wants.
			TArray<FString> StarterContentMapFiles;
			const FString FileWildcard = FString(TEXT("*")) + FPackageName::GetMapPackageExtension();
		
			// assume the first map in the /Maps folder is the default map
			IFileManager::Get().FindFilesRecursive(StarterContentMapFiles, *FPaths::StarterContentDir(), *FileWildcard, /*Files=*/true, /*Directories=*/false);
			if (StarterContentMapFiles.Num() > 0)
			{
				FString StarterContentContentDir = FPaths::StarterContentDir() + TEXT("Content/");

				const FString BaseMapFilename = FPaths::GetBaseFilename(StarterContentMapFiles[0]);

				FString MapPathRelToContent = FPaths::GetPath(StarterContentMapFiles[0]);
				FPaths::MakePathRelativeTo(MapPathRelToContent, *StarterContentContentDir);

				const FString MapPackagePath = FString(TEXT("/Game/")) + MapPathRelToContent + TEXT("/") + BaseMapFilename;
				FileContents += TEXT("[/Script/EngineSettings.GameMapsSettings]") LINE_TERMINATOR;
				FileContents += FString::Printf(TEXT("EditorStartupMap=%s") LINE_TERMINATOR, *MapPackagePath);
				FileContents += FString::Printf(TEXT("GameDefaultMap=%s") LINE_TERMINATOR, *MapPackagePath);
			}
		}

		if (WriteOutputFile(DefaultEngineIniFilename, FileContents, OutFailReason))
		{
			OutCreatedFiles.Add(DefaultEngineIniFilename);
		}
		else
		{
			return false;
		}
	}

	// DefaultGame.ini
	{
		const FString DefaultGameIniFilename = ProjectConfigPath / TEXT("DefaultGame.ini");
		FString FileContents;
		FileContents += TEXT("[/Script/EngineSettings.GeneralProjectSettings]") LINE_TERMINATOR;
		FileContents += FString::Printf( TEXT("ProjectID=%s") LINE_TERMINATOR, *FGuid::NewGuid().ToString() );
		FileContents += LINE_TERMINATOR;

		if ( bShouldGenerateCode )
		{
			FileContents += TEXT("[/Script/Engine.WorldSettings]") LINE_TERMINATOR;
			FileContents += FString::Printf(TEXT("GlobalDefaultGameMode=\"/Script/%s.%sGameMode\"") LINE_TERMINATOR, *NewProjectName, *NewProjectName);
			FileContents += FString::Printf(TEXT("GlobalDefaultServerGameMode=\"/Script/%s.%sGameMode\"") LINE_TERMINATOR, *NewProjectName, *NewProjectName);
			FileContents += LINE_TERMINATOR;
		}

		if ( WriteOutputFile(DefaultGameIniFilename, FileContents, OutFailReason) )
		{
			OutCreatedFiles.Add(DefaultGameIniFilename);
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool GameProjectUtils::GenerateBasicSourceCode(const FString& NewProjectSourcePath, const FString& NewProjectName, TArray<FString>& OutGeneratedStartupModuleNames, TArray<FString>& OutCreatedFiles, FText& OutFailReason)
{
	const FString GameModulePath = NewProjectSourcePath / NewProjectName;
	const FString EditorName = NewProjectName + TEXT("Editor");

	// MyGame.Build.cs
	{
		const FString NewBuildFilename = GameModulePath / NewProjectName + TEXT(".Build.cs");
		TArray<FString> PublicDependencyModuleNames;
		PublicDependencyModuleNames.Add(TEXT("Core"));
		PublicDependencyModuleNames.Add(TEXT("CoreUObject"));
		PublicDependencyModuleNames.Add(TEXT("Engine"));
		PublicDependencyModuleNames.Add(TEXT("InputCore"));
		TArray<FString> PrivateDependencyModuleNames;
		if ( GenerateGameModuleBuildFile(NewBuildFilename, NewProjectName, PublicDependencyModuleNames, PrivateDependencyModuleNames, OutFailReason) )
		{
			OutGeneratedStartupModuleNames.Add(NewProjectName);
			OutCreatedFiles.Add(NewBuildFilename);
		}
		else
		{
			return false;
		}
	}

	// MyGame resource folder
	if (GenerateGameResourceFiles(GameModulePath, NewProjectName, OutCreatedFiles, OutFailReason) == false)
	{
		return false;
	}

	// MyGame.Target.cs
	{
		const FString NewTargetFilename = NewProjectSourcePath / NewProjectName + TEXT(".Target.cs");
		TArray<FString> ExtraModuleNames;
		ExtraModuleNames.Add( NewProjectName );
		if ( GenerateGameModuleTargetFile(NewTargetFilename, NewProjectName, ExtraModuleNames, OutFailReason) )
		{
			OutCreatedFiles.Add(NewTargetFilename);
		}
		else
		{
			return false;
		}
	}

	// MyGameEditor.Target.cs
	{
		const FString NewTargetFilename = NewProjectSourcePath / EditorName + TEXT(".Target.cs");
		// Include the MyGame module...
		TArray<FString> ExtraModuleNames;
		ExtraModuleNames.Add(NewProjectName);
		if ( GenerateEditorModuleTargetFile(NewTargetFilename, EditorName, ExtraModuleNames, OutFailReason) )
		{
			OutCreatedFiles.Add(NewTargetFilename);
		}
		else
		{
			return false;
		}
	}

	// MyGame.h
	{
		const FString NewHeaderFilename = GameModulePath / NewProjectName + TEXT(".h");
		TArray<FString> PublicHeaderIncludes;
		PublicHeaderIncludes.Add(TEXT("Engine.h"));
		if ( GenerateGameModuleHeaderFile(NewHeaderFilename, PublicHeaderIncludes, OutFailReason) )
		{
			OutCreatedFiles.Add(NewHeaderFilename);
		}
		else
		{
			return false;
		}
	}

	// MyGame.cpp
	{
		const FString NewCPPFilename = GameModulePath / NewProjectName + TEXT(".cpp");
		if ( GenerateGameModuleCPPFile(NewCPPFilename, NewProjectName, NewProjectName, OutFailReason) )
		{
			OutCreatedFiles.Add(NewCPPFilename);
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool GameProjectUtils::GenerateGameFrameworkSourceCode(const FString& NewProjectSourcePath, const FString& NewProjectName, TArray<FString>& OutCreatedFiles, FText& OutFailReason)
{
	const FString GameModulePath = NewProjectSourcePath / NewProjectName;

	// MyGamePlayerController.h
	{
		const UClass* BaseClass = APlayerController::StaticClass();
		const FString NewHeaderFilename = GameModulePath / NewProjectName + BaseClass->GetName() + TEXT(".h");
		FString UnusedSyncLocation;
		if ( GenerateClassHeaderFile(NewHeaderFilename, BaseClass, TArray<FString>(), TEXT(""), TEXT(""), UnusedSyncLocation, OutFailReason) )
		{
			OutCreatedFiles.Add(NewHeaderFilename);
		}
		else
		{
			return false;
		}
	}

	// MyGameGameMode.h
	{
		const UClass* BaseClass = AGameMode::StaticClass();
		const FString NewHeaderFilename = GameModulePath / NewProjectName + BaseClass->GetName() + TEXT(".h");
		FString UnusedSyncLocation;
		if ( GenerateClassHeaderFile(NewHeaderFilename, BaseClass, TArray<FString>(), TEXT(""), TEXT(""), UnusedSyncLocation, OutFailReason) )
		{
			OutCreatedFiles.Add(NewHeaderFilename);
		}
		else
		{
			return false;
		}
	}

	// MyGamePlayerController.cpp
	FString PrefixedPlayerControllerClassName;
	{
		const UClass* BaseClass = APlayerController::StaticClass();
		const FString NewCPPFilename = GameModulePath / NewProjectName + BaseClass->GetName() + TEXT(".cpp");
		PrefixedPlayerControllerClassName = FString(BaseClass->GetPrefixCPP()) + NewProjectName + BaseClass->GetName();
		if ( GenerateClassCPPFile(NewCPPFilename, NewProjectName, PrefixedPlayerControllerClassName, TArray<FString>(), TArray<FString>(), TEXT(""), OutFailReason) )
		{
			OutCreatedFiles.Add(NewCPPFilename);
		}
		else
		{
			return false;
		}
	}

	// MyGameGameMode.cpp
	{
		const UClass* BaseClass = AGameMode::StaticClass();
		const FString NewCPPFilename = GameModulePath / NewProjectName + BaseClass->GetName() + TEXT(".cpp");
		const FString PrefixedClassName = FString(BaseClass->GetPrefixCPP()) + NewProjectName + BaseClass->GetName();
		
		TArray<FString> PropertyOverrides;
		PropertyOverrides.Add( FString::Printf( TEXT("PlayerControllerClass = %s::StaticClass();"), *PrefixedPlayerControllerClassName ) );

		// PropertyOverrides references PlayerController class so we need to include its header to properly compile under non-unity
		const UClass* PlayerControllerBaseClass = APlayerController::StaticClass();
		const FString PlayerControllerClassName = NewProjectName + PlayerControllerBaseClass->GetName() + TEXT(".h");
		TArray<FString> AdditionalIncludes;
		AdditionalIncludes.Add(PlayerControllerClassName);

		if ( GenerateClassCPPFile(NewCPPFilename, NewProjectName, PrefixedClassName, AdditionalIncludes, PropertyOverrides, TEXT(""), OutFailReason) )
		{
			OutCreatedFiles.Add(NewCPPFilename);
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool GameProjectUtils::GenerateCodeProjectFiles(const FString& ProjectFilename, FText& OutFailReason)
{
	if ( FModuleManager::Get().GenerateCodeProjectFiles(ProjectFilename, *GLog) )
	{
		return true;
	}

	FFormatNamedArguments Args;
	Args.Add( TEXT("ProjectFilename"), FText::FromString( ProjectFilename ) );
	OutFailReason = FText::Format( LOCTEXT("FailedToGenerateCodeProjectFiles", "Failed to generate code project files for \"{ProjectFilename}\"."), Args );
	return false;
}

bool GameProjectUtils::IsStarterContentAvailableForNewProjects()
{
	TArray<FString> StarterContentFiles;
	GetStarterContentFiles(StarterContentFiles);

	return (StarterContentFiles.Num() > 0);
}

bool GameProjectUtils::ReadTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason)
{
	const FString FullFileName = FPaths::EngineContentDir() / TEXT("Editor") / TEXT("Templates") / TemplateFileName;
	if ( FFileHelper::LoadFileToString(OutFileContents, *FullFileName) )
	{
		return true;
	}

	FFormatNamedArguments Args;
	Args.Add( TEXT("FullFileName"), FText::FromString( FullFileName ) );
	OutFailReason = FText::Format( LOCTEXT("FailedToReadTemplateFile", "Failed to read template file \"{FullFileName}\""), Args );
	return false;
}

bool GameProjectUtils::WriteOutputFile(const FString& OutputFilename, const FString& OutputFileContents, FText& OutFailReason)
{
	if ( FFileHelper::SaveStringToFile(OutputFileContents, *OutputFilename ) )
	{
		return true;
	}

	FFormatNamedArguments Args;
	Args.Add( TEXT("OutputFilename"), FText::FromString( OutputFilename ) );
	OutFailReason = FText::Format( LOCTEXT("FailedToWriteOutputFile", "Failed to write output file \"{OutputFilename}\". Perhaps the file is Read-Only?"), Args );
	return false;
}

FString GameProjectUtils::MakeCopyrightLine()
{
	return FString(TEXT("// ")) + Cast<UGeneralProjectSettings>(UGeneralProjectSettings::StaticClass()->GetDefaultObject())->CopyrightNotice;
}

FString GameProjectUtils::MakeCommaDelimitedList(const TArray<FString>& InList, bool bPlaceQuotesAroundEveryElement)
{
	FString ReturnString;

	for ( auto ListIt = InList.CreateConstIterator(); ListIt; ++ListIt )
	{
		FString ElementStr;
		if ( bPlaceQuotesAroundEveryElement )
		{
			ElementStr = FString::Printf( TEXT("\"%s\""), **ListIt);
		}
		else
		{
			ElementStr = *ListIt;
		}

		if ( ReturnString.Len() > 0 )
		{
			// If this is not the first item in the list, prepend with a comma
			ElementStr = FString::Printf(TEXT(", %s"), *ElementStr);
		}

		ReturnString += ElementStr;
	}

	return ReturnString;
}

FString GameProjectUtils::MakeIncludeList(const TArray<FString>& InList)
{
	FString ReturnString;

	for ( auto ListIt = InList.CreateConstIterator(); ListIt; ++ListIt )
	{
		ReturnString += FString::Printf( TEXT("#include \"%s\"") LINE_TERMINATOR, **ListIt);
	}

	return ReturnString;
}

bool GameProjectUtils::GenerateClassHeaderFile(const FString& NewHeaderFileName, const UClass* BaseClass, const TArray<FString>& ClassSpecifierList, const FString& ClassProperties, const FString& ClassFunctionDeclarations, FString& OutSyncLocation, FText& OutFailReason)
{
	FString Template;
	if ( !ReadTemplateFile(TEXT("UObjectClass.h.template"), Template, OutFailReason) )
	{
		return false;
	}

	const FString UnPrefixedClassName = FPaths::GetBaseFilename(NewHeaderFileName);
	const FString ClassPrefix = BaseClass->GetPrefixCPP();
	const FString PrefixedClassName = ClassPrefix + UnPrefixedClassName;
	const FString PrefixedBaseClassName = ClassPrefix + BaseClass->GetName();

	FString BaseClassIncludeDirective;
	if(BaseClass->HasMetaData(TEXT("IncludePath")))
	{
		BaseClassIncludeDirective = FString::Printf(LINE_TERMINATOR TEXT("#include \"%s\""), *BaseClass->GetMetaData(TEXT("IncludePath")));
	}

	const FString UnprefixedClassName = PrefixedClassName.Mid(1);
	FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *MakeCopyrightLine(), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%UNPREFIXED_CLASS_NAME%"), *UnprefixedClassName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%UCLASS_SPECIFIER_LIST%"), *MakeCommaDelimitedList(ClassSpecifierList, false), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PREFIXED_CLASS_NAME%"), *PrefixedClassName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PREFIXED_BASE_CLASS_NAME%"), *PrefixedBaseClassName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%CLASS_PROPERTIES%"), *ClassProperties, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%CLASS_FUNCTION_DECLARATIONS%"), *ClassFunctionDeclarations, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%BASE_CLASS_INCLUDE_DIRECTIVE%"), *BaseClassIncludeDirective, ESearchCase::CaseSensitive);

	// Determine the cursor focus location if this file will by synced after creation
	TArray<FString> Lines;
	FinalOutput.ParseIntoArray(&Lines, TEXT("\n"), false);
	for ( int32 LineIdx = 0; LineIdx < Lines.Num(); ++LineIdx )
	{
		const FString& Line = Lines[LineIdx];
		int32 CharLoc = Line.Find( TEXT("%CURSORFOCUSLOCATION%") );
		if ( CharLoc != INDEX_NONE )
		{
			// Found the sync marker
			OutSyncLocation = FString::Printf( TEXT("%d:%d"), LineIdx + 1, CharLoc + 1 );
			break;
		}
	}

	// If we did not find the sync location, just sync to the top of the file
	if ( OutSyncLocation.IsEmpty() )
	{
		OutSyncLocation = TEXT("1:1");
	}

	// Now remove the cursor focus marker
	FinalOutput = FinalOutput.Replace(TEXT("%CURSORFOCUSLOCATION%"), TEXT(""), ESearchCase::CaseSensitive);

	return WriteOutputFile(NewHeaderFileName, FinalOutput, OutFailReason);
}

bool GameProjectUtils::GenerateClassCPPFile(const FString& NewCPPFileName, const FString& ModuleName, const FString& PrefixedClassName, const TArray<FString>& AdditionalIncludes, const TArray<FString>& PropertyOverrides, const FString& AdditionalMemberDefinitions, FText& OutFailReason)
{
	FString Template;
	if ( !ReadTemplateFile(TEXT("UObjectClass.cpp.template"), Template, OutFailReason) )
	{
		return false;
	}

	FString AdditionalIncludesStr;
	for (int32 IncludeIdx = 0; IncludeIdx < AdditionalIncludes.Num(); ++IncludeIdx)
	{
		if (IncludeIdx > 0)
		{
			AdditionalIncludesStr += LINE_TERMINATOR;
		}

		AdditionalIncludesStr += FString::Printf(TEXT("#include \"%s\""), *AdditionalIncludes[IncludeIdx]);
	}

	FString PropertyOverridesStr;
	for ( int32 OverrideIdx = 0; OverrideIdx < PropertyOverrides.Num(); ++OverrideIdx )
	{
		if ( OverrideIdx > 0 )
		{
			PropertyOverridesStr += LINE_TERMINATOR;
		}

		PropertyOverridesStr += TEXT("\t");
		PropertyOverridesStr += *PropertyOverrides[OverrideIdx];
	}

	const FString UnprefixedClassName = PrefixedClassName.Mid(1);
	FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *MakeCopyrightLine(), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%UNPREFIXED_CLASS_NAME%"), *UnprefixedClassName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PREFIXED_CLASS_NAME%"), *PrefixedClassName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PROPERTY_OVERRIDES%"), *PropertyOverridesStr, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%ADDITIONAL_MEMBER_DEFINITIONS%"), *AdditionalMemberDefinitions, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%ADDITIONAL_INCLUDE_DIRECTIVES%"), *AdditionalIncludesStr, ESearchCase::CaseSensitive);

	return WriteOutputFile(NewCPPFileName, FinalOutput, OutFailReason);
}

bool GameProjectUtils::GenerateGameModuleBuildFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& PublicDependencyModuleNames, const TArray<FString>& PrivateDependencyModuleNames, FText& OutFailReason)
{
	FString Template;
	if ( !ReadTemplateFile(TEXT("GameModule.Build.cs.template"), Template, OutFailReason) )
	{
		return false;
	}

	FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *MakeCopyrightLine(), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PUBLIC_DEPENDENCY_MODULE_NAMES%"), *MakeCommaDelimitedList(PublicDependencyModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PRIVATE_DEPENDENCY_MODULE_NAMES%"), *MakeCommaDelimitedList(PrivateDependencyModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);

	return WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

bool GameProjectUtils::GenerateGameModuleTargetFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& ExtraModuleNames, FText& OutFailReason)
{
	FString Template;
	if ( !ReadTemplateFile(TEXT("Stub.Target.cs.template"), Template, OutFailReason) )
	{
		return false;
	}

	FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *MakeCopyrightLine(), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%EXTRA_MODULE_NAMES%"), *MakeCommaDelimitedList(ExtraModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%TARGET_TYPE%"), TEXT("Game"), ESearchCase::CaseSensitive);

	return WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

bool GameProjectUtils::GenerateGameResourceFile(const FString& NewResourceFolderName, const FString& TemplateFilename, const FString& GameName, TArray<FString>& OutCreatedFiles, FText& OutFailReason)
{
	FString Template;
	if (!ReadTemplateFile(TemplateFilename, Template, OutFailReason))
	{
		return false;
	}

	FString FinalOutput = Template.Replace(TEXT("%GAME_NAME%"), *GameName, ESearchCase::CaseSensitive);
	
	FString OutputFilename = TemplateFilename.Replace(TEXT("_GAME_NAME_"), *GameName);
	FString FullOutputFilename = NewResourceFolderName / OutputFilename;
	if (WriteOutputFile(FullOutputFilename, FinalOutput, OutFailReason))
	{
		OutCreatedFiles.Add(FullOutputFilename);
		return true;
	}

	return false;
}

bool GameProjectUtils::GenerateGameResourceFiles(const FString& NewResourceFolderName, const FString& GameName, TArray<FString>& OutCreatedFiles, FText& OutFailReason)
{
	bool bSucceeded = true;
	FString TemplateFilename;

#if PLATFORM_WINDOWS
	FString IconPartialName = TEXT("_GAME_NAME_");

	// Icon (just copy this)
	TemplateFilename = FString::Printf(TEXT("Resources/Windows/%s.ico"), *IconPartialName);
	FString FullTemplateFilename = FPaths::EngineContentDir() / TEXT("Editor") / TEXT("Templates") / TemplateFilename;
	FString OutputFilename = TemplateFilename.Replace(*IconPartialName, *GameName);
	FString FullOutputFilename = NewResourceFolderName / OutputFilename;
	bSucceeded &= (IFileManager::Get().Copy(*FullOutputFilename, *FullTemplateFilename) == COPY_OK);
	if (bSucceeded)
	{
		OutCreatedFiles.Add(FullOutputFilename);
	}

	// RC
	TemplateFilename = TEXT("Resources/Windows/_GAME_NAME_.rc");
	bSucceeded &= GenerateGameResourceFile(NewResourceFolderName, TemplateFilename, GameName, OutCreatedFiles, OutFailReason);
#elif PLATFORM_MAC
	//@todo MAC: Implement MAC version of these files...
#endif

	return bSucceeded;
}

bool GameProjectUtils::GenerateEditorModuleBuildFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& PublicDependencyModuleNames, const TArray<FString>& PrivateDependencyModuleNames, FText& OutFailReason)
{
	FString Template;
	if ( !ReadTemplateFile(TEXT("EditorModule.Build.cs.template"), Template, OutFailReason) )
	{
		return false;
	}

	FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *MakeCopyrightLine(), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PUBLIC_DEPENDENCY_MODULE_NAMES%"), *MakeCommaDelimitedList(PublicDependencyModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PRIVATE_DEPENDENCY_MODULE_NAMES%"), *MakeCommaDelimitedList(PrivateDependencyModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);

	return WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

bool GameProjectUtils::GenerateEditorModuleTargetFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& ExtraModuleNames, FText& OutFailReason)
{
	FString Template;
	if ( !ReadTemplateFile(TEXT("Stub.Target.cs.template"), Template, OutFailReason) )
	{
		return false;
	}

	FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *MakeCopyrightLine(), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%EXTRA_MODULE_NAMES%"), *MakeCommaDelimitedList(ExtraModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%TARGET_TYPE%"), TEXT("Editor"), ESearchCase::CaseSensitive);

	return WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

bool GameProjectUtils::GenerateGameModuleCPPFile(const FString& NewBuildFileName, const FString& ModuleName, const FString& GameName, FText& OutFailReason)
{
	FString Template;
	if ( !ReadTemplateFile(TEXT("GameModule.cpp.template"), Template, OutFailReason) )
	{
		return false;
	}

	FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *MakeCopyrightLine(), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%GAME_NAME%"), *GameName, ESearchCase::CaseSensitive);

	return WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

bool GameProjectUtils::GenerateGameModuleHeaderFile(const FString& NewBuildFileName, const TArray<FString>& PublicHeaderIncludes, FText& OutFailReason)
{
	FString Template;
	if ( !ReadTemplateFile(TEXT("GameModule.h.template"), Template, OutFailReason) )
	{
		return false;
	}

	FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *MakeCopyrightLine(), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PUBLIC_HEADER_INCLUDES%"), *MakeIncludeList(PublicHeaderIncludes), ESearchCase::CaseSensitive);

	return WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

void GameProjectUtils::OnUpdateProjectConfirm()
{
	UpdateProject(NULL);
}

void GameProjectUtils::UpdateProject(const TArray<FString>* StartupModuleNames)
{
	const FString& ProjectFilename = FPaths::GetProjectFilePath();
	const FString& ShortFilename = FPaths::GetCleanFilename(ProjectFilename);
	FText FailReason;
	FText UpdateMessage;
	SNotificationItem::ECompletionState NewCompletionState;
	bool bWasCheckedOut = false;
	if ( UpdateGameProjectFile(ProjectFilename, StartupModuleNames, bWasCheckedOut, FailReason) )
	{
		// The project was updated successfully.
		FFormatNamedArguments Args;
		Args.Add( TEXT("ShortFilename"), FText::FromString( ShortFilename ) );
		UpdateMessage = FText::Format( LOCTEXT("ProjectFileUpdateComplete", "{ShortFilename} was successfully updated."), Args );
		if ( bWasCheckedOut )
		{
			UpdateMessage = FText::Format( LOCTEXT("ProjectFileUpdateCheckin", "{ShortFilename} was successfully updated. Please check this file into source control."), Args );
		}
		NewCompletionState = SNotificationItem::CS_Success;
	}
	else
	{
		// The user chose to update, but the update failed. Notify the user.
		FFormatNamedArguments Args;
		Args.Add( TEXT("ShortFilename"), FText::FromString( ShortFilename ) );
		Args.Add( TEXT("FailReason"), FailReason );
		UpdateMessage = FText::Format( LOCTEXT("ProjectFileUpdateFailed", "{ShortFilename} failed to update. {FailReason}"), Args );
		NewCompletionState = SNotificationItem::CS_Fail;
	}

	if ( UpdateGameProjectNotification.IsValid() )
	{
		UpdateGameProjectNotification.Pin()->SetCompletionState(NewCompletionState);
		UpdateGameProjectNotification.Pin()->SetText(UpdateMessage);
		UpdateGameProjectNotification.Pin()->ExpireAndFadeout();
		UpdateGameProjectNotification.Reset();
	}
}

void GameProjectUtils::OnUpdateProjectCancel()
{
	if ( UpdateGameProjectNotification.IsValid() )
	{
		UpdateGameProjectNotification.Pin()->SetCompletionState(SNotificationItem::CS_None);
		UpdateGameProjectNotification.Pin()->ExpireAndFadeout();
		UpdateGameProjectNotification.Reset();
	}
}

bool GameProjectUtils::UpdateGameProjectFile(const FString& ProjectFilename, const TArray<FString>* StartupModuleNames, bool& OutbWasCheckedOut, FText& OutFailReason)
{
	// First attempt to check out the file if SCC is enabled
	if ( ISourceControlModule::Get().IsEnabled() )
	{
		OutbWasCheckedOut = GameProjectUtils::CheckoutGameProjectFile(ProjectFilename, OutFailReason);
		if ( !OutbWasCheckedOut )
		{
			// Failed to check out the file
			return false;
		}
	}
	else
	{
		if(FPlatformFileManager::Get().GetPlatformFile().IsReadOnly(*ProjectFilename))
		{
			FText ShouldMakeProjectWriteable = LOCTEXT("ShouldMakeProjectWriteable_Message", "'{ProjectFilename}' is read-only and cannot be updated, would you like to make it writeable?");
			FFormatNamedArguments Arguments;
			Arguments.Add( TEXT("ProjectFilename"), FText::FromString(ProjectFilename));
			if ( FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(ShouldMakeProjectWriteable, Arguments)) == EAppReturnType::Yes )
			{
				FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*ProjectFilename, false);
			}
		}
		OutbWasCheckedOut = false;
	}

	// Now tell the project manager to update the file
	if ( !IProjectManager::Get().UpdateLoadedProjectFileToCurrent(StartupModuleNames, OutFailReason) )
	{
		return false;
	}

	return true;
}

bool GameProjectUtils::CheckoutGameProjectFile(const FString& ProjectFilename, FText& OutFailReason)
{
	if ( !ensure(ProjectFilename.Len()) )
	{
		OutFailReason = LOCTEXT("NoProjectFilename", "The project filename was not specified.");
		return false;
	}

	if ( !ISourceControlModule::Get().IsEnabled() )
	{
		OutFailReason = LOCTEXT("SCCDisabled", "Source control is not enabled. Enable source control in the preferences menu.");
		return false;
	}

	FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(ProjectFilename);
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
	TArray<FString> FilesToBeCheckedOut;
	FilesToBeCheckedOut.Add(AbsoluteFilename);

	bool bSuccessfullyCheckedOut = false;
	OutFailReason = LOCTEXT("SCCStateInvalid", "Could not determine source control state.");

	if(SourceControlState.IsValid())
	{
		if(SourceControlState->IsCheckedOut() || SourceControlState->IsAdded() || !SourceControlState->IsSourceControlled())
		{
			// Already checked out or opened for add... or not in the depot at all
			bSuccessfullyCheckedOut = true;
		}
		else if(SourceControlState->CanCheckout() || SourceControlState->IsCheckedOutOther())
		{
			bSuccessfullyCheckedOut = (SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), FilesToBeCheckedOut) == ECommandResult::Succeeded);
			if (!bSuccessfullyCheckedOut)
			{
				OutFailReason = LOCTEXT("SCCCheckoutFailed", "Failed to check out the project file.");
			}
		}
		else if(!SourceControlState->IsCurrent())
		{
			OutFailReason = LOCTEXT("SCCNotCurrent", "The project file is not at head revision.");
		}
	}

	return bSuccessfullyCheckedOut;
}

FString GameProjectUtils::GetDefaultProjectTemplateFilename()
{
	return TEXT("");
}

int32 GameProjectUtils::GetProjectCodeFileCount()
{
	TArray<FString> Filenames;
	IFileManager::Get().FindFilesRecursive(Filenames, *FPaths::GameSourceDir(), TEXT("*.h"), true, false, false);
	IFileManager::Get().FindFilesRecursive(Filenames, *FPaths::GameSourceDir(), TEXT("*.cpp"), true, false, false);

	return Filenames.Num();
}

bool GameProjectUtils::ProjectHasCodeFiles()
{
	return GameProjectUtils::GetProjectCodeFileCount() > 0;
}

bool GameProjectUtils::AddCodeToProject_Internal(const FString& NewClassName, const UClass* ParentClass, FString& OutHeaderFilePath, FString& OutCppFilePath, FText& OutFailReason)
{
	if ( !ParentClass )
	{
		OutFailReason = LOCTEXT("NoParentClass", "You must specify a parent class");
		return false;
	}

	if ( !IsValidClassNameForCreation(NewClassName, OutFailReason) )
	{
		return false;
	}

	if ( !FApp::HasGameName() )
	{
		OutFailReason = LOCTEXT("AddCodeToProject_NoGameName", "You can not add code because you have not loaded a projet.");
		return false;
	}

	const bool bAllowNewSlowTask = true;
	FStatusMessageContext SlowTaskMessage( LOCTEXT( "AddingCodeToProject", "Adding code to project..." ), bAllowNewSlowTask );

	const FString SourceDir = FPaths::GameSourceDir().LeftChop(1); // Trim the trailing /

	// Assuming the game name is the same as the primary game module name
	const FString ModuleName = FApp::GetGameName();
	const FString ModulePath = SourceDir / ModuleName;

	// If the project does not already contain code, add the primary game module
	TArray<FString> CreatedFiles;
	if ( !ProjectHasCodeFiles() )
	{
		TArray<FString> StartupModuleNames;
		if ( GenerateBasicSourceCode(SourceDir, ModuleName, StartupModuleNames, CreatedFiles, OutFailReason) )
		{
			UpdateProject(&StartupModuleNames);
		}
		else
		{
			DeleteCreatedFiles(SourceDir, CreatedFiles);
			return false;
		}
	}

	// Class Header File
	FString SyncLocation;
	const FString NewHeaderFilename = ModulePath / NewClassName + TEXT(".h");
	{
		if ( GenerateClassHeaderFile(NewHeaderFilename, ParentClass, TArray<FString>(), TEXT(""), TEXT(""), SyncLocation, OutFailReason) )
		{
			CreatedFiles.Add(NewHeaderFilename);
		}
		else
		{
			DeleteCreatedFiles(SourceDir, CreatedFiles);
			return false;
		}
	}

	// Class CPP file
	const FString NewCPPFilename = ModulePath / NewClassName + TEXT(".cpp");
	{
		const FString PrefixedClassName = FString(ParentClass->GetPrefixCPP()) + NewClassName;
		if ( GenerateClassCPPFile(NewCPPFilename, ModuleName, PrefixedClassName, TArray<FString>(), TArray<FString>(), TEXT(""), OutFailReason) )
		{
			CreatedFiles.Add(NewCPPFilename);
		}
		else
		{
			DeleteCreatedFiles(SourceDir, CreatedFiles);
			return false;
		}
	}

	// Generate project files if we happen to be using a project file.
	if ( !FModuleManager::Get().GenerateCodeProjectFiles( FPaths::GetProjectFilePath(), *GLog ) )
	{
		OutFailReason = LOCTEXT("FailedToGenerateProjectFiles", "Failed to generate project files.");
		return false;
	}

	// Mark the files for add in SCC
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( ISourceControlModule::Get().IsEnabled() && SourceControlProvider.IsAvailable() )
	{
		TArray<FString> FilesToCheckOut;
		for ( auto FileIt = CreatedFiles.CreateConstIterator(); FileIt; ++FileIt )
		{
			FilesToCheckOut.Add( IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(**FileIt) );
		}

		SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), FilesToCheckOut);
	}

	OutHeaderFilePath = NewHeaderFilename;
	OutCppFilePath = NewCPPFilename;

	return true;
}

#undef LOCTEXT_NAMESPACE
