// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashDebugHelperPrivatePCH.h"
#include "CrashDebugHelper.h"
#include "Database.h"
#include "ISourceControlModule.h"
#include "ISourceControlLabel.h"
#include "ISourceControlRevision.h"

#include "../../../../Launch/Resources/Version.h"

#ifndef MINIDUMPDIAGNOSTICS
	#define MINIDUMPDIAGNOSTICS	0
#endif

/** 
 * Global initialisation of this module
 */
bool ICrashDebugHelper::Init()
{
	bInitialized = true;

	// Look up the depot name
	// Try to use the command line param
	FString CmdLineBranchName;
	if ( FParse::Value(FCommandLine::Get(), TEXT("BranchName="), CmdLineBranchName) )
	{
		DepotName = FString::Printf( TEXT( "//depot/%s" ), *CmdLineBranchName );
	}
	// Try to use what is configured in ini
	else if (GConfig->GetString(TEXT("Engine.CrashDebugHelper"), TEXT("DepotName"), DepotName, GEngineIni) == true)
	{
		// Hack to get around ini files treating '//' as an inlined comment
		DepotName.ReplaceInline(TEXT("\\"), TEXT("/"));
	}
	// Default to BRANCH_NAME
	else
	{
		DepotName = FString::Printf( TEXT( "//depot/%s" ), TEXT( BRANCH_NAME ) );
	}

	// Try to get the BuiltFromCL from command line to use this instead of attempting to locate the CL in the minidump
	FString CmdLineBuiltFromCL;
	BuiltFromCL = -1;
	if (FParse::Value(FCommandLine::Get(), TEXT("BuiltFromCL="), CmdLineBuiltFromCL))
	{
		if ( !CmdLineBuiltFromCL.IsEmpty() )
		{
			BuiltFromCL = FCString::Atoi(*CmdLineBuiltFromCL);
		}
	}

	// Look up the source control search pattern. This pattern is used to identify a build label in the event that it was not already found in the database.
	if (GConfig->GetString(TEXT("Engine.CrashDebugHelper"), TEXT("SourceControlBuildLabelPattern"), SourceControlBuildLabelPattern, GEngineIni) == true)
	{
		// Using a build label pattern to search in P4 if we don't find the label in the database
	}

	// Look up the local symbol store - fail if not found
	if (GConfig->GetString(TEXT("Engine.CrashDebugHelper"), TEXT("LocalSymbolStore"), LocalSymbolStore, GEngineIni) == false)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("Failed to get LocalSymbolStore from ini file... crash handling disabled"));
		bInitialized = false;
	}

	// Look up the builder database name - fail if not found
	if (GConfig->GetString(TEXT("Engine.CrashDebugHelper"), TEXT("BuilderDatabase"), DatabaseName, GEngineIni) == false)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("Failed to get BuilderDatabase from ini file... crash handling disabled"));
		bInitialized = false;
	}

	// Look up the builder database catalog - fail if not found
	if (GConfig->GetString(TEXT("Engine.CrashDebugHelper"), TEXT("DatabaseCatalog"), DatabaseCatalog, GEngineIni) == false)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("Failed to get builder DatabaseCatalog from ini file... crash handling disabled"));
		bInitialized = false;
	}

	return bInitialized;
}

/** 
 * Initialise the source control interface, and ensure we have a valid connection
 */
bool ICrashDebugHelper::InitSourceControl(bool bShowLogin)
{
	// Ensure we are in a valid state to sync
	if (bInitialized == false)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("InitSourceControl: CrashDebugHelper is not initialized properly."));
		return false;
	}

	// Initialize the source control if it hasn't already been
	if( !ISourceControlModule::Get().IsEnabled() || !ISourceControlModule::Get().GetProvider().IsAvailable() )
	{
		// make sure our provider is set to Perforce
		ISourceControlModule::Get().SetProvider("Perforce");

		// Attempt to load in a source control module
		ISourceControlModule::Get().GetProvider().Init();
#if !MINIDUMPDIAGNOSTICS
		if ((ISourceControlModule::Get().GetProvider().IsAvailable() == false) || bShowLogin)
		{
			// Unable to connect? Prompt the user for login information
			ISourceControlModule::Get().ShowLoginDialog(FSourceControlLoginClosed(), ELoginWindowMode::Modeless, EOnLoginWindowStartup::PreserveProvider);
		}
#endif
		// If it's still disabled, none was found, so exit
		if( !ISourceControlModule::Get().IsEnabled() || !ISourceControlModule::Get().GetProvider().IsAvailable() )
		{
			UE_LOG(LogCrashDebugHelper, Warning, TEXT("InitSourceControl: Source control unavailable or disabled."));
			return false;
		}
	}

	return true;
}

