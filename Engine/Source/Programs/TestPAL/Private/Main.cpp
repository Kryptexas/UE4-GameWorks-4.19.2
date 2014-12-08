// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PrivatePCH.h"
#include "RequiredProgramMainCPPInclude.h"

DEFINE_LOG_CATEGORY(LogTestPAL);

IMPLEMENT_APPLICATION(TestPAL, "TestPAL");

#define CHILD_INVOCATION_ARG			"-child"

namespace
{
	/**
	 * Checks if this instance is nested (i.e. run under itself)
	 *
	 * @param ArgC Number of commandline arguments.
	 * @param ArgV Commandline arguments
	 * @return true if nested
	 */
	bool IsNestedInstance(int32 ArgC, ANSICHAR* ArgV[])
	{
		for (int32 IdxArg = 0; IdxArg < ArgC; ++IdxArg)
		{
			if (!FCStringAnsi::Strcmp(ArgV[IdxArg], CHILD_INVOCATION_ARG))
			{
				return true;
			}
		}

		return false;
	}
}

int32 RunAsChild(const TCHAR* CommandLine)
{
	FPlatformMisc::SetCrashHandler(NULL);
	FPlatformMisc::SetGracefulTerminationHandler();

	GEngineLoop.PreInit(CommandLine);

	// set a random delay pretending to do some useful work up to a minute. 
	double RandomWorkTime = FMath::FRandRange(0.0f, 60.0f);

	UE_LOG(LogTestPAL, Display, TEXT("Running as child (pid %d), will be doing work for %f seconds."), FPlatformProcess::GetCurrentProcessId(), RandomWorkTime);

	double StartTime = FPlatformTime::Seconds();

	// Use all the CPU!
	for (;;)
	{
		double CurrentTime = FPlatformTime::Seconds();
		if (CurrentTime - StartTime >= RandomWorkTime)
		{
			break;
		}
	}

	UE_LOG(LogTestPAL, Display, TEXT("Child (pid %d) finished work."), FPlatformProcess::GetCurrentProcessId());

	FEngineLoop::AppPreExit();
	FEngineLoop::AppExit();
	return 0;
}

int32 RunAsParent(const TCHAR* CommandLine)
{
	FPlatformMisc::SetCrashHandler(NULL);
	FPlatformMisc::SetGracefulTerminationHandler();

	GEngineLoop.PreInit(CommandLine);
	UE_LOG(LogTestPAL, Display, TEXT("Running as parent."));

	// Run slave instance continuously
	int NumChildrenToSpawn = 255, MaxAtOnce = 5;
	FParent Parent(NumChildrenToSpawn, MaxAtOnce);

	Parent.Run();

	UE_LOG(LogTestPAL, Display, TEXT("Parent quit."));

	FEngineLoop::AppPreExit();
	FEngineLoop::AppExit();
	return 0;
}

int32 main(int32 ArgC, char* ArgV[])
{
	FString CommandLine;
	for (int32 Option = 1; Option < ArgC; Option++)
	{
		CommandLine += TEXT(" ");
		FString Argument(ANSI_TO_TCHAR(ArgV[Option]));
		if (Argument.Contains(TEXT(" ")))
		{
			if (Argument.Contains(TEXT("=")))
			{
				FString ArgName;
				FString ArgValue;
				Argument.Split( TEXT("="), &ArgName, &ArgValue );
				Argument = FString::Printf( TEXT("%s=\"%s\""), *ArgName, *ArgValue );
			}
			else
			{
				Argument = FString::Printf(TEXT("\"%s\""), *Argument);
			}
		}
		CommandLine += Argument;
	}

	// fork and run under self
	return IsNestedInstance(ArgC, ArgV) ? RunAsChild(*CommandLine) : RunAsParent(*CommandLine);
}
