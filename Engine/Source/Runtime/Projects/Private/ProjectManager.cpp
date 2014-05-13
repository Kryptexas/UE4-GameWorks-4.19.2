// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectsPrivatePCH.h"


DEFINE_LOG_CATEGORY_STATIC( LogProjectManager, Log, All );

#define LOCTEXT_NAMESPACE "ProjectManager"


FProject::FProject()
{
}

FProject::FProject( const FProjectInfo& InitProjectInfo )
	: ProjectInfo(InitProjectInfo)
{
}

bool FProject::IsSignedSampleProject(const FString& FilePath) const
{
	return ProjectInfo.EpicSampleNameHash == GetTypeHash(FPaths::GetCleanFilename(FilePath));
}

void FProject::SignSampleProject(const FString& FilePath, const FString& Category)
{
	ProjectInfo.EpicSampleNameHash = GetTypeHash(FPaths::GetCleanFilename(FilePath));
	ProjectInfo.Category = Category;
}

bool FProject::PerformAdditionalDeserialization(const TSharedRef< FJsonObject >& FileObject)
{
	ReadNumberFromJSON(FileObject, TEXT("EpicSampleNameHash"), ProjectInfo.EpicSampleNameHash);

	return true;
}

void FProject::PerformAdditionalSerialization(const TSharedRef< TJsonWriter<> >& Writer) const
{
	Writer->WriteValue(TEXT("EpicSampleNameHash"), FString::Printf(TEXT("%u"), ProjectInfo.EpicSampleNameHash));
}





FProjectManager::FProjectManager()
{

}

bool FProjectManager::LoadProjectFile( const FString& InProjectFile )
{
	FText FailureReason;
	TSharedRef<FProject> NewProject = MakeShareable( new FProject() );
	if ( NewProject->LoadFromFile(InProjectFile, FailureReason) )
	{
		// Load successful. Set the loaded project file pointer.
		CurrentlyLoadedProject = NewProject;

		return true;
	}
	
#if PLATFORM_IOS
    FString UpdatedMessage = FString::Printf(TEXT("%s\n%s"), *FailureReason.ToString(), TEXT("For troubleshooting, please go to https://docs.unrealengine.com/latest/INT/Platforms/iOS/GettingStarted/index.html"));
    FailureReason = FText::FromString(UpdatedMessage);
#endif
	UE_LOG(LogProjectManager, Error, TEXT("%s"), *FailureReason.ToString());
	FMessageDialog::Open(EAppMsgType::Ok, FailureReason);
    
	return false;
}


bool FProjectManager::LoadModulesForProject( const ELoadingPhase::Type LoadingPhase )
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading Game Modules"), STAT_GameModule, STATGROUP_LoadTime);

	bool bSuccess = true;

	if ( CurrentlyLoadedProject.IsValid() )
	{
		TMap<FName, ELoadModuleFailureReason::Type> ModuleLoadFailures;
		CurrentlyLoadedProject->LoadModules(LoadingPhase, ModuleLoadFailures);

		if ( ModuleLoadFailures.Num() > 0 )
		{
			FText FailureMessage;
			for ( auto FailureIt = ModuleLoadFailures.CreateConstIterator(); FailureIt; ++FailureIt )
			{
				const ELoadModuleFailureReason::Type FailureReason = FailureIt.Value();

				if( FailureReason != ELoadModuleFailureReason::Success )
				{
					const FText TextModuleName = FText::FromName(FailureIt.Key());

					if ( FailureReason == ELoadModuleFailureReason::FileNotFound )
					{
						FailureMessage = FText::Format( LOCTEXT("PrimaryGameModuleNotFound", "The game module '{0}' could not be found. Please ensure that this module exists and that it is compiled."), TextModuleName );
					}
					else if ( FailureReason == ELoadModuleFailureReason::FileIncompatible )
					{
						FailureMessage = FText::Format( LOCTEXT("PrimaryGameModuleIncompatible", "The game module '{0}' does not appear to be up to date. This may happen after updating the engine. Please recompile this module and try again."), TextModuleName );
					}
					else if ( FailureReason == ELoadModuleFailureReason::FailedToInitialize )
					{
						FailureMessage = FText::Format( LOCTEXT("PrimaryGameModuleFailedToInitialize", "The game module '{0}' could not be successfully initialized after it was loaded."), TextModuleName );
					}
					else if ( FailureReason == ELoadModuleFailureReason::CouldNotBeLoadedByOS )
					{
						FailureMessage = FText::Format( LOCTEXT("PrimaryGameModuleCouldntBeLoaded", "The game module '{0}' could not be loaded. There may be an operating system error or the module may not be properly set up."), TextModuleName );
					}
					else 
					{
						ensure(0);	// If this goes off, the error handling code should be updated for the new enum values!
						FailureMessage = FText::Format( LOCTEXT("PrimaryGameModuleGenericLoadFailure", "The game module '{0}' failed to load for an unspecified reason.  Please report this error."), TextModuleName );
					}

					// Just report the first error
					break;
				}
			}

			FMessageDialog::Open(EAppMsgType::Ok, FailureMessage);
			bSuccess = false;
		}
	}

	return bSuccess;
}

