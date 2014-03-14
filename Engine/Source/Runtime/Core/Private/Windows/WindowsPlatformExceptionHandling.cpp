// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsPlatformExceptionHandling.cpp: Exception handling for functions that want to create crash dumps.
=============================================================================*/

#include "CorePrivate.h"
#include "ExceptionHandling.h"
#include "../../Launch/Resources/Version.h"

#include "AllowWindowsPlatformTypes.h"

	#include <strsafe.h>
#if WINVER > 0x502	// Windows Error Reporting is not supported on Windows XP
	#include <werapi.h>
	#pragma comment( lib, "wer.lib" )
#endif	// WINVER
	#include <dbghelp.h>
	#include <Shlwapi.h>

#pragma comment( lib, "version.lib" )
#pragma comment( lib, "Shlwapi.lib" )

static bool GAlreadyCreatedMinidump = false;

/** 
 * Get a string description of the mode the engine was running in when it crashed
 */
static const TCHAR* GetEngineMode()
{
	if( IsRunningCommandlet() )
	{
		return TEXT( "Commandlet" );
	}
	else if( GIsEditor )
	{
		return TEXT( "Editor" );
	}
	else if( IsRunningDedicatedServer() )
	{
		return TEXT( "Server" );
	}

	return TEXT( "Game" );
}

#if WINVER > 0x502	// Windows Error Reporting is not supported on Windows XP

/** 
 * Get one line of text to describe the crash
 */
static void GetCrashDescription( WER_REPORT_INFORMATION& ReportInformation, const TCHAR* ErrorMessage )
{
	if( ErrorMessage != NULL )
	{
		StringCchCat( ReportInformation.wzDescription, ARRAYSIZE( ReportInformation.wzDescription ), ErrorMessage );
	}
	else
	{
		StringCchCopy( ReportInformation.wzDescription, ARRAYSIZE( ReportInformation.wzDescription ), TEXT( "The application crashed while running " ) );

		if( IsRunningCommandlet() )
		{
			StringCchCat( ReportInformation.wzDescription, ARRAYSIZE( ReportInformation.wzDescription ), TEXT( "a commandlet" ) );
		}
		else if( GIsEditor )
		{
			StringCchCat( ReportInformation.wzDescription, ARRAYSIZE( ReportInformation.wzDescription ), TEXT( "the editor" ) );
		}
		else if( GIsServer && !GIsClient )
		{
			StringCchCat( ReportInformation.wzDescription, ARRAYSIZE( ReportInformation.wzDescription ), TEXT( "a server" ) );
		}
		else
		{
			StringCchCat( ReportInformation.wzDescription, ARRAYSIZE( ReportInformation.wzDescription ), TEXT( "the game" ) );
		}
	}
}

#endif	// WINVER

/**
 * Get the 4 element version number of the module
 */
static void GetModuleVersion( TCHAR* ModuleName, TCHAR* StringBuffer, DWORD MaxSize )
{
	StringCchCopy( StringBuffer, MaxSize, TEXT( "0.0.0.0" ) );
	
	DWORD Handle = 0;
	DWORD InfoSize = GetFileVersionInfoSize( ModuleName, &Handle );
	if( InfoSize > 0 )
	{
		char* VersionInfo = new char[InfoSize];
		if( GetFileVersionInfo( ModuleName, 0, InfoSize, VersionInfo ) )
		{
			VS_FIXEDFILEINFO* FixedFileInfo;

			UINT InfoLength = 0;
			if( VerQueryValue( VersionInfo, TEXT( "\\" ), ( void** )&FixedFileInfo, &InfoLength ) )
			{
				StringCchPrintf( StringBuffer, MaxSize, TEXT( "%d.%d.%d.%d" ), 
					HIWORD( FixedFileInfo->dwProductVersionMS ), LOWORD( FixedFileInfo->dwProductVersionMS ), HIWORD( FixedFileInfo->dwProductVersionLS ), LOWORD( FixedFileInfo->dwProductVersionLS ) );
			}
		}

		delete VersionInfo;
	}
}


