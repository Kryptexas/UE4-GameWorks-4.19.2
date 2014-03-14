// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsPlatformProcess.cpp: Windows implementations of Process functions
=============================================================================*/

#include "CorePrivate.h"
#include "EngineVersion.h"

#include "AllowWindowsPlatformTypes.h"
	#include <shellapi.h>
	#include <shlobj.h>
	#include <LM.h>
	#include <tlhelp32.h>

	namespace ProcessConstants
	{
		uint32 WIN_STD_INPUT_HANDLE = STD_INPUT_HANDLE;
		uint32 WIN_STD_OUTPUT_HANDLE = STD_OUTPUT_HANDLE;
		uint32 WIN_ATTACH_PARENT_PROCESS = ATTACH_PARENT_PROCESS;		
		uint32 WIN_STILL_ACTIVE = STILL_ACTIVE;
	}

#include "HideWindowsPlatformTypes.h"
#include "WindowsPlatformMisc.h"

// static variables
TArray<FString> FWindowsPlatformProcess::DllDirectoryStack;

void* FWindowsPlatformProcess::GetDllHandle( const TCHAR* Filename )
{
	check(Filename);
	::SetErrorMode(SEM_NOOPENFILEERRORBOX);
	return ::LoadLibraryW(Filename);
}

void FWindowsPlatformProcess::FreeDllHandle( void* DllHandle )
{
	// It is okay to call FreeLibrary on 0
	::FreeLibrary((HMODULE)DllHandle);
}

FString FWindowsPlatformProcess::GenerateApplicationPath( const FString& AppName, EBuildConfigurations::Type BuildConfiguration)
{
	FString PlatformName = GetBinariesSubdirectory();
	FString ExecutablePath = FString::Printf(TEXT("..\\%s\\%s"), *PlatformName, *AppName);

	if (BuildConfiguration != EBuildConfigurations::Development)
	{
		ExecutablePath += FString::Printf(TEXT("-%s-%s"), *PlatformName, EBuildConfigurations::ToString(BuildConfiguration));
	}

	ExecutablePath += TEXT(".exe");

	return ExecutablePath;
}

void* FWindowsPlatformProcess::GetDllExport( void* DllHandle, const TCHAR* ProcName )
{
	check(DllHandle);
	check(ProcName);
	return (void*)::GetProcAddress( (HMODULE)DllHandle, TCHAR_TO_ANSI(ProcName) );
}

FBinaryFileVersion FWindowsPlatformProcess::GetBinaryFileVersion( const TCHAR* Filename )
{
	check(Filename);

	::DWORD dwSize = GetFileVersionInfoSize( Filename, NULL );
	if ( dwSize == 0 )
	{
		// Failed to get the version info size
		return FBinaryFileVersion(0, 0, 0, 0);
	}

	uint8* VersionInfo = new uint8[ dwSize ];
	if ( !GetFileVersionInfo(Filename, NULL, dwSize, VersionInfo) )
	{
		// Failed to get the version info
		delete[] VersionInfo;
		return FBinaryFileVersion(0, 0, 0, 0);
	}

	VS_FIXEDFILEINFO* FileInfo = NULL;
	::UINT pLenFileInfo = 0;
	if ( !VerQueryValue( VersionInfo, TEXT("\\"), (LPVOID*) &FileInfo, &pLenFileInfo ) )
	{
		// Failed to query the version info for a value
		delete[] VersionInfo;
		return FBinaryFileVersion(0, 0, 0, 0);
	}

	// Get the major/minor/patch fields from the product version
	int32 Major = (FileInfo->dwProductVersionMS >> 16) & 0xFFFF;
	int32 Minor = (FileInfo->dwProductVersionMS) & 0xFFFF;
	int32 Patch = 0;	// We never want to report back the 'patch' version, because the engine has no business using it for determining compatibility.  All hotfix versions are supposed to be binary and API-compatible.
	int32 Build = (FileInfo->dwProductVersionLS) & 0xFFFF;

	// Try to get the build number from the ProductVersion string. It's too large to fit in a 16-bit ProductVersion field
	if (Build == 0 && !FRocketSupport::IsRocket())
	{
		TCHAR *ProductVersion;
		::UINT ProductVersionLen;
		if (VerQueryValue(VersionInfo, TEXT("\\StringFileInfo\\040904b0\\ProductVersion"), (LPVOID*) &ProductVersion, &ProductVersionLen))
		{
			FEngineVersion DllEngineVersion;
			if (FEngineVersion::Parse(ProductVersion, DllEngineVersion))
			{
				Build = DllEngineVersion.GetChangelist();
			}
		}
	}

	delete[] VersionInfo;

	return FBinaryFileVersion(Major, Minor, Patch, Build);
}

