// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	WindowsPlatformProcess.h: Windows platform Process functions
==============================================================================================*/

#pragma once

/** Windows implementation of the process handle. */
struct FProcHandle : public TProcHandle<HANDLE, nullptr>
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

	FORCEINLINE bool Close()
	{
		if( IsValid() )
		{
			::CloseHandle( Handle );
			Reset();
			return true;
		}
		else
		{
			return false;
		}
	}
};

/**
* Windows implementation of the Process OS functions
**/
struct CORE_API FWindowsPlatformProcess : public FGenericPlatformProcess
{
	// Begin FGenericPlatformProcess interface

	static void* GetDllHandle( const TCHAR* Filename );
	static void FreeDllHandle( void* DllHandle );
	static void* GetDllExport( void* DllHandle, const TCHAR* ProcName );
	static FBinaryFileVersion GetBinaryFileVersion( const TCHAR* Filename );
	static void PushDllDirectory(const TCHAR* Directory);
	static void PopDllDirectory(const TCHAR* Directory);
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
	static const TCHAR* GetBinariesSubdirectory();
	static void LaunchURL( const TCHAR* URL, const TCHAR* Parms, FString* Error );
	static FProcHandle CreateProc( const TCHAR* URL, const TCHAR* Parms, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden, uint32* OutProcessID, int32 PriorityModifier, const TCHAR* OptionalWorkingDirectory, void* PipeWrite );
	static bool IsProcRunning( FProcHandle & ProcessHandle );
	static void WaitForProc( FProcHandle & ProcessHandle );
	static void TerminateProc( FProcHandle & ProcessHandle, bool KillTree = false );
	static bool GetProcReturnCode( FProcHandle & ProcHandle, int32* ReturnCode );
	static bool IsApplicationRunning( uint32 ProcessId );
	static bool IsApplicationRunning( const TCHAR* ProcName );
	static bool IsThisApplicationForeground();
	static void ExecProcess( const TCHAR* URL, const TCHAR* Params, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr );
	static void LaunchFileInDefaultExternalApplication( const TCHAR* FileName, const TCHAR* Parms = NULL );
	static void ExploreFolder( const TCHAR* FilePath );
	static bool ResolveNetworkPath( FString InUNCPath, FString& OutPath ); 
	static void Sleep( float Seconds );
	static void SleepInfinite();
	static class FEvent* CreateSynchEvent(bool bIsManualReset = false);
	static class FRunnableThread* CreateRunnableThread();
	static void ClosePipe( void* ReadPipe, void* WritePipe );
	static bool CreatePipe( void*& ReadPipe, void*& WritePipe );
	static FString ReadPipe( void* ReadPipe );

	// End FGenericPlatformProcess interface

protected:

	/**
	 * Reads from a collection of anonymous pipes.
	 *
	 * @param OutStrings - Will hold the read data.
	 * @param InPipes - The pipes to read from.
	 * @param PipeCount - The number of pipes.
	 */
	static void ReadFromPipes(FString* OutStrings[], HANDLE InPipes[], int32 PipeCount);


private:

	// Since Windows can only have one directory at a time, this stack is used to reset the previous DllDirectory
	static TArray<FString> DllDirectoryStack;
};


typedef FWindowsPlatformProcess FPlatformProcess;

#include "WindowsCriticalSection.h"
typedef FWindowsCriticalSection FCriticalSection;