#if WINVER > 0x502	// Windows Error Reporting is not supported on Windows XP

/** 
 * Set the ten Windows Error Reporting parameters
 *
 * Parameters 0 through 7 are predefined for Windows
 * Parameters 8 and 9 are user defined
 */
static void SetReportParameters( HREPORT ReportHandle, EXCEPTION_POINTERS* ExceptionInfo )
{
	HRESULT Result;
	TCHAR StringBuffer[1024] = { 0 };
	TCHAR ModuleName[1024] = { 0 };

	// Set the parameters for the standard problem signature
	HMODULE ModuleHandle = GetModuleHandle( NULL );

	StringCchPrintf( StringBuffer, 1024, TEXT( "UE4-%s" ), FApp::GetGameName() );
	Result = WerReportSetParameter( ReportHandle, WER_P0, TEXT( "Application Name" ), StringBuffer );

	GetModuleFileName( ModuleHandle, ModuleName, 1024 );
	PathStripPath( ModuleName );
	GetModuleVersion( ModuleName, StringBuffer, 1024 );
	Result = WerReportSetParameter( ReportHandle, WER_P1, TEXT( "Application Version" ), StringBuffer );

	StringCchPrintf( StringBuffer, 1024, TEXT( "%08x" ), GetTimestampForLoadedLibrary( ModuleHandle ) );
	Result = WerReportSetParameter( ReportHandle, WER_P2, TEXT( "Application Timestamp" ), StringBuffer );

	HMODULE FaultModuleHandle = NULL;
	GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, ( LPCTSTR )ExceptionInfo->ExceptionRecord->ExceptionAddress, &FaultModuleHandle );

	GetModuleFileName( FaultModuleHandle, ModuleName, 1024 );
	PathStripPath( ModuleName );
	Result = WerReportSetParameter( ReportHandle, WER_P3, TEXT( "Fault Module Name" ), ModuleName );

	GetModuleVersion( ModuleName, StringBuffer, 1024 );
	Result = WerReportSetParameter( ReportHandle, WER_P4, TEXT( "Fault Module Version" ), StringBuffer );

	StringCchPrintf( StringBuffer, 1024, TEXT( "%08x" ), GetTimestampForLoadedLibrary( FaultModuleHandle ) );
	Result = WerReportSetParameter( ReportHandle, WER_P5, TEXT( "Fault Module Timestamp" ), StringBuffer );

	StringCchPrintf( StringBuffer, 1024, TEXT( "%08x" ), ExceptionInfo->ExceptionRecord->ExceptionCode );
	Result = WerReportSetParameter( ReportHandle, WER_P6, TEXT( "Exception Code" ), StringBuffer );

	INT_PTR ExceptionOffset = ( char* )( ExceptionInfo->ExceptionRecord->ExceptionAddress ) - ( char* )FaultModuleHandle;
	StringCchPrintf( StringBuffer, 1024, TEXT( "%p" ), ExceptionOffset );
	Result = WerReportSetParameter( ReportHandle, WER_P7, TEXT( "Exception Offset" ), StringBuffer );

	StringCchPrintf( StringBuffer, 1024, TEXT( "!%s!" ), FCommandLine::Get() );
	Result = WerReportSetParameter( ReportHandle, WER_P8, TEXT( "Commandline" ), StringBuffer );

	StringCchPrintf( StringBuffer, 1024, TEXT( "%s!%s!%s!%d" ), TEXT( BRANCH_NAME ), FPlatformProcess::BaseDir(), GetEngineMode(), BUILT_FROM_CHANGELIST );
	Result = WerReportSetParameter( ReportHandle, WER_P9, TEXT( "BranchBaseDir" ), StringBuffer );
}

/**
 * Add a minidump to the Windows Error Report
 *
 * Note this has to be a minidump (and not a microdump) for the dbgeng functions to work correctly
 */
