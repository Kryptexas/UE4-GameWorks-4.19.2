// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	MacPlatformProcess.h: Mac platform Process functions
==============================================================================================*/

#pragma once


/** Mac implementation of the process handle. */
struct FProcHandle : public TProcHandle<void*, nullptr>
{
public:
	/** Default constructor. */
	FORCEINLINE FProcHandle()
		: TProcHandle()
	{}

	/** Initialization constructor. */
	FORCEINLINE explicit FProcHandle( HandleType Other )
		: TProcHandle( Other )
	{}
};

/**
* Mac implementation of the Process OS functions
**/
struct CORE_API FMacPlatformProcess : public FGenericPlatformProcess
{
	static void* GetDllHandle( const TCHAR* Filename );
	static void FreeDllHandle( void* DllHandle );
	static void* GetDllExport( void* DllHandle, const TCHAR* ProcName );
	static FBinaryFileVersion GetBinaryFileVersion( const TCHAR* Filename );
	static void CleanFileCache();
	static uint32 GetCurrentProcessId();
	static const TCHAR* BaseDir();
	static const TCHAR* UserDir();
	static const TCHAR* UserSettingsDir();
	static const TCHAR* ApplicationSettingsDir();
	static const TCHAR* ComputerName();
	static const TCHAR* UserName(bool bOnlyAlphaNumeric = true);
	static void SetCurrentWorkingDirectoryToBaseDir();
	static const TCHAR* ExecutableName(bool bRemoveExtension = true);
	static FString GenerateApplicationPath( const FString& AppName, EBuildConfigurations::Type BuildConfiguration);
	static const TCHAR* GetModuleExtension();
	static const TCHAR* GetBinariesSubdirectory();
	static const FString GetModulesDirectory();
	static void LaunchURL( const TCHAR* URL, const TCHAR* Parms, FString* Error );
	static FProcHandle CreateProc( const TCHAR* URL, const TCHAR* Parms, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden, uint32* OutProcessID, int32 PriorityModifier, const TCHAR* OptionalWorkingDirectory, void* PipeWrite );
	static bool IsProcRunning( FProcHandle & ProcessHandle );
	static void WaitForProc( FProcHandle & ProcessHandle );
	static void TerminateProc( FProcHandle & ProcessHandle, bool KillTree = false );
	static bool GetProcReturnCode( FProcHandle & ProcHandle, int32* ReturnCode );
    static bool IsApplicationRunning( const TCHAR* ProcName );
	static bool IsApplicationRunning( uint32 ProcessId );
	static bool IsThisApplicationForeground();
	static void LaunchFileInDefaultExternalApplication( const TCHAR* FileName, const TCHAR* Parms = NULL );
	static void ExploreFolder( const TCHAR* FilePath );
    static FRunnableThread* CreateRunnableThread();
	static void ClosePipe( void* ReadPipe, void* WritePipe );
	static bool CreatePipe( void*& ReadPipe, void*& WritePipe );
	static FString ReadPipe( void* ReadPipe );
	static void ExecProcess( const TCHAR* URL, const TCHAR* Params, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr );
};

typedef FMacPlatformProcess FPlatformProcess;

