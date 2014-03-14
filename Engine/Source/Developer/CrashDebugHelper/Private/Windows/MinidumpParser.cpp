// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashDebugHelperPrivatePCH.h"
#include "Database.h"

#include "Windows/WindowsPlatformStackWalkExt.h"
#include "ISourceControlModule.h"

#include "AllowWindowsPlatformTypes.h"

bool FCrashDebugHelperWindows::CreateMinidumpDiagnosticReport( const FString& InCrashDumpFilename )
{
	bool bResult = false;

	const bool bSyncSymbols = FParse::Param( FCommandLine::Get(), TEXT( "SyncSymbols" ) );
	const bool bAnnotate = FParse::Param( FCommandLine::Get(), TEXT( "Annotate" ) );
	const bool bSyncMicrosoftSymbols = FParse::Param( FCommandLine::Get(), TEXT( "SyncMicrosoftSymbols" ) );
	const bool bUseSCC = bSyncSymbols || bAnnotate || bSyncMicrosoftSymbols;

	if( bUseSCC )
	{
		InitSourceControl(false);
	}

	FWindowsPlatformStackWalkExt::InitStackWalking();

	if( FWindowsPlatformStackWalkExt::OpenDumpFile( &CrashInfo, InCrashDumpFilename ) )
	{
		// Get a list of modules, and the version of the executable
		FWindowsPlatformStackWalkExt::GetModuleList( &CrashInfo );

		// If a BuiltFromCL was specified, use that instead of the one parsed from the executable
		if (BuiltFromCL > 0)
		{
			CrashInfo.ChangelistBuiltFrom = BuiltFromCL;
		}

		if( CrashInfo.ChangelistBuiltFrom != -1 )
		{
			// CrashInfo now contains a changelist to lookup a label for
			if( bSyncSymbols )
			{
				CrashInfo.LabelName = RetrieveBuildLabel( -1, CrashInfo.ChangelistBuiltFrom );
				if( CrashInfo.LabelName.Len() > 0 )
				{
					// Sync all the modules, and their associated pdbs
					CrashInfo.Log( FString::Printf( TEXT( " ... found label '%s'; syncing all modules to this label" ), *CrashInfo.LabelName ) );
					SyncModules( &CrashInfo );
				}
			}

			// Initialise the symbol options
			FWindowsPlatformStackWalkExt::InitSymbols( &CrashInfo );

			// Set the symbol path based on the loaded modules
			FWindowsPlatformStackWalkExt::SetSymbolPathsFromModules( &CrashInfo );

			// Get all the info we should ever need about the modules
			FWindowsPlatformStackWalkExt::GetModuleInfoDetailed( &CrashInfo );

			// Get info about the system that created the minidump
			FWindowsPlatformStackWalkExt::GetSystemInfo( &CrashInfo );

			// Get all the thread info
			FWindowsPlatformStackWalkExt::GetThreadInfo( &CrashInfo );

			// Get exception info
			FWindowsPlatformStackWalkExt::GetExceptionInfo( &CrashInfo );

			// Get the callstacks for each thread
			FWindowsPlatformStackWalkExt::GetCallstacks( &CrashInfo );

			// Sync the source file where the crash occurred
			if( CrashInfo.SourceFile.Len() > 0 )
			{
				if( bSyncSymbols )
				{
					CrashInfo.Log( FString::Printf( TEXT( " ... using label '%s' to sync crash source file" ), *CrashInfo.LabelName ) );
					SyncSourceFile( &CrashInfo );
				}

				// Try to annotate the file if requested
				bool bAnnotationSuccessful = false;
				if( bAnnotate )
				{
					bAnnotationSuccessful = AddAnnotatedSourceToReport( &CrashInfo );
				}
				
				// If annotation is not requested, or failed, add the standard source context
				if( !bAnnotationSuccessful )
				{
					AddSourceToReport( &CrashInfo );
				}
			}
		}
		else
		{
			CrashInfo.Log( FString::Printf( TEXT( "CreateMinidumpDiagnosticReport: Failed to find executable image; possibly pathless module names?" ), *InCrashDumpFilename ) );
		}
	}
	else
	{
		CrashInfo.Log( FString::Printf( TEXT( "CreateMinidumpDiagnosticReport: Failed to open crash dump file: %s" ), *InCrashDumpFilename ) );
	}

	FWindowsPlatformStackWalkExt::ShutdownStackWalking();

	if( bUseSCC )
	{
		ShutdownSourceControl();
	}

	return bResult;
}

#include "HideWindowsPlatformTypes.h"
