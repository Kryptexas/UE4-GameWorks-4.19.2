// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsPlatformMisc.cpp: Windows implementations of misc functions
=============================================================================*/

#include "CorePrivate.h"
#include "ExceptionHandling.h"
#include "SecureHash.h"
#include "WindowsApplication.h"
#include "EngineVersion.h"

// Resource includes.
#include "Runtime/Launch/Resources/Windows/Resource.h"
#include "Runtime/Launch/Resources/Version.h"
#include "AllowWindowsPlatformTypes.h"
	#include <time.h>
	#include <MMSystem.h>
	#include <rpcsal.h>					// from DXSDK
	#include <gameux.h>					// from DXSDK For IGameExplorer
	#include <shlobj.h>
	#include <intshcut.h>
	#include <shellapi.h>
	#include <Iphlpapi.h>
#include "HideWindowsPlatformTypes.h"
#include "MallocTBB.h"
#include "ModuleManager.h"
#include "MallocAnsi.h"
#include "VarargsHelper.h"

#if !FORCE_ANSI_ALLOCATOR
	#include "MallocBinned.h"
	#include "AllowWindowsPlatformTypes.h"
		#include <psapi.h>
	#include "HideWindowsPlatformTypes.h"
	#pragma comment(lib, "psapi.lib")
#endif



/** 
 * Whether support for integrating into the firewall is there
 */
#define WITH_FIREWALL_SUPPORT	0

/** Width of the primary monitor, in pixels. */
CORE_API int32 GPrimaryMonitorWidth = 0;
/** Height of the primary monitor, in pixels. */
CORE_API int32 GPrimaryMonitorHeight = 0;
/** Rectangle of the work area on the primary monitor (excluding taskbar, etc) in "virtual screen coordinates" (pixels). */
CORE_API RECT GPrimaryMonitorWorkRect;
/** Virtual screen rectangle including all monitors. */
CORE_API RECT GVirtualScreenRect;


/** Settings for the game Window */
CORE_API HWND GGameWindow = NULL;
CORE_API bool GGameWindowUsingStartupWindowProc = false;	// Whether the game is using the startup window procedure
CORE_API uint32 GGameWindowStyle = 0;
CORE_API int32 GGameWindowPosX = 0;
CORE_API int32 GGameWindowPosY = 0;
CORE_API int32 GGameWindowWidth = 100;
CORE_API int32 GGameWindowHeight = 100;

extern "C"
{
	CORE_API HINSTANCE hInstance = NULL;
}


/** Original C- Runtime pure virtual call handler that is being called in the (highly likely) case of a double fault */
_purecall_handler DefaultPureCallHandler;

/**
* Our own pure virtual function call handler, set by appPlatformPreInit. Falls back
* to using the default C- Runtime handler in case of double faulting.
*/
static void PureCallHandler()
{
	static bool bHasAlreadyBeenCalled = false;
	FPlatformMisc::DebugBreak();
	if( bHasAlreadyBeenCalled )
	{
		// Call system handler if we're double faulting.
		if( DefaultPureCallHandler )
		{
			DefaultPureCallHandler();
		}
	}
	else
	{
		bHasAlreadyBeenCalled = true;
		if( GIsRunning )
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("Core", "PureVirtualFunctionCalledWhileRunningApp", "Pure virtual function being called while application was running (GIsRunning == 1).") );
		}
		UE_LOG(LogWindows, Fatal,TEXT("Pure virtual function being called") );
	}
}

/*-----------------------------------------------------------------------------
	SHA-1 functions.
-----------------------------------------------------------------------------*/

/**
* Get the hash values out of the executable hash section
*
* NOTE: hash keys are stored in the executable, you will need to put a line like the
*		 following into your PCLaunch.rc settings:
*			ID_HASHFILE RCDATA "../../../../GameName/Build/Hashes.sha"
*
*		 Then, use the -sha option to the cooker (must be from commandline, not
*       frontend) to generate the hashes for .ini, loc, startup packages, and .usf shader files
*
*		 You probably will want to make and checkin an empty file called Hashses.sha
*		 into your source control to avoid linker warnings. Then for testing or final
*		 final build ONLY, use the -sha command and relink your executable to put the
*		 hashes for the current files into the executable.
*/
static void InitSHAHashes()
{
	uint32 SectionSize = 0;
	void* SectionData = NULL;
	// find the resource for the file hash in the exe by ID
	HRSRC HashFileFindResH = FindResource(NULL,MAKEINTRESOURCE(ID_HASHFILE),RT_RCDATA);
	if( HashFileFindResH )
	{
		// load it
		HGLOBAL HashFileLoadResH = LoadResource(NULL,HashFileFindResH);
		if( !HashFileLoadResH )
		{
			FMessageDialog::ShowLastError();
		}
		else
		{
			// get size
			SectionSize = SizeofResource(NULL,HashFileFindResH);
			// get the data. no need to unlock it
			SectionData = (uint8*)LockResource(HashFileLoadResH);
		}
	}

	// there may be a dummy byte for platforms that can't handle empty files for linking
	if (SectionSize <= 1)
	{
		return;
	}

	// look for the hash section
	if( SectionData )
	{
		FSHA1::InitializeFileHashesFromBuffer((uint8*)SectionData, SectionSize);
	}
}



void FWindowsPlatformMisc::PlatformPreInit()
{
	// Use our own handler for pure virtuals being called.
	DefaultPureCallHandler = _set_purecall_handler( PureCallHandler );

	// Check Windows version.
	OSVERSIONINFOEX OsVersionInfo = { 0 };
	OsVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
	GetVersionEx( ( LPOSVERSIONINFO )&OsVersionInfo );

	const int32 MinResolution[] = {640,480};
	if ( ::GetSystemMetrics(SM_CXSCREEN) < MinResolution[0] || ::GetSystemMetrics(SM_CYSCREEN) < MinResolution[1] )
	{
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("Launch", "Error_ResolutionTooLow", "The current resolution is too low to run this game.") );
		FPlatformMisc::RequestExit( false );
	}

#if WITH_ENGINE
	// Get the total screen size of the primary monitor.
	GPrimaryMonitorWidth = ::GetSystemMetrics( SM_CXSCREEN );
	GPrimaryMonitorHeight = ::GetSystemMetrics( SM_CYSCREEN );
	GVirtualScreenRect.left = ::GetSystemMetrics( SM_XVIRTUALSCREEN );
	GVirtualScreenRect.top = ::GetSystemMetrics( SM_YVIRTUALSCREEN );
	GVirtualScreenRect.right = ::GetSystemMetrics( SM_CXVIRTUALSCREEN ) + GVirtualScreenRect.left;
	GVirtualScreenRect.bottom = ::GetSystemMetrics( SM_CYVIRTUALSCREEN ) + GVirtualScreenRect.top;

	// Get the screen rect of the primary monitor, exclusing taskbar etc.
	SystemParametersInfo( SPI_GETWORKAREA, 0, &GPrimaryMonitorWorkRect, 0 );
#endif		// WITH_ENGINE

	// initialize the file SHA hash mapping
	InitSHAHashes();
}


void FWindowsPlatformMisc::PlatformInit()
{
	// Randomize.
	if( GIsBenchmarking )
	{
		srand( 0 );
	}
	else
	{
		srand( (unsigned)time( NULL ) );
	}

	// Set granularity of sleep and such to 1 ms.
	timeBeginPeriod( 1 );

	// Identity.
	UE_LOG(LogInit, Log, TEXT("Computer: %s"), FPlatformProcess::ComputerName() );
	UE_LOG(LogInit, Log, TEXT("User: %s"), FPlatformProcess::UserName() );

	// Get CPU info.
	const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
	UE_LOG(LogInit, Log, TEXT("CPU Page size=%i, Cores=%i"), MemoryConstants.PageSize, FPlatformMisc::NumberOfCores() );

	// Timer resolution.
	UE_LOG(LogInit, Log, TEXT("High frequency timer resolution =%f MHz"), 0.000001 / FPlatformTime::GetSecondsPerCycle() );

	// Register on the game thread.
	FWindowsPlatformStackWalk::RegisterOnModulesChanged();
}


#if WITH_ENGINE
/**
 * Temporary window procedure for the game window during startup.
 * It gets replaced later on with SetWindowLong().
 */
LRESULT CALLBACK StartupWindowProc(HWND hWnd, uint32 Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		// Prevent power management
		case WM_POWERBROADCAST:
		{
			switch( wParam )
			{
				case PBT_APMQUERYSUSPEND:
				case PBT_APMQUERYSTANDBY:
					return BROADCAST_QUERY_DENY;
			}
		}
	}

	return DefWindowProc(hWnd, Message, wParam, lParam);
}

