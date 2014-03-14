// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealHeaderTool.h"

#include "RequiredProgramMainCPPInclude.h"

IMPLEMENT_APPLICATION(UnrealHeaderTool, "UnrealHeaderTool");

DEFINE_LOG_CATEGORY(LogCompile);

/**
 * Application entry point
 *
 * @param	ArgC	Command-line argument count
 * @param	ArgV	Argument strings
 */
int32 main( int32 ArgC, char* ArgV[] )
{
	FString CmdLine;

	for (int32 Arg = 0; Arg < ArgC; Arg++)
	{
		FString LocalArg = ArgV[Arg];
		if (LocalArg.Contains(TEXT(" ")))
		{
			CmdLine += TEXT("\"");
			CmdLine += LocalArg;
			CmdLine += TEXT("\"");
		}
		else
		{
			CmdLine += LocalArg;
		}

		if (Arg + 1 < ArgC)
		{
			CmdLine += TEXT(" ");
		}
	}

	FString ShortCmdLine = FCommandLine::RemoveExeName(*CmdLine);
	ShortCmdLine = ShortCmdLine.Trim();	

	// Get game name from the commandline. It will later be used to load the correct ini files.
	FString ModuleInfoFilename;
	if (ShortCmdLine.Len() && **ShortCmdLine != TEXT('-'))
	{
		const TCHAR* CmdLinePtr = *ShortCmdLine;

		// Parse the game name.  UHT doesn't care about this directly, but the engine init code will be
		// expecting it when it parses the command-line itself.
		FString GameName = FParse::Token(CmdLinePtr, false);

		// This parameter is the absolute path to the file which contains information about the modules
		// that UHT needs to generate code for.
		ModuleInfoFilename = FParse::Token(CmdLinePtr, false );
	}

	GIsUCCMakeStandaloneHeaderGenerator = true;
	GEngineLoop.PreInit(*ShortCmdLine);

	if (ModuleInfoFilename.IsEmpty())
	{
		if (!FPlatformMisc::IsDebuggerPresent())
		{
			UE_LOG(LogCompile, Error, TEXT( "Missing module info filename on command line" ));
			return 1;
		}

		// If we have a debugger, let's use a pre-existing manifest file to streamline debugging
		// without the user having to faff around trying to get a UBT-generated manifest
		ModuleInfoFilename = FPaths::ConvertRelativePathToFull(FPlatformProcess::BaseDir(), TEXT("../../Source/Programs/UnrealHeaderTool/Resources/UHTDebugging.manifest"));
	}

	extern int32 UnrealHeaderTool_Main(const FString& ModuleInfoFilename);
	int32 ErrorLevel = UnrealHeaderTool_Main(ModuleInfoFilename);

	if (GWarn->Errors.Num() > 0 || GWarn->Warnings.Num() > 0)
	{
		ErrorLevel = 1;
	}

	FEngineLoop::AppPreExit();
	FEngineLoop::AppExit();

	return ErrorLevel;
}