static void AddMiniDump( HREPORT ReportHandle, EXCEPTION_POINTERS* ExceptionInfo )
{
#if 0
	// This doesn't work in x64
	WER_EXCEPTION_INFORMATION WerExceptionInfo = { ExceptionInfo, FALSE };
	Result = WerReportAddDump( ReportHandle, GetCurrentProcess(), GetCurrentThread(), WerDumpTypeMiniDump, &WerExceptionInfo, NULL, 0 );
#else
	FString MinidumpFileName = FString::Printf( TEXT( "%sDump%d.dmp" ), *FPaths::GameLogDir(), FDateTime::UtcNow().GetTicks() );
	HANDLE DumpHandle = CreateFileW( *MinidumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( DumpHandle != INVALID_HANDLE_VALUE )
	{
		MINIDUMP_EXCEPTION_INFORMATION DumpExceptionInfo = { 0 };
		DumpExceptionInfo.ThreadId = GetCurrentThreadId();
		DumpExceptionInfo.ExceptionPointers = ExceptionInfo;
		DumpExceptionInfo.ClientPointers = true;

		MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), DumpHandle, MiniDumpNormal, &DumpExceptionInfo, NULL, NULL );
		CloseHandle( DumpHandle );

		WerReportAddFile( ReportHandle, *MinidumpFileName, WerFileTypeMinidump, WER_FILE_ANONYMOUS_DATA ); 
	}
#endif
}

/** 
 * Add miscellaneous files to the report. Currently the log and the video file
 */
static void AddMiscFiles( HREPORT ReportHandle )
{
	FString LogFileName = FPaths::GameLogDir() / FApp::GetGameName() + TEXT( ".log" );
	WerReportAddFile( ReportHandle, *LogFileName, WerFileTypeOther, WER_FILE_ANONYMOUS_DATA ); 

	FString CrashVideoPath = FPaths::GameLogDir() / TEXT( "CrashVideo.avi" );
	WerReportAddFile( ReportHandle, *CrashVideoPath, WerFileTypeOther, WER_FILE_ANONYMOUS_DATA ); 
}

/**
 * Force WER queuing on or off
 *
 * This is required so that the crash report doesn't immediately get sent to Microsoft, and we can't intercept it
 */
static void ForceWERQueuing( bool bEnable )
{
	HKEY Key = 0;
	HRESULT KeyResult = RegOpenKeyEx( HKEY_CURRENT_USER, TEXT( "Software\\Microsoft\\Windows\\Windows Error Reporting" ), 0, KEY_WRITE, &Key );
	if( KeyResult == S_OK )
	{
		DWORD ForceQueue = ( DWORD )bEnable;
		KeyResult = RegSetValueExW( Key, TEXT( "ForceQueue" ), 0, REG_DWORD, ( const BYTE* )&ForceQueue, sizeof( DWORD ) );
	}
}


namespace
{
/**
 * Enum indicating whether to run the crash reporter UI
 */
enum class EErrorReportUI
{
	/** Ask the user for a description */
	ShowDialog,