void FWindowsPlatformMisc::PlatformPostInit()
{

}

/**
 * Prevents screen-saver from kicking in by moving the mouse by 0 pixels. This works even on
 * Vista in the presence of a group policy for password protected screen saver.
 */
void FWindowsPlatformMisc::PreventScreenSaver()
{
	INPUT Input = { 0 };
	Input.type = INPUT_MOUSE;
	Input.mi.dx = 0;
	Input.mi.dy = 0;	
	Input.mi.mouseData = 0;
	Input.mi.dwFlags = MOUSEEVENTF_MOVE;
	Input.mi.time = 0;
	Input.mi.dwExtraInfo = 0; 	
	SendInput(1,&Input,sizeof(INPUT));
}

#endif		// WITH_ENGINE



/**
 * Handler called for console events like closure, CTRL-C, ...
 *
 * @param Type	unused
 */
static BOOL WINAPI ConsoleCtrlHandler( ::DWORD /*Type*/ )
{
	// make sure as much data is written to disk as possible
	if (GLog)
	{
		GLog->Flush();
	}
	if (GWarn)
	{
		GWarn->Flush();
	}
	if (GError)
	{
		GError->Flush();
	}

	if( !GIsRequestingExit )
	{
		PostQuitMessage( 0 );
		GIsRequestingExit = 1;
	}
	else
	{
		// User has pressed Ctrl-C twice and we should forcibly terminate the application.
		// ExitProcess would run global destructors, possibly causing assertions.
		TerminateProcess(GetCurrentProcess(), 0);
	}
	return true;
}

GenericApplication* FWindowsPlatformMisc::CreateApplication()
{
	HICON AppIconHandle = LoadIcon( hInstance, MAKEINTRESOURCE( GetAppIcon() ) );
	if( AppIconHandle == NULL )
	{
		AppIconHandle = LoadIcon( (HINSTANCE)NULL, IDI_APPLICATION ); 
	}

	return FWindowsApplication::CreateWindowsApplication( hInstance, AppIconHandle );
}

void FWindowsPlatformMisc::SetGracefulTerminationHandler()
{
	// Set console control handler so we can exit if requested.
	SetConsoleCtrlHandler(ConsoleCtrlHandler, true);
}

void FWindowsPlatformMisc::GetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, int32 ResultLength)
{
	uint32 Error = ::GetEnvironmentVariable(VariableName, Result, ResultLength);
	if (Error <= 0)
	{
		// on error, just return an empty string
		*Result = 0;
	}
}

TArray<uint8> FWindowsPlatformMisc::GetMacAddress()
{
	TArray<uint8> Result;
	IP_ADAPTER_INFO IpAddresses[16];
	ULONG OutBufferLength = sizeof(IP_ADAPTER_INFO) * 16;
	// Read the adapters
	uint32 RetVal = GetAdaptersInfo(IpAddresses,&OutBufferLength);
	if (RetVal == NO_ERROR)
	{
		PIP_ADAPTER_INFO AdapterList = IpAddresses;
		// Walk the set of addresses copying each one
		while (AdapterList)
		{
			// If there is an address to read
			if (AdapterList->AddressLength > 0)
			{
				// Copy the data and say we did
				Result.AddZeroed(AdapterList->AddressLength);
				FMemory::Memcpy(Result.GetData(),AdapterList->Address,AdapterList->AddressLength);
				break;
			}
			AdapterList = AdapterList->Next;
		}
	}
	return Result;
}

/**
 * We need to see if we are doing AutomatedPerfTesting and we are -unattended if we are then we have
 * crashed in some terrible way and we need to make certain we can kill -9 the devenv process /
 * vsjitdebugger.exe and any other processes that are still running
 */
static void HardKillIfAutomatedTesting()
{
	// so here 
	int32 FromCommandLine = 0;
	FParse::Value( FCommandLine::Get(), TEXT("AutomatedPerfTesting="), FromCommandLine );
	if(( FApp::IsUnattended() == true ) && ( FromCommandLine != 0 ) && ( FParse::Param(FCommandLine::Get(), TEXT("KillAllPopUpBlockingWindows")) == true ))
	{

		UE_LOG(LogWindows, Warning, TEXT("Attempting to run KillAllPopUpBlockingWindows"));

		TCHAR KillAllBlockingWindows[] = TEXT("KillAllPopUpBlockingWindows.bat");
		// .bat files never seem to launch correctly with FPlatformProcess::CreateProc so we just use the FPlatformProcess::LaunchURL which will call ShellExecute
		// we don't really care about the return code in this case 
		FPlatformProcess::LaunchURL( TEXT("KillAllPopUpBlockingWindows.bat"), NULL, NULL );
	}
}