void FWindowsPlatformProcess::PushDllDirectory(const TCHAR* Directory)
{
	// set the directory in windows
	::SetDllDirectory(Directory);
	// remember it
	DllDirectoryStack.Push(Directory);
}

void FWindowsPlatformProcess::PopDllDirectory(const TCHAR* Directory)
{
	// don't allow too many pops (indicates bad code that should be fixed, but won't kill anything, so using ensure)
	ensureMsg(DllDirectoryStack.Num() > 0, TEXT("Tried to PopDllDirectory too many times"));
	// verify we are popping the top
	checkf(DllDirectoryStack.Top() == Directory, TEXT("There was a PushDllDirectory/PopDllDirectory mismatch (Popped %s, which didn't match %s)"), *DllDirectoryStack.Top(), Directory);
	// pop it off
	DllDirectoryStack.Pop();

	// and now set the new DllDirectory to the old value
	if (DllDirectoryStack.Num() > 0)
	{
		::SetDllDirectory(*DllDirectoryStack.Top());
	}
	else
	{
		::SetDllDirectory(TEXT(""));
	}
}

void FWindowsPlatformProcess::LaunchURL( const TCHAR* URL, const TCHAR* Parms, FString* Error )
{
	check( URL );
	const FString URLParams = FString::Printf( TEXT("%s %s"), URL, Parms?Parms:TEXT("") ).TrimTrailing();
	
	if ( URLParams.StartsWith( TEXT("http://") ) || URLParams.StartsWith( TEXT("https://") ) )
	{
		UE_LOG(LogWindows, Log, TEXT("LaunchURL %s"), *URLParams);

		// Specify the browser to open the URL with as windows won't pass anchor points on to file:/// urls that are open via explorer
		const HINSTANCE Code = ::ShellExecuteW(NULL, TEXT("open"), *FWindowsPlatformMisc::GetDefaultBrowser(), *URLParams, TEXT(""), SW_SHOWNORMAL);
		if (Error)
		{
			*Error = ((PTRINT)Code <= 32) ? NSLOCTEXT("Core", "UrlFailed", "Failed launching URL").ToString() : TEXT("");
		}
	}
}