/** 
 * Shutdown the connection to source control
 */
void ICrashDebugHelper::ShutdownSourceControl()
{
	ISourceControlModule::Get().GetProvider().Close();
}

/** 
 * Sync the branch root relative file names to the requested label
 */
bool ICrashDebugHelper::SyncModules( FCrashInfo* CrashInfo )
{
	if( CrashInfo->LabelName.Len() <= 0 )
	{
		UE_LOG( LogCrashDebugHelper, Warning, TEXT( "SyncModules: Invalid Label parameter." ) );
		return false;
	}

	// Check source control
	if( !ISourceControlModule::Get().IsEnabled() )
	{
		return false;
	}

	// Sync all the dll, exes, and related symbol files
	TArray< TSharedRef<ISourceControlLabel> > Labels = ISourceControlModule::Get().GetProvider().GetLabels( CrashInfo->LabelName );
	if(Labels.Num() > 0)
	{
		// Sync every module from every label. If the same modules appear in every label, this will fail.
		for (auto LabelIt = Labels.CreateConstIterator(); LabelIt; ++LabelIt)
		{
			TSharedRef<ISourceControlLabel> Label = *LabelIt;
			UE_LOG(LogCrashDebugHelper, Log, TEXT(" Syncing modules with label '%s'."), *Label->GetName());

			TArray<FString> FilesToSync;
			for( int32 ModuleNameIndex = 0; ModuleNameIndex < CrashInfo->ModuleNames.Num(); ModuleNameIndex++ )
			{
				FString DepotPath = FString::Printf( TEXT( "%s/%s" ), *DepotName, *CrashInfo->ModuleNames[ModuleNameIndex] );

				{
					if( Label->Sync(DepotPath) )
					{
						UE_LOG( LogCrashDebugHelper, Warning, TEXT( " ... synced binary '%s'."), *DepotPath );
					}

					FString PDBName = DepotPath.Replace( TEXT( ".dll" ), TEXT( ".pdb" ) ).Replace( TEXT( ".exe" ), TEXT( ".pdb" ) );
					if( Label->Sync(PDBName) )
					{
						UE_LOG( LogCrashDebugHelper, Warning, TEXT( " ... synced symbol '%s'."), *PDBName );
					}

					//@TODO: ROCKETHACK: Adding additional Installed and Symbol paths - revisit when builds are made by the builder...
					//@TODO: MAC: Excluding labels for Mac since we are only syncing windows binaries here...
					if ( !Label->GetName().Contains(TEXT("Mac")) )
					{
						DepotPath = FString::Printf( TEXT( "%s/Rocket/Installed/Windows/%s" ), *DepotName, *CrashInfo->ModuleNames[ModuleNameIndex] );
						if( Label->Sync(DepotPath) )
						{
							UE_LOG( LogCrashDebugHelper, Warning, TEXT( " ... synced binary '%s'."), *DepotPath );
						}

						DepotPath = FString::Printf( TEXT( "%s/Rocket/Symbols/%s" ), *DepotName, *CrashInfo->ModuleNames[ModuleNameIndex] );
						PDBName = DepotPath.Replace( TEXT( ".dll" ), TEXT( ".pdb" ) ).Replace( TEXT( ".exe" ), TEXT( ".pdb" ) );
						if( Label->Sync(PDBName) )
						{
							UE_LOG( LogCrashDebugHelper, Warning, TEXT( " ... synced symbol '%s'."), *PDBName );
						}

						DepotPath = FString::Printf( TEXT( "%s/Rocket/LauncherInstalled/Windows/Launcher/%s" ), *DepotName, *CrashInfo->ModuleNames[ModuleNameIndex] );
						if( Label->Sync(DepotPath) )
						{
							UE_LOG( LogCrashDebugHelper, Warning, TEXT( " ... synced binary '%s'."), *DepotPath );
						}

						DepotPath = FString::Printf( TEXT( "%s/Rocket/LauncherSymbols/Windows/Launcher/%s" ), *DepotName, *CrashInfo->ModuleNames[ModuleNameIndex] );
						PDBName = DepotPath.Replace( TEXT( ".dll" ), TEXT( ".pdb" ) ).Replace( TEXT( ".exe" ), TEXT( ".pdb" ) );
						if( Label->Sync(PDBName) )
						{
							UE_LOG( LogCrashDebugHelper, Warning, TEXT( " ... synced symbol '%s'."), *PDBName );
						}
					}
				}
			}
		}
	}
	else
	{
		UE_LOG( LogCrashDebugHelper, Error, TEXT( "Could not find label '%s'."), *CrashInfo->LabelName );
	}

	return true;
}