void FWindowsPlatformMisc::SubmitErrorReport( const TCHAR* InErrorHist, EErrorReportMode::Type InMode )
{
	if ((!FPlatformMisc::IsDebuggerPresent() || GAlwaysReportCrash) && !FParse::Param(FCommandLine::Get(), TEXT("CrashForUAT")))
	{
		if (GUseCrashReportClient)
		{
			HardKillIfAutomatedTesting();
			return;
		}

		TCHAR ReportDumpVersion[] = TEXT("3");

		FString ReportDumpPath;
		{
			const TCHAR ReportDumpFilename[] = TEXT("UnrealAutoReportDump");
			ReportDumpPath = FPaths::CreateTempFilename( *FPaths::GameLogDir(), ReportDumpFilename, TEXT( ".txt" ) );
		}

		FArchive * AutoReportFile = IFileManager::Get().CreateFileWriter(*ReportDumpPath, FILEWRITE_EvenIfReadOnly);
		if (AutoReportFile != NULL)
		{
			TCHAR CompName[256];
			FCString::Strcpy(CompName, FPlatformProcess::ComputerName());
			TCHAR UserName[256];
			FCString::Strcpy(UserName, FPlatformProcess::UserName());
			TCHAR GameName[256];
			FCString::Strcpy(GameName, *FString::Printf(TEXT("%s %s"), TEXT(BRANCH_NAME), FApp::GetGameName()));
			TCHAR PlatformName[32];
#if PLATFORM_64BITS
			FCString::Strcpy(PlatformName, TEXT("PC 64-bit"));
#else	//PLATFORM_64BITS
			FCString::Strcpy(PlatformName, TEXT("PC 32-bit"));
#endif	//PLATFORM_64BITS
			TCHAR CultureName[10];
			FCString::Strcpy(CultureName, *FInternationalization::GetDefaultCulture()->GetName());
			TCHAR SystemTime[256];
			FCString::Strcpy(SystemTime, *FDateTime::Now().ToString());
			TCHAR EngineVersionStr[32];
			FCString::Strcpy(EngineVersionStr, *GEngineVersion.ToString());

			TCHAR ChangelistVersionStr[32];
			int32 ChangelistFromCommandLine = 0;
			const bool bFoundAutomatedBenchMarkingChangelist = FParse::Value( FCommandLine::Get(), TEXT("-gABC="), ChangelistFromCommandLine );
			if( bFoundAutomatedBenchMarkingChangelist == true )
			{
				FCString::Strcpy(ChangelistVersionStr, *FString::FromInt(ChangelistFromCommandLine));
			}
			// we are not passing in the changelist to use so use the one that was stored in the ObjectVersion
			else
			{
				FCString::Strcpy(ChangelistVersionStr, *FString::FromInt(GEngineVersion.GetChangelist()));
			}

			TCHAR CmdLine[2048];
			FCString::Strcpy(CmdLine, FCommandLine::Get());
			TCHAR BaseDir[260];
			FCString::Strcpy(BaseDir, FPlatformProcess::BaseDir());
			TCHAR separator = 0;

			TCHAR EngineMode[64];
			if( IsRunningCommandlet() )
			{
				FCString::Strcpy(EngineMode, TEXT("Commandlet"));
			}
			else if( GIsEditor )
			{
				FCString::Strcpy(EngineMode, TEXT("Editor"));
			}
			else if( IsRunningDedicatedServer() )
			{
				FCString::Strcpy(EngineMode, TEXT("Server"));
			}
			else
			{
				FCString::Strcpy(EngineMode, TEXT("Game"));
			}

			//build the report dump file
			AutoReportFile->Serialize(ReportDumpVersion, FCString::Strlen(ReportDumpVersion) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(CompName, FCString::Strlen(CompName) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(UserName, FCString::Strlen(UserName) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(GameName, FCString::Strlen(GameName) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(PlatformName, FCString::Strlen(PlatformName) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(CultureName, FCString::Strlen(CultureName) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(SystemTime, FCString::Strlen(SystemTime) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(EngineVersionStr, FCString::Strlen(EngineVersionStr) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(ChangelistVersionStr, FCString::Strlen(ChangelistVersionStr) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(CmdLine, FCString::Strlen(CmdLine) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(BaseDir, FCString::Strlen(BaseDir) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));

			TCHAR* NonConstErrorHist = const_cast< TCHAR* >( InErrorHist );
			AutoReportFile->Serialize(NonConstErrorHist, FCString::Strlen(NonConstErrorHist) * sizeof(TCHAR));

			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Serialize(EngineMode, FCString::Strlen(EngineMode) * sizeof(TCHAR));
			AutoReportFile->Serialize(&separator, sizeof(TCHAR));
			AutoReportFile->Close();
			
			if ( !GIsBuildMachine )
			{
				TCHAR AutoReportExe[] = TEXT("../../../Engine/Binaries/DotNET/AutoReporter.exe");

				FString IniDumpPath;
				if (GGameName[0])
				{
					const TCHAR IniDumpFilename[] = TEXT("UnrealAutoReportIniDump");
					IniDumpPath = FPaths::CreateTempFilename(*FPaths::GameLogDir(), IniDumpFilename, TEXT(".txt"));
					//build the ini dump
					FOutputDeviceFile AutoReportIniFile(*IniDumpPath);
					GConfig->Dump(AutoReportIniFile);
					AutoReportIniFile.Flush();
					AutoReportIniFile.TearDown();
				}

				FString CrashVideoPath = FPaths::GameLogDir() + TEXT("CrashVideo.avi");

				//get the paths that the files will actually have been saved to
				FString UserIniDumpPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*IniDumpPath);
				FString LogDirectory = FPaths::GameLogDir();
				TCHAR CommandlineLogFile[MAX_SPRINTF]=TEXT("");

				//use the log file specified on the commandline if there is one
				if (FParse::Value(FCommandLine::Get(), TEXT("LOG="), CommandlineLogFile, ARRAY_COUNT(CommandlineLogFile)))
				{
					LogDirectory += CommandlineLogFile;
				}
				else if (FCString::Strlen(GGameName) != 0)
				{
					// If the app name is defined, use it as the log filename
					LogDirectory += FString::Printf(TEXT("%s.Log"), GGameName);
				}
				else
				{
					// Revert to hardcoded UE4.log
					LogDirectory += TEXT("UE4.Log");
				}

				FString UserLogFile = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*LogDirectory);
				FString UserReportDumpPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*ReportDumpPath);
				FString UserCrashVideoPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*CrashVideoPath);

				//start up the auto reporting app, passing the report dump file path, the games' log file,
				// the ini dump path, the minidump path, and the crashvideo path
				//protect against spaces in paths breaking them up on the commandline
				FString CallingCommandLine = FString::Printf(TEXT("%d \"%s\" \"%s\" \"%s\" \"%s\" \"%s\""),
					(uint32)(GetCurrentProcessId()), *UserReportDumpPath, *UserLogFile, *UserIniDumpPath, 
					MiniDumpFilenameW, *UserCrashVideoPath);

				switch( InMode )
				{
				case EErrorReportMode::Unattended:
					CallingCommandLine += TEXT( " -unattended" );
					break;

				case EErrorReportMode::Balloon:
					CallingCommandLine += TEXT( " -balloon" );
					break;
				}

				if (!FPlatformProcess::CreateProc(AutoReportExe, *CallingCommandLine, true, false, false, NULL, 0, NULL, NULL).IsValid())
				{
					UE_LOG(LogWindows, Warning, TEXT("Couldn't start up the Auto Reporting process!"));
					FPlatformMemory::DumpStats(*GWarn);
					FMessageDialog::Open( EAppMsgType::Ok, FText::FromString( InErrorHist ) );
				}
			}

			HardKillIfAutomatedTesting();
		}
	}
}

static void WinPumpMessages()
{
	{
		MSG Msg;
		while( PeekMessage(&Msg,NULL,0,0,PM_REMOVE) )
		{
			TranslateMessage( &Msg );
			DispatchMessage( &Msg );
		}
	}
}

static void WinPumpSentMessages()
{
	MSG Msg;
	PeekMessage(&Msg,NULL,0,0,PM_NOREMOVE | PM_QS_SENDMESSAGE);
}

void FWindowsPlatformMisc::PumpMessages(bool bFromMainLoop)
{
	if (!bFromMainLoop)
	{
		TGuardValue<bool> PumpMessageGuard( GPumpingMessagesOutsideOfMainLoop, true );
		// Process pending windows messages, which is necessary to the rendering thread in some rare cases where D3D
		// sends window messages (from IDXGISwapChain::Present) to the main thread owned viewport window.
		WinPumpSentMessages();
		return;
	}

	GPumpingMessagesOutsideOfMainLoop = false;
	WinPumpMessages();

	// check to see if the window in the foreground was made by this process (ie, does this app
	// have focus)
	::DWORD ForegroundProcess;
	GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundProcess);
	bool HasFocus = ForegroundProcess == GetCurrentProcessId();

	// If editor thread doesn't have the focus, don't suck up too much CPU time.
	if( GIsEditor )
	{
		static bool HadFocus=1;
		if( HadFocus && !HasFocus )
		{
			// Drop our priority to speed up whatever is in the foreground.
			SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
		}
		else if( HasFocus && !HadFocus )
		{
			// Boost our priority back to normal.
			SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
		}
		if( !HasFocus )
		{
			// Sleep for a bit to not eat up all CPU time.
			FPlatformProcess::Sleep(0.005f);
		}
		HadFocus = HasFocus;
	}

	// if its our window, allow sound, otherwise silence it
	GVolumeMultiplier = HasFocus ? 1.0f : 0.0f;
}

uint32 FWindowsPlatformMisc::GetCharKeyMap(uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
	return FGenericPlatformMisc::GetStandardPrintableKeyMap(KeyCodes, KeyNames, MaxMappings, true, false);
}

uint32 FWindowsPlatformMisc::GetKeyMap( uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings )
{
#define ADDKEYMAP(KeyCode, KeyName)		if (NumMappings<MaxMappings) { KeyCodes[NumMappings]=KeyCode; KeyNames[NumMappings]=KeyName; ++NumMappings; };

	uint32 NumMappings = 0;

	if ( KeyCodes && KeyNames && (MaxMappings > 0) )
	{
		ADDKEYMAP( VK_LBUTTON, TEXT("LeftMouseButton") );
		ADDKEYMAP( VK_RBUTTON, TEXT("RightMouseButton") );
		ADDKEYMAP( VK_MBUTTON, TEXT("MiddleMouseButton") );

		ADDKEYMAP( VK_XBUTTON1, TEXT("ThumbMouseButton") );
		ADDKEYMAP( VK_XBUTTON2, TEXT("ThumbMouseButton2") );

		ADDKEYMAP( VK_BACK, TEXT("BackSpace") );
		ADDKEYMAP( VK_TAB, TEXT("Tab") );
		ADDKEYMAP( VK_RETURN, TEXT("Enter") );
		ADDKEYMAP( VK_PAUSE, TEXT("Pause") );

		ADDKEYMAP( VK_CAPITAL, TEXT("CapsLock") );
		ADDKEYMAP( VK_ESCAPE, TEXT("Escape") );
		ADDKEYMAP( VK_SPACE, TEXT("SpaceBar") );
		ADDKEYMAP( VK_PRIOR, TEXT("PageUp") );
		ADDKEYMAP( VK_NEXT, TEXT("PageDown") );
		ADDKEYMAP( VK_END, TEXT("End") );
		ADDKEYMAP( VK_HOME, TEXT("Home") );

		ADDKEYMAP( VK_LEFT, TEXT("Left") );
		ADDKEYMAP( VK_UP, TEXT("Up") );
		ADDKEYMAP( VK_RIGHT, TEXT("Right") );
		ADDKEYMAP( VK_DOWN, TEXT("Down") );

		ADDKEYMAP( VK_INSERT, TEXT("Insert") );
		ADDKEYMAP( VK_DELETE, TEXT("Delete") );

		ADDKEYMAP( VK_NUMPAD0, TEXT("NumPadZero") );
		ADDKEYMAP( VK_NUMPAD1, TEXT("NumPadOne") );
		ADDKEYMAP( VK_NUMPAD2, TEXT("NumPadTwo") );
		ADDKEYMAP( VK_NUMPAD3, TEXT("NumPadThree") );
		ADDKEYMAP( VK_NUMPAD4, TEXT("NumPadFour") );
		ADDKEYMAP( VK_NUMPAD5, TEXT("NumPadFive") );
		ADDKEYMAP( VK_NUMPAD6, TEXT("NumPadSix") );
		ADDKEYMAP( VK_NUMPAD7, TEXT("NumPadSeven") );
		ADDKEYMAP( VK_NUMPAD8, TEXT("NumPadEight") );
		ADDKEYMAP( VK_NUMPAD9, TEXT("NumPadNine") );

		ADDKEYMAP( VK_MULTIPLY, TEXT("Multiply") );
		ADDKEYMAP( VK_ADD, TEXT("Add") );
		ADDKEYMAP( VK_SUBTRACT, TEXT("Subtract") );
		ADDKEYMAP( VK_DECIMAL, TEXT("Decimal") );
		ADDKEYMAP( VK_DIVIDE, TEXT("Divide") );

		ADDKEYMAP( VK_F1, TEXT("F1") );
		ADDKEYMAP( VK_F2, TEXT("F2") );
		ADDKEYMAP( VK_F3, TEXT("F3") );
		ADDKEYMAP( VK_F4, TEXT("F4") );
		ADDKEYMAP( VK_F5, TEXT("F5") );
		ADDKEYMAP( VK_F6, TEXT("F6") );
		ADDKEYMAP( VK_F7, TEXT("F7") );
		ADDKEYMAP( VK_F8, TEXT("F8") );
		ADDKEYMAP( VK_F9, TEXT("F9") );
		ADDKEYMAP( VK_F10, TEXT("F10") );
		ADDKEYMAP( VK_F11, TEXT("F11") );
		ADDKEYMAP( VK_F12, TEXT("F12") );

		ADDKEYMAP( VK_NUMLOCK, TEXT("NumLock") );

		ADDKEYMAP( VK_SCROLL, TEXT("ScrollLock") );

		ADDKEYMAP( VK_LSHIFT, TEXT("LeftShift") );
		ADDKEYMAP( VK_RSHIFT, TEXT("RightShift") );
		ADDKEYMAP( VK_LCONTROL, TEXT("LeftControl") );
		ADDKEYMAP( VK_RCONTROL, TEXT("RightControl") );
		ADDKEYMAP( VK_LMENU, TEXT("LeftAlt") );
		ADDKEYMAP( VK_RMENU, TEXT("RightAlt") );
		ADDKEYMAP( VK_LWIN, TEXT("LeftCommand") );
		ADDKEYMAP( VK_RWIN, TEXT("RightCommand") );
	}

	check(NumMappings < MaxMappings);
	return NumMappings;
}


void FWindowsPlatformMisc::LocalPrint( const TCHAR *Message )
{
	OutputDebugString(Message);
}

void FWindowsPlatformMisc::RequestExit( bool Force )
{
	UE_LOG(LogWindows, Log,  TEXT("FPlatformMisc::RequestExit(%i)"), Force );
	if( Force )
	{
		// Force immediate exit. In case of an error set the exit code to 3.
		// Dangerous because config code isn't flushed, global destructors aren't called, etc.
		// Suppress abort message and MS reports.
		//_set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );
		//abort();
	
		// Make sure the log is flushed.
		if( GLog )
		{
			// This may be called from other thread, so set this thread as the master.
			GLog->SetCurrentThreadAsMasterThread();
			GLog->TearDown();
		}

		TerminateProcess(GetCurrentProcess(), GIsCriticalError ? 3 : 0); 
	}
	else
	{
		// Tell the platform specific code we want to exit cleanly from the main loop.
		PostQuitMessage( 0 );
		GIsRequestingExit = 1;
	}
}

const TCHAR* FWindowsPlatformMisc::GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error)
{
	check(OutBuffer && BufferCount);
	*OutBuffer = TEXT('\0');
	if (Error == 0)
	{
		Error = GetLastError();
	}
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), OutBuffer, BufferCount, NULL );
	TCHAR* Found = FCString::Strchr(OutBuffer,TEXT('\r'));
	if (Found)
	{
		*Found = TEXT('\0');
	}
	Found = FCString::Strchr(OutBuffer,TEXT('\n'));
	if (Found)
	{
		*Found = TEXT('\0');
	}
	return OutBuffer;
}

// Disabling optimizations helps to reduce the frequency of OpenClipboard failing with error code 0. It still happens
// though only with really large text buffers and we worked around this by changing the editor to use an intermediate
// text buffer for internal operations.
PRAGMA_DISABLE_OPTIMIZATION 

void FWindowsPlatformMisc::ClipboardCopy(const TCHAR* Str)
{
	if( OpenClipboard(GetActiveWindow()) )
	{
		verify(EmptyClipboard());
		HGLOBAL GlobalMem;
		int32 StrLen = FCString::Strlen(Str);
		GlobalMem = GlobalAlloc( GMEM_MOVEABLE, sizeof(TCHAR)*(StrLen+1) );
		check(GlobalMem);
		TCHAR* Data = (TCHAR*) GlobalLock( GlobalMem );
		FCString::Strcpy( Data, (StrLen+1), Str );
		GlobalUnlock( GlobalMem );
		if( SetClipboardData( CF_UNICODETEXT, GlobalMem ) == NULL )
			UE_LOG(LogWindows, Fatal,TEXT("SetClipboardData failed with error code %i"), (uint32)GetLastError() );
		verify(CloseClipboard());
	}
}

void FWindowsPlatformMisc::ClipboardPaste(class FString& Result)
{
	if( OpenClipboard(GetActiveWindow()) )
	{
		HGLOBAL GlobalMem = NULL;
		bool Unicode = 0;
		GlobalMem = GetClipboardData( CF_UNICODETEXT );
		Unicode = 1;
		if( !GlobalMem )
		{
			GlobalMem = GetClipboardData( CF_TEXT );
			Unicode = 0;
		}
		if( !GlobalMem )
		{
			Result = TEXT("");
		}
		else
		{
			void* Data = GlobalLock( GlobalMem );
			check( Data );	
			if( Unicode )
				Result = (TCHAR*) Data;
			else
			{
				ANSICHAR* ACh = (ANSICHAR*) Data;
				int32 i;
				for( i=0; ACh[i]; i++ );
				TArray<TCHAR> Ch;
				Ch.AddUninitialized(i+1);
				for( i=0; i<Ch.Num(); i++ )
					Ch[i]=CharCast<TCHAR>(ACh[i]);
				Result.GetCharArray() = MoveTemp(Ch);
			}
			GlobalUnlock( GlobalMem );
		}
		verify(CloseClipboard());
	}
	else 
	{
		Result=TEXT("");
	}
}

PRAGMA_ENABLE_OPTIMIZATION 

void FWindowsPlatformMisc::CreateGuid(FGuid& Result)
{
	verify( CoCreateGuid( (GUID*)&Result )==S_OK );
}


#define HOTKEY_YES			100
#define HOTKEY_NO			101
#define HOTKEY_CANCEL		102

/**
 * Helper global variables, used in MessageBoxDlgProc for set message text.
 */
static TCHAR* GMessageBoxText = NULL;
static TCHAR* GMessageBoxCaption = NULL;
/**
 * Used by MessageBoxDlgProc to indicate whether a 'Cancel' button is present and
 * thus 'Esc should be accepted as a hotkey.
 */
static bool GCancelButtonEnabled = false;

/**
 * Calculates button position and size, localize button text.
 * @param HandleWnd handle to dialog window
 * @param Text button text to localize
 * @param DlgItemId dialog item id
 * @param PositionX current button position (x coord)
 * @param PositionY current button position (y coord)
 * @return true if succeeded
 */
static bool SetDlgItem( HWND HandleWnd, const TCHAR* Text, int32 DlgItemId, int32* PositionX, int32* PositionY )
{
	SIZE SizeButton;
		
	HDC DC = CreateCompatibleDC( NULL );
	GetTextExtentPoint32( DC, Text, wcslen(Text), &SizeButton );
	DeleteDC(DC);
	DC = NULL;

	SizeButton.cx += 14;
	SizeButton.cy += 8;

	HWND Handle = GetDlgItem( HandleWnd, DlgItemId );
	if( Handle )
	{
		*PositionX -= ( SizeButton.cx + 5 );
		SetWindowPos( Handle, HWND_TOP, *PositionX, *PositionY - SizeButton.cy, SizeButton.cx, SizeButton.cy, 0 );
		SetDlgItemText( HandleWnd, DlgItemId, Text );
		
		return true;
	}

	return false;
}

/**
 * Callback for MessageBoxExt dialog (allowing for Yes to all / No to all )
 * @return		One of EAppReturnType::Yes, EAppReturnType::YesAll, EAppReturnType::No, EAppReturnType::NoAll, EAppReturnType::Cancel.
 */
PTRINT CALLBACK MessageBoxDlgProc( HWND HandleWnd, uint32 Message, WPARAM WParam, LPARAM LParam )
{
	switch(Message)
	{
		case WM_INITDIALOG:
			{
				// Sets most bottom and most right position to begin button placement
				RECT Rect;
				POINT Point;
				
				GetWindowRect( HandleWnd, &Rect );
				Point.x = Rect.right;
				Point.y = Rect.bottom;
				ScreenToClient( HandleWnd, &Point );
				
				int32 PositionX = Point.x - 8;
				int32 PositionY = Point.y - 10;

				// Localize dialog buttons, sets position and size.
				FString CancelString;
				FString NoToAllString;
				FString NoString;
				FString YesToAllString;
				FString YesString;

				// The Localize* functions will return the Key if a dialog is presented before the config system is initialized.
				// Instead, we use hard-coded strings if config is not yet initialized.
				if( !GConfig )
				{
					CancelString = TEXT("Cancel");
					NoToAllString = TEXT("No to All");
					NoString = TEXT("No");
					YesToAllString = TEXT("Yes to All");
					YesString = TEXT("Yes");
				}
				else
				{
					CancelString = NSLOCTEXT("UnrealEd", "Cancel", "Cancel").ToString();
					NoToAllString = NSLOCTEXT("UnrealEd", "NoToAll", "No to All").ToString();
					NoString = NSLOCTEXT("UnrealEd", "No", "No").ToString();
					YesToAllString = NSLOCTEXT("UnrealEd", "YesToAll", "Yes to All").ToString();
					YesString = NSLOCTEXT("UnrealEd", "Yes", "Yes").ToString();
				}
				SetDlgItem( HandleWnd, *CancelString, IDC_CANCEL, &PositionX, &PositionY );
				SetDlgItem( HandleWnd, *NoToAllString, IDC_NOTOALL, &PositionX, &PositionY );
				SetDlgItem( HandleWnd, *NoString, IDC_NO_B, &PositionX, &PositionY );
				SetDlgItem( HandleWnd, *YesToAllString, IDC_YESTOALL, &PositionX, &PositionY );
				SetDlgItem( HandleWnd, *YesString, IDC_YES, &PositionX, &PositionY );

				SetDlgItemText( HandleWnd, IDC_MESSAGE, GMessageBoxText );
				SetWindowText( HandleWnd, GMessageBoxCaption );

				// If parent window exist, get it handle and make it foreground.
				HWND ParentWindow = GetTopWindow( HandleWnd );
				if( ParentWindow )
				{
					SetWindowPos( ParentWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
				}

				SetForegroundWindow( HandleWnd );
				SetWindowPos( HandleWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

				RegisterHotKey( HandleWnd, HOTKEY_YES, 0, 'Y' );
				RegisterHotKey( HandleWnd, HOTKEY_NO, 0, 'N' );
				if ( GCancelButtonEnabled )
				{
					RegisterHotKey( HandleWnd, HOTKEY_CANCEL, 0, VK_ESCAPE );
				}

				// Windows are foreground, make them not top most.
				SetWindowPos( HandleWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
				if( ParentWindow )
				{
					SetWindowPos( ParentWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
				}

				return true;
			}
		case WM_DESTROY:
			{
				UnregisterHotKey( HandleWnd, HOTKEY_YES );
				UnregisterHotKey( HandleWnd, HOTKEY_NO );
				if ( GCancelButtonEnabled )
				{
					UnregisterHotKey( HandleWnd, HOTKEY_CANCEL );
				}
				return true;
			}
		case WM_COMMAND:
			switch( LOWORD( WParam ) )
			{
				case IDC_YES:
					EndDialog( HandleWnd, EAppReturnType::Yes );
					break;
				case IDC_YESTOALL:
					EndDialog( HandleWnd, EAppReturnType::YesAll );
					break;
				case IDC_NO_B:
					EndDialog( HandleWnd, EAppReturnType::No );
					break;
				case IDC_NOTOALL:
					EndDialog( HandleWnd, EAppReturnType::NoAll );
					break;
				case IDC_CANCEL:
					if ( GCancelButtonEnabled )
					{
						EndDialog( HandleWnd, EAppReturnType::Cancel );
					}
					break;
			}
			break;
		case WM_HOTKEY:
			switch( WParam )
			{
			case HOTKEY_YES:
				EndDialog( HandleWnd, EAppReturnType::Yes );
				break;
			case HOTKEY_NO:
				EndDialog( HandleWnd, EAppReturnType::No );
				break;
			case HOTKEY_CANCEL:
				if ( GCancelButtonEnabled )
				{
					EndDialog( HandleWnd, EAppReturnType::Cancel );
				}
				break;
			}
			break;
		default:
			return false;
	}
	return true;
}

/**
 * Displays extended message box allowing for YesAll/NoAll
 * @return 3 - YesAll, 4 - NoAll, -1 for Fail
 */
int MessageBoxExtInternal( EAppMsgType::Type MsgType, HWND HandleWnd, const TCHAR* Text, const TCHAR* Caption )
{
	GMessageBoxText = (TCHAR *) Text;
	GMessageBoxCaption = (TCHAR *) Caption;

	if( MsgType == EAppMsgType::YesNoYesAllNoAll )
	{
		GCancelButtonEnabled = false;
		return DialogBox( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_YESNO2ALL), HandleWnd, MessageBoxDlgProc );
	}
	else if( MsgType == EAppMsgType::YesNoYesAllNoAllCancel )
	{
		GCancelButtonEnabled = true;
		return DialogBox( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_YESNO2ALLCANCEL), HandleWnd, MessageBoxDlgProc );
	}

	return -1;
}




EAppReturnType::Type FWindowsPlatformMisc::MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption )
{
	HWND ParentWindow = (HWND)NULL;
	switch( MsgType )
	{
	case EAppMsgType::YesNo:
		{
			int32 Return = MessageBox( ParentWindow, Text, Caption, MB_YESNO|MB_SYSTEMMODAL );
			return Return == IDYES ? EAppReturnType::Yes : EAppReturnType::No;
		}
	case EAppMsgType::OkCancel:
		{
			int32 Return = MessageBox( ParentWindow, Text, Caption, MB_OKCANCEL|MB_SYSTEMMODAL );
			return Return == IDOK ? EAppReturnType::Ok : EAppReturnType::Cancel;
		}
	case EAppMsgType::YesNoCancel:
		{
			int32 Return = MessageBox(ParentWindow, Text, Caption, MB_YESNOCANCEL | MB_ICONQUESTION | MB_SYSTEMMODAL);
			return Return == IDYES ? EAppReturnType::Yes : (Return == IDNO ? EAppReturnType::No : EAppReturnType::Cancel);
		}
	case EAppMsgType::CancelRetryContinue:
		{
			int32 Return = MessageBox(ParentWindow, Text, Caption, MB_CANCELTRYCONTINUE | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SYSTEMMODAL);
			return Return == IDCANCEL ? EAppReturnType::Cancel : (Return == IDTRYAGAIN ? EAppReturnType::Retry : EAppReturnType::Continue);
		}
		break;
	case EAppMsgType::YesNoYesAllNoAll:
		return (EAppReturnType::Type)MessageBoxExtInternal( EAppMsgType::YesNoYesAllNoAll, ParentWindow, Text, Caption );
		//These return codes just happen to match up with ours.
		// return 0 for No, 1 for Yes, 2 for YesToAll, 3 for NoToAll
		break;
	case EAppMsgType::YesNoYesAllNoAllCancel:
		return (EAppReturnType::Type)MessageBoxExtInternal( EAppMsgType::YesNoYesAllNoAllCancel, ParentWindow, Text, Caption );
		//These return codes just happen to match up with ours.
		// return 0 for No, 1 for Yes, 2 for YesToAll, 3 for NoToAll, 4 for Cancel
		break;
	default:
		MessageBox( ParentWindow, Text, Caption, MB_OK|MB_SYSTEMMODAL );
		break;
	}
	return EAppReturnType::Cancel;
}

#if WITH_ENGINE
void FWindowsPlatformMisc::ShowGameWindow( void* StaticWndProc )
{
	if ( FPlatformProperties::SupportsWindowedMode() && GGameWindow && GGameWindowUsingStartupWindowProc )
	{
		// Convert position from screen coordinates to workspace coordinates.
		HMONITOR Monitor = MonitorFromWindow(GGameWindow, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO MonitorInfo;
		MonitorInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo( Monitor, &MonitorInfo );
		int32 PosX = GGameWindowPosX - MonitorInfo.rcWork.left;
		int32 PosY = GGameWindowPosY - MonitorInfo.rcWork.top;

		// Clear out old messages using the old StartupWindowProc
		WinPumpMessages();

		SetWindowLong(GGameWindow, GWL_STYLE, GGameWindowStyle);
		SetWindowLongPtr(GGameWindow, GWLP_WNDPROC, (LONG_PTR)StaticWndProc);
		GGameWindowUsingStartupWindowProc = false;

		// Restore the minimized window to the correct position, size and styles.
		WINDOWPLACEMENT Placement;
		FMemory::Memzero(&Placement, sizeof(WINDOWPLACEMENT));
		Placement.length					= sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(GGameWindow, &Placement);
		Placement.flags						= 0;
		Placement.showCmd					= SW_SHOWNORMAL;	// Restores the minimized window (SW_SHOW won't do that)
		Placement.rcNormalPosition.left		= PosX;
		Placement.rcNormalPosition.right	= PosX + GGameWindowWidth;
		Placement.rcNormalPosition.top		= PosY;
		Placement.rcNormalPosition.bottom	= PosY + GGameWindowHeight;
		SetWindowPlacement(GGameWindow, &Placement);
		UpdateWindow(GGameWindow);

		// Pump the messages using the new (correct) WindowProc
		WinPumpMessages();
	}
}
#endif		// WITH_ENGINE


static bool HandleGameExplorerIntegration()
{
	if (FPlatformProperties::SupportsWindowedMode())
	{
		TCHAR AppPath[MAX_PATH];
		GetModuleFileName(NULL, AppPath, MAX_PATH - 1);

		// Initialize COM. We only want to do this once and not override settings of previous calls.
		if (!FWindowsPlatformMisc::CoInitialize())
		{
			return false;
		}
		
		// check to make sure we are able to run, based on parental rights
		IGameExplorer* GameExp;
		HRESULT hr = CoCreateInstance(__uuidof(GameExplorer), NULL, CLSCTX_INPROC_SERVER, __uuidof(IGameExplorer), (void**) &GameExp);

		BOOL bHasAccess = 1;
		BSTR AppPathBSTR = SysAllocString(AppPath);

		// @todo: This will allow access if the CoCreateInstance fails, but it should probaly disallow 
		// access if OS is Vista and it fails, succeed for XP
		if (SUCCEEDED(hr) && GameExp)
		{
			GameExp->VerifyAccess(AppPathBSTR, &bHasAccess);
		}


		// Guid for testing GE (un)installation
		static const GUID GEGuid = 
		{ 0x7089dd1d, 0xfe97, 0x4cc8, { 0x8a, 0xac, 0x26, 0x3e, 0x44, 0x1f, 0x3c, 0x42 } };

		// add the game to the game explorer if desired
		if (FParse::Param( FCommandLine::Get(), TEXT("installge")))
		{
			if (bHasAccess && GameExp)
			{
				BSTR AppDirBSTR = SysAllocString(FPlatformProcess::BaseDir());
				GUID Guid = GEGuid;
				hr = GameExp->AddGame(AppPathBSTR, AppDirBSTR, FParse::Param( FCommandLine::Get(), TEXT("allusers")) ? GIS_ALL_USERS : GIS_CURRENT_USER, &Guid);

				bool bWasSuccessful = false;
				// if successful
				if (SUCCEEDED(hr))
				{
					// get location of app local dir
					TCHAR UserPath[MAX_PATH];
					SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, UserPath);

					// convert guid to a string
					TCHAR GuidDir[MAX_PATH];
					StringFromGUID2(GEGuid, GuidDir, MAX_PATH - 1);

					// make the base path for all tasks
					FString BaseTaskDirectory = FString(UserPath) + TEXT("\\Microsoft\\Windows\\GameExplorer\\") + GuidDir;

					// make full paths for play and support tasks
					FString PlayTaskDirectory = BaseTaskDirectory + TEXT("\\PlayTasks");
					FString SupportTaskDirectory = BaseTaskDirectory + TEXT("\\SupportTasks");
				
					// make sure they exist
					IFileManager::Get().MakeDirectory(*PlayTaskDirectory, true);
					IFileManager::Get().MakeDirectory(*SupportTaskDirectory, true);

					// interface for creating a shortcut
					IShellLink* Link;
					hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,	IID_IShellLink, (void**)&Link);

					// get the persistent file interface of the link
					IPersistFile* LinkFile;
					Link->QueryInterface(IID_IPersistFile, (void**)&LinkFile);

					Link->SetPath(AppPath);

					// create all of our tasks

					// first is just the game
					Link->SetArguments(TEXT(""));
					Link->SetDescription(TEXT("Play"));
					IFileManager::Get().MakeDirectory(*(PlayTaskDirectory + TEXT("\\0")), true);
					LinkFile->Save(*(PlayTaskDirectory + TEXT("\\0\\Play.lnk")), true);

					Link->SetArguments(TEXT("editor"));
					Link->SetDescription(TEXT("Editor"));
					IFileManager::Get().MakeDirectory(*(PlayTaskDirectory + TEXT("\\1")), true);
					LinkFile->Save(*(PlayTaskDirectory + TEXT("\\1\\Editor.lnk")), true);

					LinkFile->Release();
					Link->Release();

					IUniformResourceLocator* InternetLink;
					CoCreateInstance (CLSID_InternetShortcut, NULL, 
						CLSCTX_INPROC_SERVER, IID_IUniformResourceLocator, (LPVOID*) &InternetLink);

					InternetLink->QueryInterface(IID_IPersistFile, (void**)&LinkFile);

					// make an internet shortcut
					InternetLink->SetURL(TEXT("http://www.unrealtournament3.com/"), 0);
					IFileManager::Get().MakeDirectory(*(SupportTaskDirectory + TEXT("\\0")), true);
					LinkFile->Save(*(SupportTaskDirectory + TEXT("\\0\\UT3.url")), true);

					LinkFile->Release();
					InternetLink->Release();
				}

				if ( SUCCEEDED(hr) ) 
				{
					FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("WindowsPlatform", "GameExplorerInstallationSuccessful", "GameExplorer installation was successful, quitting now.") );
				}
				else
				{
					FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("WindowsPlatform", "GameExplorerInstallationFailed", "GameExplorer installation was a failure, quitting now.") );
				}

				SysFreeString(AppDirBSTR);
			}
			else
			{
				FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("WindowsPlatform", "GameExplorerInstallationFailedDoToAccessPermissions", "GameExplorer installation failed because you don't have access (check parental control levels and that you are running XP). You should not need Admin access"));
			}

			// free the string and shutdown COM
			SysFreeString(AppPathBSTR);
			SAFE_RELEASE(GameExp);
			FWindowsPlatformMisc::CoUninitialize();

			return false;
		}
		else if (FParse::Param( FCommandLine::Get(), TEXT("uninstallge")))
		{
			if (GameExp)
			{
				hr = GameExp->RemoveGame(GEGuid);
				if ( SUCCEEDED(hr) ) 
				{
					FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("WindowsPlatform", "GameExplorerUninstallationSuccessful", "GameExplorer uninstallation was successful, quitting now.") );
				}
				else
				{
					FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("WindowsPlatform", "GameExplorerUninstallationFailed", "GameExplorer uninstallation was a failure, quitting now.") );
				}
			}
			else
			{
				FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("WindowsPlatform", "GameExplorerUninstallationFailedDoToNotRunningVista", "GameExplorer uninstallation failed because you are probably not running Vista."));
			}

			// free the string and shutdown COM
			SysFreeString(AppPathBSTR);
			SAFE_RELEASE(GameExp);
			FWindowsPlatformMisc::CoUninitialize();

			return false;
		}

		// free the string and shutdown COM
		SysFreeString(AppPathBSTR);
		SAFE_RELEASE(GameExp);
		FWindowsPlatformMisc::CoUninitialize();

		// if we don't have access, we must quit ASAP after showing a message
		if (!bHasAccess)
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_ParentalControls", "The current level of parental controls do not allow you to run this game." ) );
			return false;
		}
	}
	return true;
}

