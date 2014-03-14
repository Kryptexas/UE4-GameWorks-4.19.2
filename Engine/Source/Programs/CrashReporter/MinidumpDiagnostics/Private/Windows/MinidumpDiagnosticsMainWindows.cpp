// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MinidumpDiagnosticsApp.h"

#include "Core.h"
#include "RequiredProgramMainCPPInclude.h"
#include "ExceptionHandling.h"

IMPLEMENT_APPLICATION( MinidumpDiagnostics, "MinidumpDiagnostics" )

// More Windows glue
int32 GuardedMain( int32 Argc, ANSICHAR* Argv[] )
{
	GEngineLoop.PreInit( Argc, Argv );

#if PLATFORM_WINDOWS
	SetConsoleTitle( TEXT( "MinidumpDiagnostics" ) );
#endif

	int32 Result = RunMinidumpDiagnostics( Argc, Argv );
	return Result;
}

// Windows glue
int32 GuardedMainWrapper( int32 ArgC, ANSICHAR* ArgV[] )
{
	int32 ReturnCode = 0;

#if !PLATFORM_MAC
	if (FPlatformMisc::IsDebuggerPresent())
#endif
	{
		ReturnCode = GuardedMain( ArgC, ArgV );
	}
#if !PLATFORM_MAC
	else
	{
		__try
		{
			GIsGuarded = 1;
			ReturnCode = GuardedMain( ArgC, ArgV );
			GIsGuarded = 0;
		}
		__except( NullReportCrash( GetExceptionInformation() ) )
		{
		}
	}
#endif

	FEngineLoop::AppPreExit();
	FEngineLoop::AppExit();

	return ReturnCode;
}

 // Main entry point to the application
int32 main( int32 ArgC, ANSICHAR* ArgV[] )
{
	const int32 Result = GuardedMainWrapper( ArgC, ArgV );
	return Result;
}
