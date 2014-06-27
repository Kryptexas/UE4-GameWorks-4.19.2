// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectsPrivatePCH.h"

DEFINE_LOG_CATEGORY_STATIC( LogProjectManager, Log, All );

#define LOCTEXT_NAMESPACE "ProjectManager"

FProjectManager::FProjectManager()
{
}

bool FProjectManager::LoadProjectFile( const FString& InProjectFile )
{
	// Try to load the descriptor
	FText FailureReason;
	TSharedPtr<FProjectDescriptor> Descriptor = MakeShareable(new FProjectDescriptor());
	if(Descriptor->Load(InProjectFile, FailureReason))
	{
		// Create the project
		CurrentlyLoadedProject = Descriptor;
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
		TMap<FName, EModuleLoadResult> ModuleLoadFailures;
		FModuleDescriptor::LoadModulesForPhase(LoadingPhase, CurrentlyLoadedProject->Modules, ModuleLoadFailures);

		if ( ModuleLoadFailures.Num() > 0 )
		{
			FText FailureMessage;
			for ( auto FailureIt = ModuleLoadFailures.CreateConstIterator(); FailureIt; ++FailureIt )
			{
				const EModuleLoadResult FailureReason = FailureIt.Value();

				if( FailureReason != EModuleLoadResult::Success )
				{
					const FText TextModuleName = FText::FromName(FailureIt.Key());

					if ( FailureReason == EModuleLoadResult::FileNotFound )
					{
						FailureMessage = FText::Format( LOCTEXT("PrimaryGameModuleNotFound", "The game module '{0}' could not be found. Please ensure that this module exists and that it is compiled."), TextModuleName );
					}
					else if ( FailureReason == EModuleLoadResult::FileIncompatible )
					{
						FailureMessage = FText::Format( LOCTEXT("PrimaryGameModuleIncompatible", "The game module '{0}' does not appear to be up to date. This may happen after updating the engine. Please recompile this module and try again."), TextModuleName );
					}
					else if ( FailureReason == EModuleLoadResult::FailedToInitialize )
					{
						FailureMessage = FText::Format( LOCTEXT("PrimaryGameModuleFailedToInitialize", "The game module '{0}' could not be successfully initialized after it was loaded."), TextModuleName );
					}
					else if ( FailureReason == EModuleLoadResult::CouldNotBeLoadedByOS )
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
	return !CurrentlyLoadedProject.IsValid() || FModuleDescriptor::AreModulesUpToDate(CurrentlyLoadedProject->Modules);
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
	FProjectDescriptor Descriptor;
	Descriptor.EngineAssociation = EngineIdentifier;

	for(int32 Idx = 0; Idx < StartupModuleNames.Num(); Idx++)
	{
		Descriptor.Modules.Add(FModuleDescriptor(*StartupModuleNames[Idx]));
	}

	return Descriptor.Save(NewProjectFilename, OutFailReason);
}

bool FProjectManager::DuplicateProjectFile(const FString& SourceProjectFilename, const FString& NewProjectFilename, const FString& EngineIdentifier, FText& OutFailReason)
{
	// Load the source project
	FProjectDescriptor Project;
	if(!Project.Load(SourceProjectFilename, OutFailReason))
	{
		return false;
	}

	// Update it to current
	Project.EngineAssociation = EngineIdentifier;
	Project.EpicSampleNameHash = 0;

	// Fix up module names
	const FString BaseSourceName = FPaths::GetBaseFilename(SourceProjectFilename);
	const FString BaseNewName = FPaths::GetBaseFilename(NewProjectFilename);
	for ( auto ModuleIt = Project.Modules.CreateIterator(); ModuleIt; ++ModuleIt )
	{
		FModuleDescriptor& ModuleInfo = *ModuleIt;
		ModuleInfo.Name = FName(*ModuleInfo.Name.ToString().Replace(*BaseSourceName, *BaseNewName));
	}

	// Save it to disk
	return Project.Save(NewProjectFilename, OutFailReason);
}

bool FProjectManager::UpdateLoadedProjectFileToCurrent(const TArray<FString>* StartupModuleNames, const FString& EngineIdentifier, FText& OutFailReason)
{
	if ( !CurrentlyLoadedProject.IsValid() )
	{
		return false;
	}

	// Freshen version information
	CurrentlyLoadedProject->EngineAssociation = EngineIdentifier;

	// Replace the modules names, if specified
	if(StartupModuleNames != NULL)
	{
		CurrentlyLoadedProject->Modules.Empty();
		for(int32 Idx = 0; Idx < StartupModuleNames->Num(); Idx++)
		{
			CurrentlyLoadedProject->Modules.Add(FModuleDescriptor(*(*StartupModuleNames)[Idx]));
		}
	}

	// Update file on disk
	return CurrentlyLoadedProject->Save(FPaths::GetProjectFilePath(), OutFailReason);
}

bool FProjectManager::SignSampleProject(const FString& FilePath, const FString& Category, FText& OutFailReason)
{
	FProjectDescriptor Descriptor;
	if(!Descriptor.Load(FilePath, OutFailReason))
	{
		return false;
	}

	Descriptor.Sign(FilePath);
	Descriptor.Category = Category;
	return Descriptor.Save(FilePath, OutFailReason);
}

bool FProjectManager::QueryStatusForProject(const FString& FilePath, FProjectStatus& OutProjectStatus) const
{
	FText FailReason;
	FProjectDescriptor Descriptor;
	if(!Descriptor.Load(FilePath, FailReason))
	{
		return false;
	}

	QueryStatusForProjectImpl(Descriptor, FilePath, OutProjectStatus);
	return true;
}

bool FProjectManager::QueryStatusForCurrentProject(FProjectStatus& OutProjectStatus) const
{
	if ( !CurrentlyLoadedProject.IsValid() )
	{
		return false;
	}

	QueryStatusForProjectImpl(*CurrentlyLoadedProject, FPaths::GetProjectFilePath(), OutProjectStatus);
	return true;
}

void FProjectManager::QueryStatusForProjectImpl(const FProjectDescriptor& ProjectInfo, const FString& FilePath, FProjectStatus& OutProjectStatus)
{
	OutProjectStatus.Name = FPaths::GetBaseFilename(FilePath);
	OutProjectStatus.Description = ProjectInfo.Description;
	OutProjectStatus.Category = ProjectInfo.Category;
	OutProjectStatus.bCodeBasedProject = ProjectInfo.Modules.Num() > 0;
	OutProjectStatus.bSignedSampleProject = ProjectInfo.IsSigned(FilePath);
	OutProjectStatus.bRequiresUpdate = ProjectInfo.FileVersion < EProjectDescriptorVersion::Latest;
	OutProjectStatus.TargetPlatforms = ProjectInfo.TargetPlatforms;
}

void FProjectManager::UpdateSupportedTargetPlatformsForProject(const FString& FilePath, const FName& InPlatformName, const bool bIsSupported)
{
	FProjectDescriptor NewProject;

	FText FailReason;
	if ( !NewProject.Load(FilePath, FailReason) )
	{
		return;
	}

	if ( bIsSupported )
	{
		NewProject.TargetPlatforms.AddUnique(InPlatformName);
	}
	else
	{
		NewProject.TargetPlatforms.Remove(InPlatformName);
	}

	NewProject.Save(FilePath, FailReason);

	// Call OnTargetPlatformsForCurrentProjectChangedEvent if this project is the same as the one we currently have loaded
	const FString CurrentProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	const FString InProjectPath = FPaths::ConvertRelativePathToFull(FilePath);
	if ( CurrentProjectPath == InProjectPath )
	{
		OnTargetPlatformsForCurrentProjectChangedEvent.Broadcast();
	}
}

void FProjectManager::UpdateSupportedTargetPlatformsForCurrentProject(const FName& InPlatformName, const bool bIsSupported)
{
	if ( !CurrentlyLoadedProject.IsValid() )
	{
		return;
	}

	CurrentlyLoadedProject->UpdateSupportedTargetPlatforms(InPlatformName, bIsSupported);

	FText FailReason;
	CurrentlyLoadedProject->Save(FPaths::GetProjectFilePath(), FailReason);

	OnTargetPlatformsForCurrentProjectChangedEvent.Broadcast();
}

void FProjectManager::ClearSupportedTargetPlatformsForProject(const FString& FilePath)
{
	FProjectDescriptor Descriptor;

	FText FailReason;
	if ( !Descriptor.Load(FilePath, FailReason) )
	{
		return;
	}

	Descriptor.TargetPlatforms.Empty();
	Descriptor.Save(FilePath, FailReason);

	// Call OnTargetPlatformsForCurrentProjectChangedEvent if this project is the same as the one we currently have loaded
	const FString CurrentProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	const FString InProjectPath = FPaths::ConvertRelativePathToFull(FilePath);
	if ( CurrentProjectPath == InProjectPath )
	{
		OnTargetPlatformsForCurrentProjectChangedEvent.Broadcast();
	}
}

void FProjectManager::ClearSupportedTargetPlatformsForCurrentProject()
{
	if ( !CurrentlyLoadedProject.IsValid() )
	{
		return;
	}

	CurrentlyLoadedProject->TargetPlatforms.Empty();

	FText FailReason;
	CurrentlyLoadedProject->Save(FPaths::GetProjectFilePath(), FailReason);

	OnTargetPlatformsForCurrentProjectChangedEvent.Broadcast();
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