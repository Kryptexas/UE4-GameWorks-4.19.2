// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FLauncherUATTask.h: Declares the FLauncherUATTask class.
=============================================================================*/

#pragma once


/**
 * class for UAT launcher tasks.
 */
static const FString ConfigStrings[] = { TEXT("Unknown"), TEXT("Debug"), TEXT("DebugGame"), TEXT("Development"), TEXT("Shipping"), TEXT("Test") };

class FLauncherUATTask
	: public FLauncherTask
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InCommand - the command used for the task
	 * @param InTargetPlatform - the target platform for the task
	 * @param InName - The name of the task.
	 */
	FLauncherUATTask( const TSharedPtr<FLauncherUATCommand>& InCommand, const ITargetPlatform& InTargetPlatform, const FString& InName, void* InReadPipe, void* InWritePipe)
		: FLauncherTask(InName, InCommand->GetDesc(), InReadPipe, InWritePipe)
		, TaskCommand(InCommand)
		, TargetPlatform(InTargetPlatform)
	{
		NoCompile = !FParse::Param( FCommandLine::Get(), TEXT("development") ) ? TEXT(" -nocompile") : TEXT("");
	}


protected:

	/**
	 * Performs the actual task.
	 *
	 * @param ChainState - Holds the state of the task chain.
	 *
	 * @return true if the task completed successfully, false otherwise.
	 */
	virtual bool PerformTask( FLauncherTaskChainState& ChainState ) OVERRIDE
	{
		// spawn a UAT process to cook the data
		// UAT executable
		FString ExecutablePath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() + FString(TEXT("Build")) / TEXT("BatchFiles"));
#if PLATFORM_MAC
		FString Executable = TEXT("RunUAT.command");
#else
		FString Executable = TEXT("RunUAT.bat");
#endif

		// switch server and no editor platforms to the proper type
		FString Platform = TargetPlatform.PlatformName();
		if (TargetPlatform.PlatformName() == TEXT("LinuxServer"))
		{
			Platform = TEXT("Linux");
		}
		else if (TargetPlatform.PlatformName() == TEXT("WindowsServer") || TargetPlatform.PlatformName() == TEXT("WindowsNoEditor") || TargetPlatform.PlatformName() == TEXT("Windows"))
		{
			Platform = TEXT("Win64");
		}
        else if (TargetPlatform.PlatformName() == TEXT("MacNoEditor"))
        {
            Platform = TEXT("Mac");
        }

		// for things like Android_ATC, pull it apart to be Android, with cook flavor ATC
		FString CookFlavor;
		int32 UnderscoreLoc;
		if (Platform.FindChar(TEXT('_'), UnderscoreLoc))
		{
			CookFlavor = FString(TEXT(" -cookflavor=")) + Platform.Mid(UnderscoreLoc + 1);
			Platform = Platform.Mid(0, UnderscoreLoc);
		}

        // check for rocket
        FString Rocket;
        if ( FRocketSupport::IsRocket() )
        {
            Rocket = TEXT(" -rocket" );
        }
        
		// base UAT command arguments
		FString CommandLine;
		FString ProjectPath = *ChainState.Profile->GetProjectPath();
		ProjectPath = FPaths::ConvertRelativePathToFull(ProjectPath);
		CommandLine = FString::Printf(TEXT("BuildCookRun -project=\"%s\" -noP4 -platform=%s -clientconfig=%s -serverconfig=%s"),
			*ProjectPath,
			*Platform,
			*ConfigStrings[ChainState.Profile->GetBuildConfiguration()],
			*ConfigStrings[ChainState.Profile->GetBuildConfiguration()]);

		CommandLine += NoCompile;
        CommandLine += Rocket;
		CommandLine += CookFlavor;

		// specialized command arguments for this particular task
		CommandLine += TaskCommand->GetArguments(ChainState);

		// pre-execute any command
		if (!TaskCommand->PreExecute(ChainState))
		{
			return false;
		}

		// launch UAT and monitor its progress
		FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*(ExecutablePath / Executable), *CommandLine, false, true, true, NULL, 0, *ExecutablePath, WritePipe);

		while (!TaskCommand->IsComplete() || FPlatformProcess::IsProcRunning(ProcessHandle))
		{
			if (GetStatus() == ELauncherTaskStatus::Canceling)
			{
				FPlatformProcess::TerminateProc(ProcessHandle, true);
				return false;
			}

			FPlatformProcess::Sleep(0.25);
		}

		int32 ReturnCode;
		if (!FPlatformProcess::GetProcReturnCode(ProcessHandle, &ReturnCode))
		{
			return false;
		}

		if (!TaskCommand->PostExecute(ChainState))
		{
			return false;
		}

		return (ReturnCode == 0);
	}


private:

	// command for this task
	const TSharedPtr<FLauncherUATCommand> TaskCommand;

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;

	// Holds the no compile flag
	FString NoCompile;
};
