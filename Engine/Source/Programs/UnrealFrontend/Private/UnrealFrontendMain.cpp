// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealFrontendMain.h"
#include "RequiredProgramMainCPPInclude.h"

#include "DeployCommand.h"
#include "LaunchCommand.h"
#include "PackageCommand.h"
#include "StatsConvertCommand.h"
#include "StatsDumpMemoryCommand.h"
#include "UserInterfaceCommand.h"
#include "LaunchFromProfileCommand.h"

#include "Interfaces/ILauncherWorker.h"
#include "ProjectDescriptor.h"
#include "IProjectManager.h"

IMPLEMENT_APPLICATION(UnrealFrontend, "UnrealFrontend");

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLaunchStartedDelegate, ILauncherProfilePtr, double);

extern LAUNCHERSERVICES_API FOnStageStartedDelegate GLauncherWorker_StageStarted;
extern LAUNCHERSERVICES_API FOnLaunchStartedDelegate GLauncherWorker_LaunchStarted;
extern LAUNCHERSERVICES_API FOnLaunchCanceledDelegate GLauncherWorker_LaunchCanceled;
extern LAUNCHERSERVICES_API FOnLaunchCompletedDelegate GLauncherWorker_LaunchCompleted;

bool bWasBlueprintNativizationEnabled = false;

bool EnableBlueprintNativization(bool bEnable)
{
	static const FString NativizedPluginName = TEXT("NativizedAssets");
	static const FString NativizedPluginPath = TEXT("./Intermediate/Plugins");

	bool bWasEnabled = false;
	IProjectManager& ProjectManager = IProjectManager::Get();
	if (const FProjectDescriptor* CurrentProject = ProjectManager.GetCurrentProject())
	{
		FText OutFailReason;
		const FString FullPluginPath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(FPaths::GetProjectFilePath()), NativizedPluginPath);
		bWasEnabled = CurrentProject->GetAdditionalPluginDirectories().Contains(FullPluginPath);
		if (bWasEnabled && !bEnable)
		{
			// Update the project file to disable Blueprint nativization.
			ProjectManager.UpdateAdditionalPluginDirectory(FullPluginPath, false);
			ProjectManager.SetPluginEnabled(NativizedPluginName, false, OutFailReason);
			ProjectManager.SaveCurrentProjectToDisk(OutFailReason);
		}
		else if (!bWasEnabled && bEnable)
		{
			// Update the project file to enable/restore Blueprint nativization.
			ProjectManager.UpdateAdditionalPluginDirectory(FullPluginPath, true);
			ProjectManager.RemovePluginReference(NativizedPluginName, OutFailReason);
			ProjectManager.SaveCurrentProjectToDisk(OutFailReason);
		}
	}

	return bWasEnabled;
}

void OnLauncherWorker_LaunchStarted(ILauncherProfilePtr InProfile, double InStartTime)
{
	if (InProfile.IsValid())
	{
		// Set the global project file path.
		FPaths::SetProjectFilePath(InProfile->GetProjectPath());

		// Load the current project file. This is not currently handled by UFE.
		IProjectManager::Get().LoadProjectFile(InProfile->GetProjectPath());

		// If enabled, temporarily disable Blueprint nativization on the project file.
		bWasBlueprintNativizationEnabled = EnableBlueprintNativization(false);
	}
}

void OnLauncherWorker_StageStarted(const FString& InStageName)
{
	// Restore Blueprint nativization on the project file.
	if (bWasBlueprintNativizationEnabled && InStageName.Equals(TEXT("Run Task")))
	{
		EnableBlueprintNativization(true);
	}
}

void OnLauncherWorker_LaunchCanceled(double InDuration)
{
	// Restore Blueprint nativization on the project file.
	if (bWasBlueprintNativizationEnabled)
	{
		EnableBlueprintNativization(true);
	}
}

void OnLauncherWorker_LaunchCompleted(bool bSucceeded, double InDuration, int32 InResultCode)
{
	// Restore Blueprint nativization on the project file.
	if (bWasBlueprintNativizationEnabled)
	{
		EnableBlueprintNativization(true);
	}
}

/**
 * Platform agnostic implementation of the main entry point.
 */