#if WITH_FIREWALL_SUPPORT
/** 
 * Get the INetFwProfile interface for current profile
 */
INetFwProfile* GetFirewallProfile( void )
{
	HRESULT hr;
	INetFwMgr* pFwMgr = NULL;
	INetFwPolicy* pFwPolicy = NULL;
	INetFwProfile* pFwProfile = NULL;

	// Create an instance of the Firewall settings manager
	hr = CoCreateInstance( __uuidof( NetFwMgr ), NULL, CLSCTX_INPROC_SERVER, __uuidof( INetFwMgr ), ( void** )&pFwMgr );
	if( SUCCEEDED( hr ) )
	{
		hr = pFwMgr->get_LocalPolicy( &pFwPolicy );
		if( SUCCEEDED( hr ) )
		{
			pFwPolicy->get_CurrentProfile( &pFwProfile );
		}
	}

	// Cleanup
	if( pFwPolicy )
	{
		pFwPolicy->Release();
	}
	if( pFwMgr )
	{
		pFwMgr->Release();
	}

	return( pFwProfile );
}
#endif

static bool HandleFirewallIntegration()
{
#if WITH_FIREWALL_SUPPORT
	// only do with with the given commandlines
	if( !(FParse::Param( FCommandLine::Get(), TEXT( "installfw" ) ) || FParse::Param( FCommandLine::Get(), TEXT( "uninstallfw" ) )) )
#endif
	{
		return true; // allow the game to continue;
	}
#if WITH_FIREWALL_SUPPORT

	TCHAR AppPath[MAX_PATH];

	GetModuleFileName( NULL, AppPath, MAX_PATH - 1 );
	BSTR bstrGameExeFullPath = SysAllocString( AppPath );
	BSTR bstrFriendlyAppName = SysAllocString( TEXT( "Unreal Tournament 3" ) );

	if( bstrGameExeFullPath && bstrFriendlyAppName )
	{
		HRESULT hr = S_OK;
				
		if( FWindowsPlatformMisc::CoInitialize() )
		{
			INetFwProfile* pFwProfile = GetFirewallProfile();
			if( pFwProfile )
			{
				INetFwAuthorizedApplications* pFwApps = NULL;

				hr = pFwProfile->get_AuthorizedApplications( &pFwApps );
				if( SUCCEEDED( hr ) && pFwApps ) 
				{
					// add the game to the game explorer if desired
					if( FParse::Param( CmdLine, TEXT( "installfw" ) ) )
					{
						INetFwAuthorizedApplication* pFwApp = NULL;

						// Create an instance of an authorized application.
						hr = CoCreateInstance( __uuidof( NetFwAuthorizedApplication ), NULL, CLSCTX_INPROC_SERVER, __uuidof( INetFwAuthorizedApplication ), ( void** )&pFwApp );
						if( SUCCEEDED( hr ) && pFwApp )
						{
							// Set the process image file name.
							hr = pFwApp->put_ProcessImageFileName( bstrGameExeFullPath );
							if( SUCCEEDED( hr ) )
							{
								// Set the application friendly name.
								hr = pFwApp->put_Name( bstrFriendlyAppName );
								if( SUCCEEDED( hr ) )
								{
									// Add the application to the collection.
									hr = pFwApps->Add( pFwApp );
								}
							}

							pFwApp->Release();
						}
					}
					else if( FParse::Param( CmdLine, TEXT( "uninstallfw" ) ) )
					{
						// Remove the application from the collection.
						hr = pFwApps->Remove( bstrGameExeFullPath );
					}

					pFwApps->Release();
				}

				pFwProfile->Release();
			}

			FWindowsPlatformMisc::CoUninitialize();
		}

		SysFreeString( bstrFriendlyAppName );
		SysFreeString( bstrGameExeFullPath );
	}
	return false; // terminate the game
#endif // WITH_FIREWALL_SUPPORT
}