	/** Silently uploaded the report */
	ReportInUnattendedMode	
};

/** 
 * Create a Windows Error Report, add the user log and video, and add it to the WER queue
 * Launch CrashReportUploader.exe to intercept the report and upload to our local site
 */
int32 NewReportCrash( EXCEPTION_POINTERS* ExceptionInfo, const TCHAR* ErrorMessage, EErrorReportUI ReportUI )
{
	HRESULT Result;
	HREPORT ReportHandle = NULL;
	WER_REPORT_INFORMATION ReportInformation = { sizeof( WER_REPORT_INFORMATION ), 0 };
	WER_SUBMIT_RESULT SubmitResult;
	
	// Flush out the log
#if 1
	GLog->Flush();
#else
	GError->HandleError();
#endif

	// Set the report to force queue
	ForceWERQueuing( true );

	// Construct the report details
	StringCchCopy( ReportInformation.wzConsentKey, ARRAYSIZE( ReportInformation.wzConsentKey ), TEXT( "" ) );
	StringCchCopy( ReportInformation.wzApplicationName, ARRAYSIZE( ReportInformation.wzApplicationName ), FApp::GetGameName() );
	StringCchCopy( ReportInformation.wzApplicationPath, ARRAYSIZE( ReportInformation.wzApplicationPath ), FPlatformProcess::BaseDir() );
	StringCchCat( ReportInformation.wzApplicationPath, ARRAYSIZE( ReportInformation.wzApplicationPath ), FPlatformProcess::ExecutableName() );
	StringCchCat( ReportInformation.wzApplicationPath, ARRAYSIZE( ReportInformation.wzApplicationPath ), TEXT( ".exe" ) );

	GetCrashDescription( ReportInformation, ErrorMessage );
	
	// Create a crash event report
	if( WerReportCreate( APPCRASH_EVENT, WerReportApplicationCrash, &ReportInformation, &ReportHandle ) == S_OK )
	{
		// Set the standard set of a crash parameters
		SetReportParameters( ReportHandle, ExceptionInfo );

		// Add a manually generated minidump
		AddMiniDump( ReportHandle, ExceptionInfo );

		// Add the log and video
		AddMiscFiles( ReportHandle );

		// Submit
		Result = WerReportSubmit( ReportHandle, WerConsentAlwaysPrompt, WER_SUBMIT_QUEUE | WER_SUBMIT_BYPASS_DATA_THROTTLING, &SubmitResult );

		// Cleanup
		Result = WerReportCloseHandle( ReportHandle );
	}

	// Build machines do not upload these automatically since it is not okay to have lingering processes after the build completes.
	if ( !GIsBuildMachine )
	{
		// When invoking CrashReportUploader, put the logs in our saved directory
		FString CrashReportUploaderArguments = FString::Printf( TEXT( "-LogFolder=%s" ), *FPaths::Combine(*FPaths::GameAgnosticSavedDir(), TEXT("CrashLogs")) );
	
		// Suppress the user input dialog if we're running in unattended mode
		if( FApp::IsUnattended() || ReportUI == EErrorReportUI::ReportInUnattendedMode )
		{
			CrashReportUploaderArguments += TEXT( " -Unattended" );
		}

		const TCHAR* CrashClientPath = GUseCrashReportClient ?
			TEXT("CrashReportClient.exe") :
			TEXT("..\\..\\..\\Engine\\Binaries\\DotNET\\CrashReportUploader.exe");

		// Passing !GUseCrashReportClient to 'hidden' and 'really-hidden' parameters
		FPlatformProcess::CreateProc(CrashClientPath, *CrashReportUploaderArguments, true, !GUseCrashReportClient, !GUseCrashReportClient, NULL, 0, NULL, NULL);
	}

	// Reset the setting after we've had our way with it
	ForceWERQueuing( false );

	// Let the system take back over
	return ( ErrorMessage != NULL ) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;
}

} // end anonymous namespace

#endif		// WINVER

/** 
 * Report an ensure to the crash reporting system
 */
void NewReportEnsure( const TCHAR* ErrorMessage )
{
#if WINVER > 0x502	// Windows Error Reporting is not supported on Windows XP
#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
	__try
#endif
	{
		FPlatformMisc::RaiseException( 1 );
	}
#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
	__except( NewReportCrash( GetExceptionInformation(), ErrorMessage, EErrorReportUI::ReportInUnattendedMode ) )
	{
	}
#endif
#endif	// WINVER
}

#include "HideWindowsPlatformTypes.h"

// Original code below

#include "AllowWindowsPlatformTypes.h"
	#include <ErrorRep.h>
	#include <DbgHelp.h>
#include "HideWindowsPlatformTypes.h"

#pragma comment(lib, "Faultrep.lib")

/** 
 * Creates an info string describing the given exception record.
 * See MSDN docs on EXCEPTION_RECORD.
 */