FProcHandle FWindowsPlatformProcess::CreateProc( const TCHAR* URL, const TCHAR* Parms, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden, uint32* OutProcessID, int32 PriorityModifier, const TCHAR* OptionalWorkingDirectory, void* PipeWrite )
{
	//UE_LOG(LogWindows, Log,  TEXT("CreateProc %s %s"), URL, Parms );

	FString CommandLine = FString::Printf(TEXT("%s %s"), URL, Parms);

	PROCESS_INFORMATION ProcInfo;
	SECURITY_ATTRIBUTES Attr;
	Attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	Attr.lpSecurityDescriptor = NULL;
	Attr.bInheritHandle = true;

	uint32 CreateFlags = NORMAL_PRIORITY_CLASS;
	if (PriorityModifier < 0)
	{
		if (PriorityModifier == -1)
		{
			CreateFlags = BELOW_NORMAL_PRIORITY_CLASS;
		}
		else
		{
			CreateFlags = IDLE_PRIORITY_CLASS;
		}
	}
	else if (PriorityModifier > 0)
	{
		if (PriorityModifier == 1)
		{
			CreateFlags = ABOVE_NORMAL_PRIORITY_CLASS;
		}
		else
		{
			CreateFlags = HIGH_PRIORITY_CLASS;
		}
	}
	if (bLaunchDetached)
	{
		CreateFlags |= DETACHED_PROCESS;
	}
	uint32 dwFlags = NULL;
	uint16 ShowWindowFlags = SW_HIDE;
	if (bLaunchReallyHidden)
	{
		dwFlags = STARTF_USESHOWWINDOW;
	}
	else if (bLaunchHidden)
	{
		dwFlags = STARTF_USESHOWWINDOW;
		ShowWindowFlags = SW_SHOWMINNOACTIVE;
	}
	if (PipeWrite)
	{
		dwFlags |= STARTF_USESTDHANDLES;
	}

	STARTUPINFO StartupInfo = { sizeof(STARTUPINFO), NULL, NULL, NULL,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, NULL, dwFlags, ShowWindowFlags, NULL, NULL,
		::GetStdHandle(ProcessConstants::WIN_STD_INPUT_HANDLE), HANDLE(PipeWrite), HANDLE(PipeWrite) };
	if( !CreateProcess( NULL, CommandLine.GetCharArray().GetTypedData(), &Attr, &Attr, true, CreateFlags,
		NULL, OptionalWorkingDirectory, &StartupInfo, &ProcInfo ) )
	{
		if (OutProcessID)
		{
			*OutProcessID = 0;
		}
		return FProcHandle();
	}
	if (OutProcessID)
	{
		*OutProcessID = ProcInfo.dwProcessId;
	}
	// Close the thread handle.
	::CloseHandle( ProcInfo.hThread );
	return FProcHandle(ProcInfo.hProcess);
}

bool FWindowsPlatformProcess::IsProcRunning( FProcHandle & ProcessHandle )
{
	bool bApplicationRunning = true;
	uint32 WaitResult = ::WaitForSingleObject(ProcessHandle.Get(), 0);
	if (WaitResult != WAIT_TIMEOUT)
	{
		bApplicationRunning = false;
	}
	return bApplicationRunning;
}

void FWindowsPlatformProcess::WaitForProc( FProcHandle & ProcessHandle )
{
	::WaitForSingleObject(ProcessHandle.Get(), INFINITE);
}

void FWindowsPlatformProcess::TerminateProc( FProcHandle & ProcessHandle, bool KillTree )
{
	if (KillTree)
	{
		HANDLE SnapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (SnapShot != INVALID_HANDLE_VALUE)
		{
			::DWORD ProcessId = ::GetProcessId(ProcessHandle.Get());

			PROCESSENTRY32 Entry;
			Entry.dwSize = sizeof(PROCESSENTRY32);

			if (::Process32First(SnapShot, &Entry))
			{
				do
				{
					if (Entry.th32ParentProcessID == ProcessId)
					{
						HANDLE ChildProcHandle = ::OpenProcess(PROCESS_ALL_ACCESS, 0, Entry.th32ProcessID);

						if (ChildProcHandle)
						{
							FProcHandle ChildHandle(ChildProcHandle);
							TerminateProc(ChildHandle, KillTree);
//							::TerminateProcess(ChildProcHandle, 1);
						}
					}
				}
				while(::Process32Next(SnapShot, &Entry));
			}
		}
	}

	TerminateProcess(ProcessHandle.Get(),0);
	// Process is terminated, so we can close the process handle.
	ProcessHandle.Close();
}

uint32 FWindowsPlatformProcess::GetCurrentProcessId()
{
	return ::GetCurrentProcessId();
}

bool FWindowsPlatformProcess::GetProcReturnCode( FProcHandle & ProcHandle, int32* ReturnCode )
{
	return ::GetExitCodeProcess( ProcHandle.Get(), (::DWORD *)ReturnCode ) && *((uint32*)ReturnCode) != ProcessConstants::WIN_STILL_ACTIVE;
}