static bool HandleFirstInstall()
{
	if (FParse::Param( FCommandLine::Get(), TEXT("firstinstall")))
	{
		GLog->Flush();

		// Flush config to ensure culture changes are written to disk.
		GConfig->Flush(false);

		return false; // terminate the game
	}
	return true; // allow the game to continue;
}

bool FWindowsPlatformMisc::CommandLineCommands()
{
	return HandleFirstInstall() && HandleGameExplorerIntegration() && HandleFirewallIntegration();
}

/**
 * Detects whether we're running in a 64-bit operating system.
 *
 * @return	true if we're running in a 64-bit operating system
 */
bool FWindowsPlatformMisc::Is64bitOperatingSystem()
{
#if defined(PLATFORM_64BITS)
	return true;
#else
	#pragma warning( push )
	#pragma warning( disable: 4191 )	// unsafe conversion from 'type of expression' to 'type required'
	typedef bool (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress( GetModuleHandle(TEXT("kernel32")), "IsWow64Process" );
	bool bIsWoW64Process = false;
	if ( fnIsWow64Process != NULL )
	{
		if ( fnIsWow64Process(GetCurrentProcess(), (PBOOL)&bIsWoW64Process) == 0 )
		{
			bIsWoW64Process = false;
		}
	}
	#pragma warning( pop )
	return bIsWoW64Process;
#endif
}

int32 FWindowsPlatformMisc::GetAppIcon()
{
	return IDICON_UE4Game;
}

bool FWindowsPlatformMisc::VerifyWindowsMajorVersion(uint32 MajorVersion)
{
	OSVERSIONINFOEX Version;
	Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	Version.dwMajorVersion = MajorVersion;
	ULONGLONG ConditionMask = 0;
	return !!VerifyVersionInfo( &Version, VER_MAJORVERSION, VerSetConditionMask(ConditionMask,VER_MAJORVERSION,VER_GREATER_EQUAL) );
}

bool FWindowsPlatformMisc::IsValidAbsolutePathFormat(const FString& Path)
{
	bool bIsValid = true;
	const FString OnlyPath = FPaths::GetPath(Path);
	if ( OnlyPath.IsEmpty() )
	{
		bIsValid = false;
	}

	// Must begin with a drive letter
	if ( bIsValid && !FChar::IsAlpha(OnlyPath[0]) )
	{
		bIsValid = false;
	}

	// On Windows the path must be absolute, i.e: "D:/" or "D:\\"
	if ( bIsValid && !(Path.Find(TEXT(":/"))==1 || Path.Find(TEXT(":\\"))==1) )
	{
		bIsValid = false;
	}

	// Find any unnamed directory changes
	if ( bIsValid && (Path.Find(TEXT("//"))!=INDEX_NONE) || (Path.Find(TEXT("\\/"))!=INDEX_NONE) || (Path.Find(TEXT("/\\"))!=INDEX_NONE) || (Path.Find(TEXT("\\\\"))!=INDEX_NONE) )
	{
		bIsValid = false;
	}

	// ensure there's no further instances of ':' in the string
	if ( bIsValid && !(Path.Find(TEXT(":"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 2)==INDEX_NONE) )
	{
		bIsValid = false;
	}

	return bIsValid;
}

int32 FWindowsPlatformMisc::NumberOfCores()
{
	static int32 CoreCount = 0;
	if (CoreCount == 0)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("usehyperthreading")))
		{
			CoreCount = NumberOfCoresIncludingHyperthreads();
		}
		else
		{
			// Get only physical cores
			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION InfoBuffer = NULL;
			::DWORD BufferSize = 0;

			// Get the size of the buffer to hold processor information.
			::BOOL Result = GetLogicalProcessorInformation(InfoBuffer, &BufferSize);
			check(!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
			check(BufferSize > 0);

			// Allocate the buffer to hold the processor info.
			InfoBuffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)FMemory::Malloc(BufferSize);
			check(InfoBuffer);

			// Get the actual information.
			Result = GetLogicalProcessorInformation(InfoBuffer, &BufferSize);
			check(Result);

			// Count physical cores
			const int32 InfoCount = (int32)(BufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
			for (int32 Index = 0; Index < InfoCount; ++Index)
			{
				SYSTEM_LOGICAL_PROCESSOR_INFORMATION* Info = &InfoBuffer[Index];
				if (Info->Relationship ==  RelationProcessorCore)
				{
					CoreCount++;
				}
			}
			FMemory::Free(InfoBuffer);
		}
	}
	return CoreCount;
}