#include "AllowWindowsPlatformTypes.h"
void CreateExceptionInfoString(EXCEPTION_RECORD* ExceptionRecord)
{
	FString ErrorString = TEXT("Unhandled Exception: ");

#define HANDLE_CASE(x) case x: ErrorString += TEXT(#x); break;

	switch (ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		ErrorString += TEXT("EXCEPTION_ACCESS_VIOLATION ");
		if (ExceptionRecord->ExceptionInformation[0] == 0)
		{
			ErrorString += TEXT("reading address ");
		}
		else if (ExceptionRecord->ExceptionInformation[0] == 1)
		{
			ErrorString += TEXT("writing address ");
		}
		ErrorString += FString::Printf(TEXT("0x%08x"), (uint32)ExceptionRecord->ExceptionInformation[1]);
		break;
	HANDLE_CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
	HANDLE_CASE(EXCEPTION_DATATYPE_MISALIGNMENT)
	HANDLE_CASE(EXCEPTION_FLT_DENORMAL_OPERAND)
	HANDLE_CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO)
	HANDLE_CASE(EXCEPTION_FLT_INVALID_OPERATION)
	HANDLE_CASE(EXCEPTION_ILLEGAL_INSTRUCTION)
	HANDLE_CASE(EXCEPTION_INT_DIVIDE_BY_ZERO)
	HANDLE_CASE(EXCEPTION_PRIV_INSTRUCTION)
	HANDLE_CASE(EXCEPTION_STACK_OVERFLOW)
	default:
		ErrorString += FString::Printf(TEXT("0x%08x"), (uint32)ExceptionRecord->ExceptionCode);
	}

#if WITH_EDITORONLY_DATA
	FCString::Strncpy(GErrorExceptionDescription, *ErrorString, FMath::Min(ErrorString.Len() + 1, (int32)ARRAY_COUNT(GErrorExceptionDescription)));
#endif
#undef HANDLE_CASE
}
#include "HideWindowsPlatformTypes.h"

/** 
 * A null crash handler to suppress error report generation
 */
int32 NullReportCrash( LPEXCEPTION_POINTERS ExceptionInfo )
{
	return EXCEPTION_EXECUTE_HANDLER;
}

int32 ReportCrash( LPEXCEPTION_POINTERS ExceptionInfo )
{
	// Only create a minidump the first time this function is called.
	// (Can be called the first time from the RenderThread, then a second time from the MainThread.)
	if ( GAlreadyCreatedMinidump == false )
	{
		GAlreadyCreatedMinidump = true;

		FCoreDelegates::OnHandleSystemError.Broadcast();

#if WINVER > 0x502	// Windows Error Reporting is not supported on Windows XP
		// Loop in the new system while keeping the old until it's fully implemented
		NewReportCrash( ExceptionInfo, NULL, EErrorReportUI::ShowDialog );

		if (GUseCrashReportClient)
		{
			return EXCEPTION_EXECUTE_HANDLER;
		}
#endif		// WINVER

		// Try to create file for minidump.
		HANDLE FileHandle	= CreateFileW( MiniDumpFilenameW, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			
		// Write a minidump.
		if( FileHandle != INVALID_HANDLE_VALUE )
		{
			MINIDUMP_EXCEPTION_INFORMATION DumpExceptionInfo;

			DumpExceptionInfo.ThreadId			= GetCurrentThreadId();
			DumpExceptionInfo.ExceptionPointers	= ExceptionInfo;
			DumpExceptionInfo.ClientPointers	= false;

			MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), FileHandle, MiniDumpNormal, &DumpExceptionInfo, NULL, NULL );
			CloseHandle( FileHandle );
		}

		const SIZE_T StackTraceSize = 65535;
		ANSICHAR* StackTrace = (ANSICHAR*) FMemory::SystemMalloc( StackTraceSize );
		StackTrace[0] = 0;
		// Walk the stack and dump it to the allocated memory.
		FPlatformStackWalk::StackWalkAndDump( StackTrace, StackTraceSize, 0, ExceptionInfo->ContextRecord );
		FCString::Strncat( GErrorHist, ANSI_TO_TCHAR(StackTrace), ARRAY_COUNT(GErrorHist) - 1 );
		CreateExceptionInfoString(ExceptionInfo->ExceptionRecord);
		FMemory::SystemFree( StackTrace );

#if UE_BUILD_SHIPPING && WITH_EDITOR
			uint32 dwOpt = 0;
			EFaultRepRetVal repret = ReportFault( ExceptionInfo, dwOpt);
#endif
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