bool FWindowsPlatformProcess::IsApplicationRunning( uint32 ProcessId )
{
	bool bApplicationRunning = true;
	HANDLE ProcessHandle = OpenProcess(SYNCHRONIZE, false, ProcessId);
	if (ProcessHandle == NULL)
	{
		bApplicationRunning = false;
	}
	else
	{
		uint32 WaitResult = WaitForSingleObject(ProcessHandle, 0);
		if (WaitResult != WAIT_TIMEOUT)
		{
			bApplicationRunning = false;
		}
		::CloseHandle(ProcessHandle);
	}
	return bApplicationRunning;
}

bool FWindowsPlatformProcess::IsApplicationRunning( const TCHAR* ProcName )
{
    // append the extension

    FString ProcNameWithExtension = ProcName;
    if( ProcNameWithExtension.Find( TEXT(".exe"), ESearchCase::IgnoreCase, ESearchDir::FromEnd ) == INDEX_NONE )
    {
        ProcNameWithExtension += TEXT(".exe");
    }

	HANDLE SnapShot = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( SnapShot != INVALID_HANDLE_VALUE )
	{
		PROCESSENTRY32 Entry;
		Entry.dwSize = sizeof( PROCESSENTRY32 );

		if( ::Process32First( SnapShot, &Entry ) )
		{
			do
			{
				if( FCString::Strcmp( *ProcNameWithExtension, Entry.szExeFile ) == 0 )
				{
					::CloseHandle( SnapShot );
					return true;
				}
			} while( ::Process32Next( SnapShot, &Entry ) );
		}
	}

	::CloseHandle( SnapShot );
	return false;
}

bool FWindowsPlatformProcess::IsThisApplicationForeground()
{
	uint32 ForegroundProcess;
	::GetWindowThreadProcessId(GetForegroundWindow(), (::DWORD *)&ForegroundProcess);
	return (ForegroundProcess == GetCurrentProcessId());
}

void FWindowsPlatformProcess::ReadFromPipes(FString* OutStrings[], HANDLE InPipes[], int32 PipeCount)
{
	for (int32 PipeIndex = 0; PipeIndex < PipeCount; ++PipeIndex)
	{
		if (InPipes[PipeIndex] && OutStrings[PipeIndex])
		{
			*OutStrings[PipeIndex] += ReadPipe(InPipes[PipeIndex]);
		}
	}
}

/**
 * Executes a process, returning the return code, stdout, and stderr. This
 * call blocks until the process has returned.
 */
void FWindowsPlatformProcess::ExecProcess( const TCHAR* URL, const TCHAR* Params, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr )
{
	PROCESS_INFORMATION ProcInfo;
	SECURITY_ATTRIBUTES Attr;

	FString CommandLine = FString::Printf(TEXT("%s %s"), URL, Params);

	Attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	Attr.lpSecurityDescriptor = NULL;
	Attr.bInheritHandle = true;

	uint32 CreateFlags = NORMAL_PRIORITY_CLASS;
	CreateFlags |= DETACHED_PROCESS;

	uint32 dwFlags = STARTF_USESHOWWINDOW;
	uint16 ShowWindowFlags = SW_SHOWMINNOACTIVE;

	const int32 MaxPipeCount = 2;
	HANDLE ReadablePipes[MaxPipeCount] = {0};
	HANDLE WritablePipes[MaxPipeCount] = {0};
	const bool bRedirectOutput = OutStdOut != NULL || OutStdErr != NULL;

	if (bRedirectOutput)
	{
		dwFlags |= STARTF_USESTDHANDLES;
		for (int32 PipeIndex = 0; PipeIndex < MaxPipeCount; ++PipeIndex)
		{
			verify(::CreatePipe(&ReadablePipes[PipeIndex], &WritablePipes[PipeIndex], &Attr, 0));
			verify(::SetHandleInformation(ReadablePipes[PipeIndex], /*dwMask=*/ HANDLE_FLAG_INHERIT, /*dwFlags=*/ 0));
		}
	}

	STARTUPINFO StartupInfo = { sizeof(STARTUPINFO), NULL, NULL, NULL,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, NULL, dwFlags, ShowWindowFlags, NULL, NULL,
		::GetStdHandle(ProcessConstants::WIN_STD_INPUT_HANDLE), WritablePipes[0], WritablePipes[1] };
	if (CreateProcess(NULL, CommandLine.GetCharArray().GetTypedData(), &Attr, &Attr, true, CreateFlags,
		NULL, NULL, &StartupInfo, &ProcInfo))
	{
		if (bRedirectOutput)
		{
			FString* OutStrings[MaxPipeCount] = { OutStdOut, OutStdErr };
			FProcHandle ProcHandle(ProcInfo.hProcess);
			do 
			{
				ReadFromPipes(OutStrings, ReadablePipes, MaxPipeCount);
				FPlatformProcess::Sleep(0);
			} while (IsProcRunning(ProcHandle));
			ReadFromPipes(OutStrings, ReadablePipes, MaxPipeCount);
		}
		else
		{
			::WaitForSingleObject(ProcInfo.hProcess, INFINITE);
		}		
		if (OutReturnCode)
		{
			verify(::GetExitCodeProcess(ProcInfo.hProcess, (::DWORD*)OutReturnCode));
		}
		::CloseHandle(ProcInfo.hProcess);
		::CloseHandle(ProcInfo.hThread);
	}
	else
	{
		if (bRedirectOutput)
		{
			for (int32 PipeIndex = 0; PipeIndex < MaxPipeCount; ++PipeIndex)
			{
				verify(::CloseHandle(WritablePipes[PipeIndex]));
			}
		}
	}

	if (bRedirectOutput)
	{
		for (int32 PipeIndex = 0; PipeIndex < MaxPipeCount; ++PipeIndex)
		{
			verify(::CloseHandle(ReadablePipes[PipeIndex]));
		}
	}
}