int32 FWindowsPlatformMisc::NumberOfCoresIncludingHyperthreads()
{
	static int32 CoreCount = 0;
	if (CoreCount == 0)
	{
		// Get the number of logical processors, including hyperthreaded ones.
		SYSTEM_INFO SI;
		GetSystemInfo(&SI);
		CoreCount = (int32)SI.dwNumberOfProcessors;
	}
	return CoreCount;
}

void FWindowsPlatformMisc::LoadPreInitModules()
{
	FModuleManager::Get().LoadModule(TEXT("D3D11RHI"));
	FModuleManager::Get().LoadModule(TEXT("OpenGLDrv"));
}

void FWindowsPlatformMisc::LoadStartupModules()
{
	FModuleManager::Get().LoadModule(TEXT("XAudio2"));
	FModuleManager::Get().LoadModule(TEXT("HeadMountedDisplay"));

#if WITH_EDITOR
	FModuleManager::Get().LoadModule(TEXT("VSAccessor"));
#endif	//WITH_EDITOR
}

const TCHAR* FWindowsPlatformMisc::GetNullRHIShaderFormat()
{
	return TEXT("PCD3D_SM5");
}

bool FWindowsPlatformMisc::OsExecute(const TCHAR* CommandType, const TCHAR* Command)
{
	HINSTANCE hApp = ShellExecute(NULL,
		CommandType,
		Command,
		NULL,
		NULL,
		SW_SHOWNORMAL);
	bool bSucceeded = hApp > (HINSTANCE)32;
	CloseHandle(hApp);
	return bSucceeded;
}