/** 
 * Sync a single source file to the requested label
 */
bool ICrashDebugHelper::SyncSourceFile( FCrashInfo* CrashInfo )
{
	if (CrashInfo->LabelName.Len() <= 0)
	{
		UE_LOG( LogCrashDebugHelper, Warning, TEXT( "SyncSourceFile: Invalid Label parameter." ) );
		return false;
	}

	// Check source control
	if( !ISourceControlModule::Get().IsEnabled() )
	{
		return false;
	}

	// Sync all the dll, exes, and related symbol files
	FString DepotPath = FString::Printf( TEXT( "%s/%s" ), *DepotName, *CrashInfo->SourceFile );
	TArray< TSharedRef<ISourceControlLabel> > Labels = ISourceControlModule::Get().GetProvider().GetLabels( CrashInfo->LabelName );
	if(Labels.Num() > 0)
	{
		TSharedRef<ISourceControlLabel> Label = Labels[0];
		if( Label->Sync( DepotPath ) )
		{
			UE_LOG( LogCrashDebugHelper, Warning, TEXT( " ... synced source file '%s'."), *DepotPath );
		}
	}
	else
	{
		UE_LOG( LogCrashDebugHelper, Error, TEXT( "Could not find label '%s'."), *CrashInfo->LabelName );
	}

	return true;
}

/**
 *	Load the given ANSI text file to an array of strings - one FString per line of the file.
 *	Intended for use in simple text parsing actions
 *
 *	@param	InFilename			The text file to read, full path
 *	@param	OutStrings			The array of FStrings to fill in
 *
 *	@return	bool				true if successful, false if not
 */
bool ICrashDebugHelper::ReadSourceFile( const TCHAR* InFilename, TArray<FString>& OutStrings )
{
	FString Line;
	if( FFileHelper::LoadFileToString( Line, InFilename ) )
	{
		Line = Line.Replace( TEXT( "\r" ), TEXT( "" ) );
		Line.ParseIntoArray( &OutStrings, TEXT( "\n" ), false );
		
		return true;
	}
	else
	{
		UE_LOG( LogCrashDebugHelper, Warning, TEXT( "Failed to open source file %s" ), InFilename );
		return false;
	}
}

/** 
 * Add adjacent lines of the source file the crash occurred in the crash report
 */
void ICrashDebugHelper::AddSourceToReport( FCrashInfo* CrashInfo )
{
	if( CrashInfo->SourceFile.Len() > 0 && CrashInfo->SourceLineNumber != 0 )
	{
		TArray<FString> Lines;
		FString FullPath = FString( TEXT( "../../../" ) ) + CrashInfo->SourceFile;
		ReadSourceFile( *FullPath, Lines );

		uint64 MinLine = FMath::Clamp( CrashInfo->SourceLineNumber - 15, ( uint64 )1, ( uint64 )Lines.Num() );
		uint64 MaxLine = FMath::Clamp( CrashInfo->SourceLineNumber + 15, ( uint64 )1, ( uint64 )Lines.Num() );

		for( int32 Line = MinLine; Line < MaxLine; Line++ )
		{
			if( Line == CrashInfo->SourceLineNumber - 1 )
			{
				CrashInfo->SourceContext.Add( FString( TEXT( "*****" ) ) + Lines[Line] );
			}
			else
			{
				CrashInfo->SourceContext.Add( FString( TEXT( "     " ) ) + Lines[Line] );
			}
		}
	}
}

/** 
 * Add source control annotated adjacent lines of the source file the crash occurred in the crash report
 */