void FWindowsPlatformProcess::CleanFileCache()
{
	bool bShouldCleanShaderWorkingDirectory = true;
#if !(UE_BUILD_SHIPPING && WITH_EDITOR)
	// Only clean the shader working directory if we are the first instance, to avoid deleting files in use by other instances
	//@todo - check if any other instances are running right now
	bShouldCleanShaderWorkingDirectory = GIsFirstInstance;
#endif

	if (bShouldCleanShaderWorkingDirectory && !FParse::Param( FCommandLine::Get(), TEXT("Multiprocess")))
	{
		// get shader path, and convert it to the userdirectory
		FString ShaderDir = FString(FPlatformProcess::BaseDir()) / FPlatformProcess::ShaderDir();
		FString UserShaderDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*ShaderDir);
		FPaths::CollapseRelativeDirectories(ShaderDir);

		// make sure we don't delete from the source directory
		if (ShaderDir != UserShaderDir)
		{
			IFileManager::Get().DeleteDirectory(*UserShaderDir, false, true);
		}

		FPlatformProcess::CleanShaderWorkingDir();
	}
}

const TCHAR* FWindowsPlatformProcess::BaseDir()
{
	static TCHAR Result[512]=TEXT("");
	if( !Result[0] )
	{
		// Get directory this executable was launched from.
		GetModuleFileName( hInstance, Result, ARRAY_COUNT(Result) );
		FString TempResult(Result);
		TempResult = TempResult.Replace(TEXT("\\"), TEXT("/"));
		FCString::Strcpy(Result, *TempResult);
		int32 StringLength = FCString::Strlen(Result);
		if(StringLength > 0)
		{
			--StringLength;
			for(; StringLength > 0; StringLength-- )
			{
				if( Result[StringLength - 1] == TEXT('/') || Result[StringLength - 1] == TEXT('\\') )
				{
					break;
				}
			}
		}
		Result[StringLength] = 0;

		FString CollapseResult(Result);
		FPaths::CollapseRelativeDirectories(CollapseResult);
		FCString::Strcpy(Result, *CollapseResult);
	}
	return Result;
}

const TCHAR* FWindowsPlatformProcess::UserDir()
{
	static FString WindowsUserDir;
	if( !WindowsUserDir.Len() )
	{
		TCHAR UserPath[MAX_PATH];
		// get the My Documents directory
		HRESULT Ret = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, UserPath);

		// make the base user dir path
		WindowsUserDir = FString(UserPath).Replace(TEXT("\\"), TEXT("/")) + TEXT("/");
	}
	return *WindowsUserDir;
}