bool FProjectManager::AreProjectModulesUpToDate()
{
	return !CurrentlyLoadedProject.IsValid() || CurrentlyLoadedProject->AreModulesUpToDate();
}

const FString& FProjectManager::GetAutoLoadProjectFileName()
{
	static FString RecentProjectFileName = FPaths::Combine(*FPaths::GameAgnosticSavedDir(), TEXT("AutoLoadProject.txt"));
	return RecentProjectFileName;
}

const FString& FProjectManager::NonStaticGetProjectFileExtension()
{
	static FString GameProjectFileExtension(TEXT("uproject"));
	return GameProjectFileExtension;
}

bool FProjectManager::GenerateNewProjectFile(const FString& NewProjectFilename, const TArray<FString>& StartupModuleNames, const FString& EngineIdentifier, FText& OutFailReason)
{
	TSharedRef<FProject> NewProject = MakeShareable( new FProject() );
	NewProject->UpdateVersionToCurrent(EngineIdentifier);
	NewProject->ReplaceModulesInProject(&StartupModuleNames);

	const FString& FileContents = NewProject->SerializeToJSON();
	if ( FFileHelper::SaveStringToFile(FileContents, *NewProjectFilename) )
	{
		return true;
	}
	else
	{
		OutFailReason = FText::Format( LOCTEXT("FailedToWriteOutputFile", "Failed to write output file '{0}'. Perhaps the file is Read-Only?"), FText::FromString(NewProjectFilename) );
		return false;
	}
}

bool FProjectManager::DuplicateProjectFile(const FString& SourceProjectFilename, const FString& NewProjectFilename, const FString& EngineIdentifier, FText& OutFailReason)
{
	// Load the source project
	TSharedRef<FProject> SourceProject = MakeShareable( new FProject() );
	if ( !SourceProject->LoadFromFile(SourceProjectFilename, OutFailReason) )
	{
		return false;
	}

	// Duplicate the project info
	FProjectInfo ProjectInfo = SourceProject->GetProjectInfo();

	// Clear the sample hash
	ProjectInfo.EpicSampleNameHash = 0;

	// Fix up module names
	const FString BaseSourceName = FPaths::GetBaseFilename(SourceProjectFilename);
	const FString BaseNewName = FPaths::GetBaseFilename(NewProjectFilename);
	for ( auto ModuleIt = ProjectInfo.Modules.CreateIterator(); ModuleIt; ++ModuleIt )
	{
		FProjectOrPluginInfo::FModuleInfo& ModuleInfo = *ModuleIt;
		ModuleInfo.Name = FName(*ModuleInfo.Name.ToString().Replace(*BaseSourceName, *BaseNewName));
	}

	// Create new project, update version numbers (no need to replace modules here)
	TSharedRef<FProject> NewProject = MakeShareable( new FProject(ProjectInfo) );
	NewProject->UpdateVersionToCurrent(EngineIdentifier);

	// Serialize and write to disk
	const FString& FileContents = NewProject->SerializeToJSON();
	if ( FFileHelper::SaveStringToFile(FileContents, *NewProjectFilename) )
	{
		return true;
	}
	else
	{
		OutFailReason = FText::Format( LOCTEXT("FailedToWriteOutputFile", "Failed to write output file '{0}'. Perhaps the file is Read-Only?"), FText::FromString(NewProjectFilename) );
		return false;
	}
}