bool ICrashDebugHelper::AddAnnotatedSourceToReport( FCrashInfo* CrashInfo )
{
	// Make sure we have a source file to interrogate
	if( CrashInfo->SourceFile.Len() > 0 && CrashInfo->SourceLineNumber != 0 )
	{
		// Check source control
		if( !ISourceControlModule::Get().IsEnabled() )
		{
			return false;
		}

		// Ask source control to annotate the file for us
		FString DepotPath = FString::Printf( TEXT( "%s/%s" ), *DepotName, *CrashInfo->SourceFile );

		TArray<FAnnotationLine> Lines;
		SourceControlHelpers::AnnotateFile( ISourceControlModule::Get().GetProvider(), CrashInfo->LabelName, DepotPath, Lines );

		uint64 MinLine = FMath::Clamp( CrashInfo->SourceLineNumber - 15, ( uint64 )1, ( uint64 )Lines.Num() );
		uint64 MaxLine = FMath::Clamp( CrashInfo->SourceLineNumber + 15, ( uint64 )1, ( uint64 )Lines.Num() );

		// Display a source context in the report, and decorate each line with the last editor of the line
		for( int32 Line = MinLine; Line < MaxLine; Line++ )
		{			
			if( Line == CrashInfo->SourceLineNumber )
			{
				CrashInfo->SourceContext.Add( FString::Printf( TEXT( "*****%20s: %s" ), *Lines[Line].UserName, *Lines[Line].Line ) );
			}
			else
			{
				CrashInfo->SourceContext.Add( FString::Printf( TEXT( "     %20s: %s" ), *Lines[Line].UserName, *Lines[Line].Line ) );
			}
		}
		return true;
	}

	return false;
}

/**
 * Add a line to the report
 */
void FCrashInfo::Log( FString Line )
{
	UE_LOG( LogCrashDebugHelper, Warning, TEXT("%s"), *Line );
	Report += Line + LINE_TERMINATOR;
}

/** 
 * Convert the processor architecture to a human readable string
 */
const TCHAR* FCrashInfo::GetProcessorArchitecture( EProcessorArchitecture PA )
{
	switch( PA )
	{
	case PA_X86:
		return TEXT( "x86" );
	case PA_X64:
		return TEXT( "x64" );
	case PA_ARM:
		return TEXT( "ARM" );
	}

	return TEXT( "Unknown" );
}

/** 
 * Calculate the byte size of a UTF-8 string
 */
int64 FCrashInfo::StringSize( const ANSICHAR* Line )
{
	int64 Size = 0;
	if( Line != NULL )
	{
		while( *Line++ != 0 )
		{
			Size++;
		}
	}
	return Size;
}

/** 
 * Write a line of UTF-8 to a file
 */
void FCrashInfo::WriteLine( FArchive* ReportFile, const ANSICHAR* Line )
{
	if( Line != NULL )
	{
		int64 StringBytes = StringSize( Line );
		ReportFile->Serialize( ( void* )Line, StringBytes );
	}

	ReportFile->Serialize( TCHAR_TO_UTF8( LINE_TERMINATOR ), 2 );
}

/** 
 * Write all the data mined from the minidump to a text file
 */