const TCHAR* FWindowsPlatformProcess::UserSettingsDir()
{
	static FString WindowsUserSettingsDir;
	if (!WindowsUserSettingsDir.Len())
	{
		TCHAR UserPath[MAX_PATH];
		// get the My Documents directory
		HRESULT Ret = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, UserPath);

		// make the base user dir path
		WindowsUserSettingsDir = FString(UserPath).Replace(TEXT("\\"), TEXT("/")) + TEXT("/");
	}
	return *WindowsUserSettingsDir;
}

const TCHAR* FWindowsPlatformProcess::ApplicationSettingsDir()
{
	static FString WindowsApplicationSettingsDir;
	if( !WindowsApplicationSettingsDir.Len() )
	{
		TCHAR ApplictionSettingsPath[MAX_PATH];
		// get the My Documents directory
		HRESULT Ret = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, ApplictionSettingsPath);

		// make the base user dir path
		// @todo rocket this folder should be based on your company name, not just be hard coded to /Epic/
		WindowsApplicationSettingsDir = FString(ApplictionSettingsPath) + TEXT("/Epic/");
	}
	return *WindowsApplicationSettingsDir;
}

const TCHAR* FWindowsPlatformProcess::ComputerName()
{
	static TCHAR Result[256]=TEXT("");
	if( !Result[0] )
	{
		uint32 Size=ARRAY_COUNT(Result);
		GetComputerName( Result, (::DWORD*)&Size );
	}
	return Result;
}

const TCHAR* FWindowsPlatformProcess::UserName(bool bOnlyAlphaNumeric/* = true*/)
{
	static TCHAR Result[256]=TEXT("");
	static TCHAR ResultAlpha[256]=TEXT("");
	if( bOnlyAlphaNumeric )
	{
		if( !ResultAlpha[0] )
		{
			uint32 Size=ARRAY_COUNT(ResultAlpha);
			GetUserName( ResultAlpha, (::DWORD*)&Size );
			TCHAR *c, *d;
			for( c=ResultAlpha, d=ResultAlpha; *c!=0; c++ )
				if( FChar::IsAlnum(*c) )
					*d++ = *c;
			*d++ = 0;
		}
		return ResultAlpha;
	}
	else
	{
		if( !Result[0] )
		{
			uint32 Size=ARRAY_COUNT(Result);
			GetUserName( Result, (::DWORD*)&Size );
		}
		return Result;
	}
}

void FWindowsPlatformProcess::SetCurrentWorkingDirectoryToBaseDir()
{
	verify(SetCurrentDirectoryW(BaseDir()));
}

const TCHAR* FWindowsPlatformProcess::ExecutableName(bool bRemoveExtension)
{
	static TCHAR Result[512]=TEXT("");
	static TCHAR ResultWithExt[512]=TEXT("");
	if( !Result[0] )
	{
		// Get complete path for the executable
		if ( GetModuleFileName( hInstance, Result, ARRAY_COUNT(Result) ) != 0 )
		{
			// Remove all of the path information by finding the base filename
			FString FileName = Result;
			FString FileNameWithExt = Result;
			FCString::Strncpy( Result, *( FPaths::GetBaseFilename(FileName) ), ARRAY_COUNT(Result) );
			FCString::Strncpy( ResultWithExt, *( FPaths::GetCleanFilename(FileNameWithExt) ), ARRAY_COUNT(ResultWithExt) );
		}
		// If the call failed, zero out the memory to be safe
		else
		{
			FMemory::Memzero( Result, sizeof( Result ) );
			FMemory::Memzero( ResultWithExt, sizeof( ResultWithExt ) );
		}
	}

	return (bRemoveExtension ? Result : ResultWithExt);
}

const TCHAR* FWindowsPlatformProcess::GetBinariesSubdirectory()
{
	if (PLATFORM_64BITS)
	{
		return TEXT("Win64");
	}
	return TEXT("Win32");
}