int32 UnrealFrontendMain( const TCHAR* CommandLine )
{
	// Override the stack size for the thread pool.
	FQueuedThreadPool::OverrideStackSize = 256 * 1024;

	FCommandLine::Set(CommandLine);

	FString Command;
	FString Params;
	FString NewCommandLine = CommandLine;

	// process command line parameters
	bool bRunCommand = FParse::Value(*NewCommandLine, TEXT("-RUN="), Command);
	
	if (Command.IsEmpty())
	{
		GLog->Logf(ELogVerbosity::Warning, TEXT("The command line argument '-RUN=' does not have a command name associated with it."));
	}

	if (bRunCommand)
	{
		// extract off any '-PARAM=' parameters so that they aren't accidentally parsed by engine init
		FParse::Value(*NewCommandLine, TEXT("-PARAMS="), Params);

		if (Params.Len() > 0)
		{
			// remove from the command line & trim quotes
			NewCommandLine = NewCommandLine.Replace(*Params, TEXT(""));
			Params = Params.TrimQuotes();
		}
	}

	// Add -Messaging if it was not given in the command line.
	if (!FParse::Param(*NewCommandLine, TEXT("MESSAGING")))
	{
		NewCommandLine += TEXT(" -Messaging");
	}

	// Add '-Log' if the Frontend was run with '-RUN=' without '-LOG' so we can read any potential log output.
	if (bRunCommand && !FParse::Param(*NewCommandLine, TEXT("LOG")))
	{
		NewCommandLine += TEXT(" -Log");
	}

	// initialize core
	GEngineLoop.PreInit(*NewCommandLine);
	FModuleManager::Get().StartProcessingNewlyLoadedObjects();

	bool Succeeded = true;

	FDelegateHandle LauncherWorker_OnStageStarted = GLauncherWorker_StageStarted.AddStatic(&OnLauncherWorker_StageStarted);
	FDelegateHandle LauncherWorker_OnLaunchStarted = GLauncherWorker_LaunchStarted.AddStatic(&OnLauncherWorker_LaunchStarted);
	FDelegateHandle LauncherWorker_OnLaunchCanceled = GLauncherWorker_LaunchCanceled.AddStatic(&OnLauncherWorker_LaunchCanceled);
	FDelegateHandle LauncherWorker_OnLaunchCompleted = GLauncherWorker_LaunchCompleted.AddStatic(&OnLauncherWorker_LaunchCompleted);

	// Execute desired command
	// To execute, run with '-RUN="COMMAND_NAME_FOUND_BELOW"'. 
	// NOTE - Some commands may require extra command line parameters.
	if (bRunCommand)
	{
		if (Command.Equals(TEXT("PACKAGE"), ESearchCase::IgnoreCase))
		{
			FPackageCommand::Run();
		}
		else if (Command.Equals(TEXT("DEPLOY"), ESearchCase::IgnoreCase))
		{
			Succeeded = FDeployCommand::Run();
		}
		else if (Command.Equals(TEXT("LAUNCH"), ESearchCase::IgnoreCase))
		{
			Succeeded = FLaunchCommand::Run(Params);
		}
		else if (Command.Equals(TEXT("CONVERT"), ESearchCase::IgnoreCase))
		{
			FStatsConvertCommand::Run();
		}
		else if( Command.Equals( TEXT("MEMORYDUMP"), ESearchCase::IgnoreCase ) )
		{
			FStatsMemoryDumpCommand::Run();
		}
		// The 'LAUNCHPROFILE' command also needs '-PROFILENAME="MY_PROFILE_NAME"' as a command line parameter.
		else if (Command.Equals(TEXT("LAUNCHPROFILE"), ESearchCase::IgnoreCase))
		{
			FLaunchFromProfileCommand* ProfileLaunch = new FLaunchFromProfileCommand;
			ProfileLaunch->Run(Params);
		}
	}
	else
	{
		FUserInterfaceCommand::Run();
	}

	GLauncherWorker_StageStarted.Remove(LauncherWorker_OnStageStarted);
	GLauncherWorker_LaunchStarted.Remove(LauncherWorker_OnLaunchStarted);
	GLauncherWorker_LaunchCanceled.Remove(LauncherWorker_OnLaunchCanceled);
	GLauncherWorker_LaunchCompleted.Remove(LauncherWorker_OnLaunchCompleted);

	// shut down
	FEngineLoop::AppPreExit();
	FModuleManager::Get().UnloadModulesAtShutdown();

#if STATS
	FThreadStats::StopThread();
#endif

	FTaskGraphInterface::Shutdown();

	return Succeeded ? 0 : -1;
}