void FCrashInfo::GenerateReport( const FString& DiagnosticsPath )
{
	FArchive* ReportFile = IFileManager::Get().CreateFileWriter( *DiagnosticsPath );
	if( ReportFile != NULL )
	{
		FString Line;

		WriteLine( ReportFile, TCHAR_TO_UTF8( TEXT( "Generating report for minidump" ) ) );
		WriteLine( ReportFile );

		if( Modules.Num() > 0 )
		{
			Line = FString::Printf( TEXT( "Application version %d.%d.%d.%d" ), Modules[0].Major, Modules[0].Minor, Modules[0].Build, Modules[0].Revision );
			WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		}

		Line = FString::Printf( TEXT( " ... built from changelist %d" ), ChangelistBuiltFrom );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		if( LabelName.Len() > 0 )
		{
			Line = FString::Printf( TEXT( " ... based on label %s" ), *LabelName );
			WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		}
		WriteLine( ReportFile );

		Line = FString::Printf( TEXT( "OS version %d.%d.%d.%d" ), SystemInfo.OSMajor, SystemInfo.OSMinor, SystemInfo.OSBuild, SystemInfo.OSRevision );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );

		Line = FString::Printf( TEXT( "Running %d %s processors" ), SystemInfo.ProcessorCount, GetProcessorArchitecture( SystemInfo.ProcessorArchitecture ) );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );

		Line = FString::Printf( TEXT( "Exception was \"%s\"" ), *Exception.ExceptionString );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		WriteLine( ReportFile );

		Line = FString::Printf( TEXT( "Source context from \"%s\"" ), *SourceFile );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		WriteLine( ReportFile );

		Line = FString::Printf( TEXT( "<SOURCE START>" ) );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		for( int32 LineIndex = 0; LineIndex < SourceContext.Num(); LineIndex++ )
		{
			Line = FString::Printf( TEXT( "%s" ), *SourceContext[LineIndex] );
			WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		}
		Line = FString::Printf( TEXT( "<SOURCE END>" ) );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		WriteLine( ReportFile );

		Line = FString::Printf( TEXT( "<CALLSTACK START>" ) );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );

		for( int32 StackIndex = 0; StackIndex < Exception.CallStackString.Num(); StackIndex++ )
		{
			Line = FString::Printf( TEXT( "%s" ), *Exception.CallStackString[StackIndex] );
			WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		}

		Line = FString::Printf( TEXT( "<CALLSTACK END>" ) );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
		WriteLine( ReportFile );

		Line = FString::Printf( TEXT( "%d loaded modules" ), Modules.Num() );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );

		for( int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++ )
		{
			FCrashModuleInfo& Module = Modules[ModuleIndex];

			FString ModuleDirectory = FPaths::GetPath(Module.Name);
			FString ModuleName = FPaths::GetBaseFilename( Module.Name, true ) + FPaths::GetExtension( Module.Name, true );

			FString ModuleDetail = FString::Printf( TEXT( "%40s" ), *ModuleName );
			FString Version = FString::Printf( TEXT( " (%d.%d.%d.%d)" ), Module.Major, Module.Minor, Module.Build, Module.Revision );
			ModuleDetail += FString::Printf( TEXT( " %22s" ), *Version );
			ModuleDetail += FString::Printf( TEXT( " 0x%016x 0x%08x" ), Module.BaseOfImage, Module.SizeOfImage );
			ModuleDetail += FString::Printf( TEXT( " %s" ), *ModuleDirectory );

			WriteLine( ReportFile, TCHAR_TO_UTF8( *ModuleDetail ) );
		}

		WriteLine( ReportFile );

		// Write out the processor debugging log
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Report ) );

		Line = FString::Printf( TEXT( "Report end!" ) );
		WriteLine( ReportFile, TCHAR_TO_UTF8( *Line )  );

		ReportFile->Close();
		delete ReportFile;
	}
}