void FWindowsPlatformProcess::LaunchFileInDefaultExternalApplication( const TCHAR* FileName, const TCHAR* Parms /*= NULL*/ )
{
	// First attempt to open the file in its default application
	UE_LOG(LogWindows, Log,  TEXT("LaunchFileInExternalEditor %s %s"), FileName, Parms ? Parms : TEXT("") );
	HINSTANCE Code = ::ShellExecuteW( NULL, TEXT("open"), FileName, Parms ? Parms : TEXT(""), TEXT(""), SW_SHOWNORMAL );
	
	UE_LOG(LogWindows, Log,  TEXT("Launch application code for %s %s: %d"), FileName, Parms ? Parms : TEXT(""), (PTRINT)Code );

	// If opening the file in the default application failed, check to see if it's because the file's extension does not have
	// a default application associated with it. If so, prompt the user with the Windows "Open With..." dialog to allow them to specify
	// an application to use.
	if ( (PTRINT)Code == SE_ERR_NOASSOC || (PTRINT)Code == SE_ERR_ASSOCINCOMPLETE )
	{
		::ShellExecuteW( NULL, TEXT("open"), TEXT("RUNDLL32.EXE"), *FString::Printf( TEXT("shell32.dll,OpenAs_RunDLL %s"), FileName ), TEXT(""), SW_SHOWNORMAL );
	}
}

void FWindowsPlatformProcess::ExploreFolder( const TCHAR* FilePath )
{
	if (IFileManager::Get().DirectoryExists( FilePath ))
	{
		// Explore the folder
		::ShellExecuteW( NULL, TEXT("explore"), FilePath, NULL, NULL, SW_SHOWNORMAL );
	}
	else
	{
		// Explore the file
		FString NativeFilePath = FString(FilePath).Replace(TEXT("/"), TEXT("\\"));
		FString Parameters = FString::Printf( TEXT("/select,%s"), *NativeFilePath);
		::ShellExecuteW( NULL, TEXT("open"), TEXT("explorer.exe"), *Parameters, NULL, SW_SHOWNORMAL );
	}
}

/**
 * Resolves UNC path to local (full) path if possible.
 *
 * @param	InUNCPath		UNC path to resolve
 *
 * @param	OutPath		Resolved local path
 *
 * @return true if the path was resolved, false otherwise
 */
bool FWindowsPlatformProcess::ResolveNetworkPath( FString InUNCPath, FString& OutPath )
{
	// Get local machine name first and check if this UNC path points to local share
	// (if it's not UNC path it will also fail this check)
	uint32 ComputerNameSize = MAX_COMPUTERNAME_LENGTH;
	TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 3] = { '\\', '\\', '\0', };

	if ( GetComputerName( ComputerName + 2, (::DWORD*)&ComputerNameSize ) )
	{
		// Check if the filename is pointing to local shared folder
		if ( InUNCPath.StartsWith( ComputerName ) )
		{
			// Get the share name (it's the first folder after the computer name)
			int32 ComputerNameLen = FCString::Strlen( ComputerName );
			int32 ShareNameLen = InUNCPath.Find( TEXT( "\\" ), ESearchCase::CaseSensitive, ESearchDir::FromStart, ComputerNameLen + 1 ) - ComputerNameLen - 1;
			FString ShareName = InUNCPath.Mid( ComputerNameLen + 1, ShareNameLen );

			// NetShareGetInfo doesn't accept const TCHAR* as the share name so copy to temp array
			SHARE_INFO_2* BufPtr = NULL;
			::NET_API_STATUS res;
			TCHAR ShareNamePtr[MAX_PATH];
			FCString::Strcpy(ShareNamePtr, ShareName.Len() + 1, *ShareName);

			// Call the NetShareGetInfo function, specifying level 2
			if ( ( res = NetShareGetInfo( NULL, ShareNamePtr, 2, (LPBYTE*)&BufPtr ) ) == ERROR_SUCCESS )
			{
				// Construct the local path
				OutPath = FString( BufPtr->shi2_path ) + InUNCPath.Mid( ComputerNameLen + 1 + ShareNameLen );

				// Free the buffer allocated by NetShareGetInfo
				NetApiBufferFree(BufPtr);
				
				return true;
			}
		}
	}

	// InUNCPath is not an UNC path or it's not pointing to local folder or something went wrong in NetShareGetInfo (insufficient privileges?)
	return false;
}