bool FWindowsPlatformMisc::GetWindowTitleMatchingText(const TCHAR* TitleStartsWith, FString& OutTitle)
{
	bool bWasFound = false;
	WCHAR Buffer[8192];
	// Get the first window so we can start walking the window chain
	HWND hWnd = FindWindow(NULL,NULL);
	if (hWnd != NULL)
	{
		do
		{
			GetWindowText(hWnd,Buffer,8192);
			// If this matches, then grab the full text
			if (_tcsnccmp(TitleStartsWith, Buffer, _tcslen(TitleStartsWith)) == 0)
			{
				OutTitle = Buffer;
				hWnd = NULL;
				bWasFound = true;
			}
			else
			{
				// Get the next window to interrogate
				hWnd = GetWindow(hWnd, GW_HWNDNEXT);
			}
		}
		while (hWnd != NULL);
	}
	return bWasFound;
}

void FWindowsPlatformMisc::RaiseException( uint32 ExceptionCode )
{
	::RaiseException( ExceptionCode, 0, 0, NULL );
}

uint32 FWindowsPlatformMisc::GetLastError()
{
	return (uint32)::GetLastError();
}

bool FWindowsPlatformMisc::CoInitialize()
{
	HRESULT hr = ::CoInitialize(NULL);
	return hr == S_OK || hr == S_FALSE;
}