bool ICrashDebugHelper::SyncRequiredFilesForDebuggingFromLabel(const FString& InLabel, const FString& InPlatform)
{
	// Ensure we are in a valid state to sync
	if (bInitialized == false)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("SyncRequiredFilesForDebuggingFromLabel: CrashDebugHelper is not initialized properly."));
		return false;
	}

	if (InLabel.Len() <= 0)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("SyncRequiredFilesForDebuggingFromLabel: Invalid Label parameter."));
		return false;
	}

	if (InPlatform.Len() <= 0)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("SyncRequiredFilesForDebuggingFromLabel: Invalid Platform parameter."));
		return false;
	}

	bool bSyncFiles = true;

	// We have a valid label... 
	// This command will get the list of all Win64 pdb files under engine at the given label
	//     p4 files //depot/UE4/Engine/Binaries/Win64/...pdb...@UE4_[2012-10-24_04.00]
	// This command will get the list of all Win64 pdb files under game folders at the given label
	//     p4 files //depot/UE4/...Game/Binaries/Win64/...pdb...@UE4_[2012-10-24_04.00]
	FString EngineRoot = FString::Printf(TEXT("%s/Engine/Binaries/%s/"), *DepotName, *InPlatform);
	FString EnginePdbFiles = EngineRoot + TEXT("...pdb...");
	FString EngineExeFiles = EngineRoot + TEXT("...exe...");
	FString EngineDllFiles = EngineRoot + TEXT("...dll...");

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	TSharedPtr<ISourceControlLabel> Label = SourceControlProvider.GetLabel(InLabel);
	if(Label.IsValid())
	{
		TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> > EnginePdbList;
		TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> > EngineExeList;
		TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> > EngineDllList;

		bool bGotEngineFiles = true;
		bGotEngineFiles |= Label->GetFileRevisions(EnginePdbFiles, EnginePdbList);
		bGotEngineFiles |= Label->GetFileRevisions(EngineExeFiles, EngineExeList);
		bGotEngineFiles |= Label->GetFileRevisions(EngineDllFiles, EngineDllList);

		FString GameRoot = FString::Printf(TEXT("%s/...Game/Binaries/%s/"), *DepotName, *InPlatform);
		FString GamePdbFiles = GameRoot + TEXT("...pdb...");
		FString GameExeFiles = GameRoot + TEXT("...exe...");
		FString GameDllFiles = GameRoot + TEXT("...dll...");
		TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> > GamePdbList;
		TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> > GameExeList;
		TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> > GameDllList;

		bool bGotGameFiles = true;
		bGotGameFiles |= Label->GetFileRevisions(GamePdbFiles, GamePdbList);
		bGotGameFiles |= Label->GetFileRevisions(GameExeFiles, GameExeList);
		bGotGameFiles |= Label->GetFileRevisions(GameDllFiles, GameDllList);

		if (bGotEngineFiles == true)
		{
			TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> > CopyFileList;
			CopyFileList += EnginePdbList;
			CopyFileList += EngineExeList;
			CopyFileList += EngineDllList;
			CopyFileList += GamePdbList;
			CopyFileList += GameExeList;
			CopyFileList += GameDllList;

			int32 CopyCount = 0;

			// Copy all the files retrieved
			for (int32 FileIdx = 0; FileIdx < CopyFileList.Num(); FileIdx++)
			{
				TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> Revision = CopyFileList[FileIdx];

				// Copy the files to a flat directory.
				// This will have problems if there are any files named the same!!!!
				FString LocalStoreFolder = FString::Printf(TEXT("%s/%s/"), *LocalSymbolStore, *InPlatform);
				FString CopyFilename = LocalStoreFolder / FPaths::GetCleanFilename(Revision->GetFilename());

//				UE_LOG(LogCrashDebugHelper, Display, TEXT("Copying engine file: %s to %s"), *Filename, *CopyFilename);

				if (Revision->Get(CopyFilename) == true)
				{
					CopyCount++;
				}
			}

			//@todo. Should we verify EVERY file was copied?
			return (CopyCount > 0);
		}
	}
	else
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("SyncRequiredFiles: Invalid label specified: %s"), *InLabel);
	}

	return false;
}

bool ICrashDebugHelper::SyncRequiredFilesForDebuggingFromChangelist(int32 InChangelistNumber, const FString& InPlatform)
{
	//@todo. Allow for syncing a changelist directly?
	// Not really useful as the source indexed pdbs will be tied to labeled builds.
	// For now, we will not support this

	FString BuildLabel = RetrieveBuildLabel(-1, InChangelistNumber);
	if (BuildLabel.Len() > 0)
	{
		return SyncRequiredFilesForDebuggingFromLabel(BuildLabel, InPlatform);
	}

	UE_LOG(LogCrashDebugHelper, Warning, 
		TEXT("SyncRequiredFilesForDebuggingFromChangelist: Failed to find label for changelist %d"), InChangelistNumber);
	return false;
}

/**
 *	Retrieve the build label for the given engine version or changelist number.
 *
 *	@param	InEngineVersion		The engine version to retrieve the label for. If -1, then will use InChangelistNumber
 *	@param	InChangelistNumber	The changelist number to retrieve the label for
 *	@return	FString				The label if successful, empty if it wasn't found or another error
 */