DECLARE_CYCLE_STAT(TEXT("CPU Stall - Sleep"),STAT_Sleep,STATGROUP_CPUStalls);

void FWindowsPlatformProcess::Sleep( float Seconds )
{
	SCOPE_CYCLE_COUNTER(STAT_Sleep);
	FThreadIdleStats::FScopeIdle Scope;
	::Sleep( (uint32)(Seconds * 1000.0) );
}

void FWindowsPlatformProcess::SleepInfinite()
{
	check(FPlatformProcess::SupportsMultithreading());
	::Sleep(INFINITE);
}

#include "WindowsEvent.h"

FEvent* FWindowsPlatformProcess::CreateSynchEvent(bool bIsManualReset)
{
	// Allocate the new object
	FEvent* Event = NULL;	
	if (FPlatformProcess::SupportsMultithreading())
	{
		Event = new FEventWin();
	}
	else
	{
		// Fake event object.
		Event = new FSingleThreadEvent();
	}
	// If the internal create fails, delete the instance and return NULL
	if (!Event->Create(bIsManualReset))
	{
		delete Event;
		Event = NULL;
	}
	return Event;
}

#include "AllowWindowsPlatformTypes.h"

DECLARE_CYCLE_STAT(TEXT("CPU Stall - Wait For Event"),STAT_EventWait,STATGROUP_CPUStalls);

bool FEventWin::Wait (uint32 WaitTime)
{
	SCOPE_CYCLE_COUNTER(STAT_EventWait);
	FThreadIdleStats::FScopeIdle Scope;
	check(Event);

	return (WaitForSingleObject(Event, WaitTime) == WAIT_OBJECT_0);
}

#include "HideWindowsPlatformTypes.h"

#include "WindowsRunnableThread.h"

FRunnableThread* FWindowsPlatformProcess::CreateRunnableThread()
{
	return new FRunnableThreadWin();
}

void FWindowsPlatformProcess::ClosePipe( void* ReadPipe, void* WritePipe )
{
	if ( ReadPipe != NULL && ReadPipe != INVALID_HANDLE_VALUE )
	{
		::CloseHandle(ReadPipe);
	}
	if ( WritePipe != NULL && WritePipe != INVALID_HANDLE_VALUE )
	{
		::CloseHandle(WritePipe);
	}
}

bool FWindowsPlatformProcess::CreatePipe( void*& ReadPipe, void*& WritePipe )
{
	SECURITY_ATTRIBUTES Attr = { sizeof(SECURITY_ATTRIBUTES), NULL, true };
	
	if (!::CreatePipe(&ReadPipe, &WritePipe, &Attr, 0))
	{
		return false;
	}

	if (!::SetHandleInformation(ReadPipe, HANDLE_FLAG_INHERIT, 0))
	{
		return false;
	}

	return true;
}

FString FWindowsPlatformProcess::ReadPipe( void* ReadPipe )
{
	FString Output;

	uint32 BytesAvailable = 0;
	if (::PeekNamedPipe(ReadPipe, NULL, 0, NULL, (::DWORD*)&BytesAvailable, NULL) && (BytesAvailable > 0))
	{
		UTF8CHAR* Buffer = new UTF8CHAR[BytesAvailable + 1];
		uint32 BytesRead = 0;
		if (::ReadFile(ReadPipe, Buffer, BytesAvailable, (::DWORD*)&BytesRead, NULL))
		{
			if (BytesRead > 0)
			{
				Buffer[BytesRead] = '\0';
				Output += FUTF8ToTCHAR((const ANSICHAR*)Buffer).Get();
			}
		}
		delete [] Buffer;
	}

	return Output;
}