bool FProjectManager::UpdateLoadedProjectFileToCurrent(const TArray<FString>* StartupModuleNames, const FString& EngineIdentifier, FText& OutFailReason)
{
	if ( !CurrentlyLoadedProject.IsValid() )
	{
		return false;
	}

	// Freshen version information
	CurrentlyLoadedProject->UpdateVersionToCurrent(EngineIdentifier);

	// Replace the modules names, if specified
	CurrentlyLoadedProject->ReplaceModulesInProject(StartupModuleNames);

	// Update file on disk
	const FString& FileContents = CurrentlyLoadedProject->SerializeToJSON();
	if ( FFileHelper::SaveStringToFile(FileContents, *FPaths::GetProjectFilePath()) )
	{
		return true;
	}
	else
	{
		// We failed to generate the file. Could be read only.
		OutFailReason = FText::Format( LOCTEXT("FailedToWriteOutputFile", "Failed to write output file '{0}'. Perhaps the file is Read-Only?"), FText::FromString(FPaths::GetProjectFilePath()) );
		return false;
	}
}

bool FProjectManager::SignSampleProject(const FString& FilePath, const FString& Category, FText& OutFailReason)
{
	TSharedRef<FProject> NewProject = MakeShareable( new FProject() );
	if ( !NewProject->LoadFromFile(FilePath, OutFailReason) )
	{
		return false;
	}

	NewProject->SignSampleProject(FilePath, Category);

	const FString& FileContents = NewProject->SerializeToJSON();
	if (FFileHelper::SaveStringToFile(FileContents, *FilePath))
	{
		return true;
	}
	else
	{
		OutFailReason = FText::Format( LOCTEXT("FailedToSaveSignedProject", "Failed to save signed project file {0}"), FText::FromString(FilePath) );
		return false;
	}
}

bool FProjectManager::QueryStatusForProject(const FString& FilePath, FProjectStatus& OutProjectStatus) const
{
	TSharedRef<FProject> NewProject = MakeShareable( new FProject() );
	FText FailReason;
	if ( !NewProject->LoadFromFile(FilePath, FailReason) )
	{
		return false;
	}

	const FProjectInfo& ProjectInfo = NewProject->GetProjectInfo();
	OutProjectStatus.Name = ProjectInfo.Name;
	OutProjectStatus.Description = ProjectInfo.Description;
	OutProjectStatus.Category = ProjectInfo.Category;
	OutProjectStatus.bCodeBasedProject = ProjectInfo.Modules.Num() > 0;
	OutProjectStatus.bSignedSampleProject = NewProject->IsSignedSampleProject(FilePath);
	OutProjectStatus.bRequiresUpdate = NewProject->RequiresUpdate();

	return true;
}

IProjectManager& IProjectManager::Get()
{
	// Single instance of manager, allocated on demand and destroyed on program exit.
	static FProjectManager* ProjectManager = NULL;
	if( ProjectManager == NULL )
	{
		ProjectManager = new FProjectManager();
	}
	return *ProjectManager;
}

#undef LOCTEXT_NAMESPACE