void FWindowsPlatformMisc::CoUninitialize()
{
	::CoUninitialize();
}

#if !UE_BUILD_SHIPPING
static TCHAR GErrorRemoteDebugPromptMessage[4096];

void FWindowsPlatformMisc::PromptForRemoteDebugging(bool bIsEnsure)
{
	if (bShouldPromptForRemoteDebugging)
	{
		if (bIsEnsure && !bPromptForRemoteDebugOnEnsure)
		{
			// Don't prompt on ensures unless overridden
			return;
		}

		FCString::Sprintf(GErrorRemoteDebugPromptMessage, 
			TEXT("Have a programmer remote debug this crash?\n")
			TEXT("Hit NO to exit and submit error report as normal.\n")
			TEXT("Otherwise, contact a programmer for remote debugging,\n")
			TEXT("giving him the changelist number below.\n")
			TEXT("Once he confirms he is connected to the machine,\n")
			TEXT("hit YES to allow him to debug the crash.\n")
			TEXT("[Changelist = %d]"),
			GEngineVersion.GetChangelist());
		if (MessageBox(0, GErrorRemoteDebugPromptMessage, TEXT("CRASHED"), MB_YESNO|MB_SYSTEMMODAL) == IDYES)
		{
			::DebugBreak();
		}
	}
}
#endif	//#if !UE_BUILD_SHIPPING

FLinearColor FWindowsPlatformMisc::GetScreenPixelColor(const FVector2D& InScreenPos, float InGamma)
{
	COLORREF PixelColorRef = GetPixel(GetDC(HWND_DESKTOP), InScreenPos.X, InScreenPos.Y);

	const float DivideBy255 = 1.0f / 255.0f;

	FLinearColor ScreenColor(
		(PixelColorRef & 0xFF) * DivideBy255,
		((PixelColorRef & 0xFF00) >> 8) * DivideBy255,
		((PixelColorRef & 0xFF0000) >> 16) * DivideBy255,
		1.0f);

	if (InGamma > 1.0f)
	{
		// Correct for render gamma
		ScreenColor.R = FMath::Pow(ScreenColor.R, InGamma);
		ScreenColor.G = FMath::Pow(ScreenColor.G, InGamma);
		ScreenColor.B = FMath::Pow(ScreenColor.B, InGamma);
	}

	return ScreenColor;
}

bool FWindowsPlatformMisc::HasCPUIDInstruction()
{
#if PLATFORM_SEH_EXCEPTIONS_DISABLED
	return false;
#else
	__try
	{
		int Args[4];
		__cpuid(Args, 0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return true;
#endif
}

FString FWindowsPlatformMisc::GetCPUVendor()
{
	union
	{
		char Buffer[12+1];
		struct
		{
			int dw0;
			int dw1;
			int dw2;
		} Dw;
	} VendorResult;
	

	FString Vendor;
	if (HasCPUIDInstruction())
	{
		int Args[4];
		__cpuid(Args, 0);

		VendorResult.Dw.dw0 = Args[1];
		VendorResult.Dw.dw1 = Args[3];
		VendorResult.Dw.dw2 = Args[2];
		VendorResult.Buffer[12] = 0;

		Vendor = ANSI_TO_TCHAR(VendorResult.Buffer);
	}
	return Vendor;
}

bool FWindowsPlatformMisc::GetRegistryString(const FString& InRegistryKey, const FString& InValueName, bool bPerUserSetting, FString& OutValue)
{
	FString RegistryValue;
	FString FullRegistryKey = FString(TEXT("Software")) / InRegistryKey;
	FullRegistryKey = FullRegistryKey.Replace(TEXT("/"), TEXT("\\"));
	if (QueryRegKey(bPerUserSetting? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, *FullRegistryKey, *InValueName, RegistryValue) == true)
	{
		OutValue = RegistryValue;
		return true;
	}

	return false;
}

bool FWindowsPlatformMisc::QueryRegKey( const HKEY InKey, const TCHAR* InSubKey, const TCHAR* InValueName, FString& OutData )
{
	bool bSuccess = false;

	// Redirect key depending on system
	for (int32 RegistryIndex = 0; RegistryIndex < 2 && !bSuccess; ++RegistryIndex)
	{
		HKEY Key = 0;
		const uint32 RegFlags = (RegistryIndex == 0) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
		if (RegOpenKeyEx( InKey, InSubKey, 0, KEY_READ | RegFlags, &Key ) == ERROR_SUCCESS)
		{
			::DWORD Size = 0;
			// First, we'll call RegQueryValueEx to find out how large of a buffer we need
			if ((RegQueryValueEx( Key, InValueName, NULL, NULL, NULL, &Size ) == ERROR_SUCCESS) && Size)
			{
				// Allocate a buffer to hold the value and call the function again to get the data
				char *Buffer = new char[Size];
				if (RegQueryValueEx( Key, InValueName, NULL, NULL, (LPBYTE)Buffer, &Size ) == ERROR_SUCCESS)
				{
					OutData = FString( Size-1, (TCHAR*)Buffer );
					OutData.TrimToNullTerminator();
					bSuccess = true;
				}
				delete [] Buffer;
			}
			RegCloseKey( Key );
		}
	}

	return bSuccess;
}

FString FWindowsPlatformMisc::GetDefaultBrowser()
{
	// Default browser, should a custom one not be set
	FString Browser( TEXT("iexplore") );

	// Internet explorer doesn't modify the standard registry keys when it is made default browser again, check the program id to see if it should be overridden
	FString IEKey;
	if ( !QueryRegKey( HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice"), TEXT("Progid"), IEKey )
		|| !IEKey.StartsWith( TEXT( "IE." ) ) )	// If the key doesn't start with IE, then it doesn't use internet explorer either
	{
		// If it's being overridden, look up the actual application used to launch urls from the registry
		FString BrowserKey;
		if ( QueryRegKey( HKEY_CLASSES_ROOT, TEXT("http\\shell\\open\\command"), NULL, BrowserKey ) )
		{
			// Extract the path to the executable, don't use TrimQuotes as there may be params in the key too which we don't want
			const int32 FirstQuote = BrowserKey.Find( TEXT("\"") );
			if ( FirstQuote != INDEX_NONE )
			{
				const int32 SecondQuote = BrowserKey.Find( TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, FirstQuote+1 );
				if ( SecondQuote != INDEX_NONE )
				{
					Browser = BrowserKey.Mid( FirstQuote+1, (SecondQuote-1)-FirstQuote );
				}
			}
		}
	}

	return Browser;
}

int32 FWindowsPlatformMisc::GetCacheLineSize()
{
	int32 Result = 1;
	if (HasCPUIDInstruction())
	{
		int Args[4];
		__cpuid(Args, 0x80000006);

		Result = Args[2] & 0xFF;
		check(Result && !(Result & (Result - 1))); // assumed to be a power of two
	}
	return Result;
}