FString ICrashDebugHelper::RetrieveBuildLabel(int32 InEngineVersion, int32 InChangelistNumber)
{
	FString FoundLabelString;

	if ((DatabaseName.Len() == 0) || (DatabaseCatalog.Len() == 0))
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("RetrieveBuildLabel: Invalid databasename and/or databasecatalog set."));
		return FoundLabelString;
	}

	if ((InEngineVersion < 0) && (InChangelistNumber < 0))
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("RetrieveBuildLabel: Invalid parameters."));
		return FoundLabelString;
	}

	// Open a database connection
	// Create the connection object; needs to be deleted via "delete".
	FDataBaseConnection* Connection = FDataBaseConnection::CreateObject();
	check(Connection);

	// Create the connection string with Windows Authentication as the way to handle permissions/ login/ security.
	FString ConnectionString = FString::Printf(
		TEXT("Provider=sqloledb;Data Source=%s;Initial Catalog=%s;Trusted_Connection=Yes;"), *DatabaseName, *DatabaseCatalog);

	// Try to open connection to DB - this is a synchronous operation.
	if (Connection->Open(*ConnectionString, NULL, NULL))
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("RetrieveBuildLabel: Connection to %s.%s succeeded"), *DatabaseName, *DatabaseCatalog);

		// Setup the query command
		FString LabelRequestCmd = TEXT("SELECT Label FROM Versioning WHERE ");
		if (InEngineVersion >= 0)
		{
			LabelRequestCmd += FString::Printf(TEXT("EngineVersion = %d"), InEngineVersion);
		}
		else
		{
			check(InChangelistNumber >= 0);
			LabelRequestCmd += FString::Printf(TEXT("Changelist = %d"), InChangelistNumber);
		}

		// Run the query
		FDataBaseRecordSet* NewRecordSet = NULL;
		if (Connection->Execute(*LabelRequestCmd, NewRecordSet) == false)
		{
			UE_LOG(LogCrashDebugHelper, Warning, TEXT("RetrieveBuildLabel: Failed to exec SQLCommand for..."));
			UE_LOG(LogCrashDebugHelper, Warning, TEXT("\t%s"), *LabelRequestCmd);
		}
		else
		{
			if ((NewRecordSet == NULL) || (NewRecordSet->GetRecordCount() == 0))
			{
				if (InEngineVersion >= 0)
				{
					UE_LOG(LogCrashDebugHelper, Warning, TEXT("RetrieveBuildLabel: No results for engine version %d"), InEngineVersion);
				}
				else
				{
					UE_LOG(LogCrashDebugHelper, Warning, TEXT("RetrieveBuildLabel: No results for changelist %d"), InChangelistNumber);
				}
			}
			else
			{
				for (FDataBaseRecordSet::TIterator It(NewRecordSet); It; ++It)
				{
					FoundLabelString = It->GetString(TEXT("Label"));
					FoundLabelString.TrimTrailing();
				}
			}
		}

		Connection->Close();
	}
	else
	{
		// Connection failed :(
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("RetrieveBuildLabel: Connection to %s.%s failed"), *DatabaseName, *DatabaseCatalog);
	}

	// delete
	delete Connection;
	Connection = NULL;

	// If we failed to find the label, try to find the label directly in source control by using the pattern supplied via ini
	if ( FoundLabelString.IsEmpty() && InChangelistNumber >= 0 && !SourceControlBuildLabelPattern.IsEmpty() )
	{
		const FString ChangelistString = FString::Printf(TEXT("%d"), InChangelistNumber);
		const FString TestLabel = SourceControlBuildLabelPattern.Replace(TEXT("%CHANGELISTNUMBER%"), *ChangelistString, ESearchCase::CaseSensitive );
		TArray< TSharedRef<ISourceControlLabel> > Labels = ISourceControlModule::Get().GetProvider().GetLabels( TestLabel );
		if ( Labels.Num() > 0 )
		{
			if (Labels.Num() == 1)
			{
				FoundLabelString = Labels[0]->GetName();
				UE_LOG(LogCrashDebugHelper, Log, TEXT("RetrieveBuildLabel: Failed to find build label in database %s.%s, but found label %s matching pattern %s in source control."), *DatabaseName, *DatabaseCatalog, *FoundLabelString, *TestLabel);
			}
			else
			{
				FoundLabelString = TestLabel;
				UE_LOG(LogCrashDebugHelper, Log, TEXT("RetrieveBuildLabel: Failed to find build label in database %s.%s, but found %d labels matching pattern %s in source control."), *DatabaseName, *DatabaseCatalog, Labels.Num(), *TestLabel);
				for (int32 LabelIdx = 0; LabelIdx < Labels.Num(); ++LabelIdx)
				{
					UE_LOG(LogCrashDebugHelper, Log, TEXT("RetrieveBuildLabel:     Label: %s"), *Labels[LabelIdx]->GetName());
				}
			}
		}
	}

	return FoundLabelString;
}

