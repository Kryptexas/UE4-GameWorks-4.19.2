// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacPlatformMisc.mm: Mac implementations of misc functions
=============================================================================*/

#include "CorePrivate.h"
#include "ExceptionHandling.h"
#include "SecureHash.h"
#include "VarargsHelper.h"
#include "MacApplication.h"
#include "MacCursor.h"
#include "Runtime/Launch/Resources/Version.h"
#include "EngineVersion.h"

#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>
#include <mach-o/dyld.h>
#include <libproc.h>

/** Width of the primary monitor, in pixels. */
CORE_API int32 GPrimaryMonitorWidth = 0;
/** Height of the primary monitor, in pixels. */
CORE_API int32 GPrimaryMonitorHeight = 0;
/** Rectangle of the work area on the primary monitor (excluding taskbar, etc) in "virtual screen coordinates" (pixels). */
CORE_API RECT GPrimaryMonitorWorkRect;
/** Virtual screen rectangle including all monitors. */
CORE_API RECT GVirtualScreenRect;


/** Settings for the game Window */
CORE_API NSWindow *GGameWindow = NULL;
CORE_API int32 GGameWindowPosX = 0;
CORE_API int32 GGameWindowPosY = 0;
CORE_API int32 GGameWindowWidth = 100;
CORE_API int32 GGameWindowHeight = 100;

CORE_API NSWindow* GExtendedAlertWindow = NULL;
CORE_API NSWindow* GEULAWindow = NULL;

/**
 * Information that cannot be obtained during a signal-handler is initialised here.
 * This ensures that we only call safe functions within the crash reporting handler.
 */
struct MacApplicationInfo
{
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
	
	void Init()
	{
		SCOPED_AUTORELEASE_POOL;
		
		NSDictionary* SystemVersion = [NSDictionary dictionaryWithContentsOfFile: @"/System/Library/CoreServices/SystemVersion.plist"];
		OSVersion = FString((NSString*)[SystemVersion objectForKey: @"ProductVersion"]);
		FCStringAnsi::Strcpy(OSVersionUTF8, PATH_MAX+1, [[SystemVersion objectForKey: @"ProductVersion"] UTF8String]);
		OSBuild = FString((NSString*)[SystemVersion objectForKey: @"ProductBuildVersion"]);
		
		char TempSysCtlBuffer[PATH_MAX] = {};
		size_t TempSysCtlBufferSize = PATH_MAX;
		
		pid_t ParentPID = getppid();
		proc_pidpath(ParentPID, TempSysCtlBuffer, PATH_MAX);
		ParentProcess = TempSysCtlBuffer;
		
		MachineUUID = TEXT("00000000-0000-0000-0000-000000000000");
		io_service_t PlatformExpert = IOServiceGetMatchingService(kIOMasterPortDefault,IOServiceMatching("IOPlatformExpertDevice"));
		if(PlatformExpert)
		{
			CFTypeRef SerialNumberAsCFString = IORegistryEntryCreateCFProperty(PlatformExpert,CFSTR(kIOPlatformUUIDKey),kCFAllocatorDefault, 0);
			if(SerialNumberAsCFString)
			{
				MachineUUID = FString((NSString*)SerialNumberAsCFString);
				CFRelease(SerialNumberAsCFString);
			}
			IOObjectRelease(PlatformExpert);
		}
		
		sysctlbyname("kern.osrelease", TempSysCtlBuffer, &TempSysCtlBufferSize, NULL, 0);
		BiosRelease = TempSysCtlBuffer;
		uint32 KernelRevision = 0;
		TempSysCtlBufferSize = 4;
		sysctlbyname("kern.osrevision", &KernelRevision, &TempSysCtlBufferSize, NULL, 0);
		BiosRevision = FString::Printf(TEXT("%d"), KernelRevision);
		TempSysCtlBufferSize = PATH_MAX;
		sysctlbyname("kern.uuid", TempSysCtlBuffer, &TempSysCtlBufferSize, NULL, 0);
		BiosUUID = TempSysCtlBuffer;
		TempSysCtlBufferSize = PATH_MAX;
		sysctlbyname("hw.model", TempSysCtlBuffer, &TempSysCtlBufferSize, NULL, 0);
		MachineModel = TempSysCtlBuffer;
		TempSysCtlBufferSize = PATH_MAX+1;
		sysctlbyname("machdep.cpu.brand_string", MachineCPUString, &TempSysCtlBufferSize, NULL, 0);
		
		FMacPlatformMisc::NumberOfCores();
		FMacPlatformMisc::NumberOfCoresIncludingHyperthreads();
		
		AppName = FApp::GetGameName();
		FCStringAnsi::Strcpy(AppNameUTF8, PATH_MAX+1, TCHAR_TO_UTF8(*AppName));
		
		gethostname(MachineName, ARRAY_COUNT(MachineName));
		
		FString CrashVideoPath = FPaths::GameLogDir() + TEXT("CrashVideo.avi");
		
		BranchBaseDir = FString::Printf(TEXT("%s!%s!%s!%d"), TEXT( BRANCH_NAME ), FPlatformProcess::BaseDir(), GetEngineMode(), BUILT_FROM_CHANGELIST);
		
		// Get the paths that the files will actually have been saved to
		FString LogDirectory = FPaths::GameLogDir();
		TCHAR CommandlineLogFile[MAX_SPRINTF]=TEXT("");
		
		// Use the log file specified on the commandline if there is one
		CommandLine = FCommandLine::Get();
		if (FParse::Value(*CommandLine, TEXT("LOG="), CommandlineLogFile, ARRAY_COUNT(CommandlineLogFile)))
		{
			LogDirectory += CommandlineLogFile;
		}
		else if (AppName.Len() != 0)
		{
			// If the app name is defined, use it as the log filename
			LogDirectory += FString::Printf(TEXT("%s.Log"), *AppName);
		}
		else
		{
			// Revert to hardcoded UE4.log
			LogDirectory += TEXT("UE4.Log");
		}
		FString LogPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*LogDirectory);
		FCStringAnsi::Strcpy(AppLogPath, PATH_MAX+1, TCHAR_TO_UTF8(*LogPath));

		FString UserCrashVideoPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*CrashVideoPath);
		FCStringAnsi::Strcpy(CrashReportVideo, PATH_MAX+1, TCHAR_TO_UTF8(*UserCrashVideoPath));
		
		FString ReportPath = FPaths::ConvertRelativePathToFull(FString::Printf(TEXT("%s"), *(FPaths::GameAgnosticSavedDir() / TEXT("Crashes"))));
		FCStringAnsi::Strcpy(CrashReportPath, PATH_MAX+1, TCHAR_TO_UTF8(*ReportPath));
		FString ReportClient = FPaths::ConvertRelativePathToFull(FPlatformProcess::GenerateApplicationPath(TEXT("CrashReportClient"), FApp::GetBuildConfiguration()));
		FCStringAnsi::Strcpy(CrashReportClient, PATH_MAX+1, TCHAR_TO_UTF8(*ReportClient));
		
		AppPath = FPlatformProcess::GenerateApplicationPath(FApp::GetName(), FApp::GetBuildConfiguration());
		
		LCID = FString::Printf(TEXT("%d"), FInternationalization::GetCurrentCulture()->GetLCID());
	}
	
	char AppNameUTF8[PATH_MAX+1];
	char AppLogPath[PATH_MAX+1];
	char CrashReportPath[PATH_MAX+1];
	char CrashReportClient[PATH_MAX+1];
	char CrashReportVideo[PATH_MAX+1];
	char OSVersionUTF8[PATH_MAX+1];
	char MachineName[PATH_MAX+1];
	char MachineCPUString[PATH_MAX+1];
	FString AppPath;
	FString AppName;
	FString OSVersion;
	FString OSBuild;
	FString MachineUUID;
	FString MachineModel;
	FString BiosRelease;
	FString BiosRevision;
	FString BiosUUID;
	FString ParentProcess;
	FString LCID;
	FString CommandLine;
	FString BranchBaseDir;
};
static MacApplicationInfo GMacAppInfo;

void FMacPlatformMisc::PlatformPreInit()
{
#if WITH_ENGINE
	SCOPED_AUTORELEASE_POOL;

	NSArray* AllScreens = [NSScreen screens];
	NSScreen* PrimaryScreen = (NSScreen*)[AllScreens objectAtIndex: 0];

	NSRect ScreenFrame = [PrimaryScreen frame];
	NSRect VisibleFrame = [PrimaryScreen visibleFrame];

	// Get the total screen size of the primary monitor.
	GPrimaryMonitorWidth = ScreenFrame.size.width;
	GPrimaryMonitorHeight = ScreenFrame.size.height;

	GVirtualScreenRect.left = 0;
	GVirtualScreenRect.top = 0;
	GVirtualScreenRect.right = 0;
	GVirtualScreenRect.bottom = 0;

	for( int32 Index = 0; Index < [AllScreens count]; Index++ )
	{
		NSScreen* Screen = (NSScreen*)[AllScreens objectAtIndex: Index];
		NSRect DisplayFrame = [Screen frame];
		if(DisplayFrame.origin.x != ScreenFrame.origin.x)
		{
			GVirtualScreenRect.right += DisplayFrame.size.width;
		}
		else
		{
			GVirtualScreenRect.right = FMath::Max(GVirtualScreenRect.right, (int)DisplayFrame.size.width);
		}
		if(DisplayFrame.origin.y != ScreenFrame.origin.y)
		{
			GVirtualScreenRect.bottom += DisplayFrame.size.height;
		}
		else
		{
			GVirtualScreenRect.bottom = FMath::Max(GVirtualScreenRect.bottom, (int)DisplayFrame.size.height);
		}
	}

	// Get the screen rect of the primary monitor, exclusing taskbar etc.
	GPrimaryMonitorWorkRect.left = VisibleFrame.origin.x;
	GPrimaryMonitorWorkRect.right = VisibleFrame.origin.x + VisibleFrame.size.width;
	GPrimaryMonitorWorkRect.top = ScreenFrame.size.height - (VisibleFrame.origin.y + VisibleFrame.size.height);
	GPrimaryMonitorWorkRect.bottom = ScreenFrame.size.height - VisibleFrame.origin.y;
#endif		// WITH_ENGINE
}

void FMacPlatformMisc::PlatformInit()
{
	// Increase the maximum number of simultaneously open files
	struct rlimit Limit;
	Limit.rlim_cur = OPEN_MAX;
	Limit.rlim_max = RLIM_INFINITY;
	int32 Result = setrlimit(RLIMIT_NOFILE, &Limit);
	check(Result == 0);

	// Randomize.
    if( GIsBenchmarking )
	{
		srand( 0 );
	}
    else
	{
		srand( (unsigned)time( NULL ) );
	}

	// Identity.
	UE_LOG(LogInit, Log, TEXT("Computer: %s"), FPlatformProcess::ComputerName() );
	UE_LOG(LogInit, Log, TEXT("User: %s"), FPlatformProcess::UserName() );

	const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
	UE_LOG(LogInit, Log, TEXT("CPU Page size=%i, Cores=%i"), MemoryConstants.PageSize, FPlatformMisc::NumberOfCores() );

	// Timer resolution.
	UE_LOG(LogInit, Log, TEXT("High frequency timer resolution =%f MHz"), 0.000001 / FPlatformTime::GetSecondsPerCycle() );
	
	GMacAppInfo.Init();
}


#if WITH_ENGINE
void FMacPlatformMisc::PlatformPostInit()
{
	if ( FApp::IsGame() )
	{
		SCOPED_AUTORELEASE_POOL;

		// Obtain width and height of primary monitor.
		int32 ScreenWidth  = GPrimaryMonitorWidth;
		int32 ScreenHeight = GPrimaryMonitorHeight;
		int32 WindowWidth  = ScreenWidth / 2;
		int32 WindowHeight = ScreenHeight / 2;
		int32 WindowPosX = (ScreenWidth - WindowWidth ) / 2;
		int32 WindowPosY = (ScreenHeight - WindowHeight ) / 2;

		const FString Name = FApp::GetGameName();

		NSRect ViewRect = NSMakeRect(WindowPosX, WindowPosY, WindowWidth, WindowHeight);

		GGameWindow = [[NSWindow alloc] initWithContentRect: ViewRect
									styleMask: (NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask)
									  backing: NSBackingStoreBuffered
										defer: NO];

		if( GGameWindow )
		{
			[GGameWindow setAcceptsMouseMovedEvents: YES];
			[GGameWindow setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
			[GGameWindow setOpaque: YES];

			CFStringRef CFName = FPlatformString::TCHARToCFString( *Name );
			[GGameWindow setTitle: ( NSString *)CFName];
			CFRelease( CFName );
		}
		verify( GGameWindow );
	}
}

/**
 * Prevents screen-saver from kicking in by moving the mouse by 0 pixels. This works even on
 * Vista in the presence of a group policy for password protected screen saver.
 */
void FMacPlatformMisc::PreventScreenSaver()
{
	CGEventRef LocationEvent = CGEventCreate( NULL );
	CGPoint MouseLocation = CGEventGetLocation( LocationEvent );
	CFRelease( LocationEvent );
	CGEventRef MouseMoveCommand = CGEventCreateMouseEvent( NULL, kCGEventMouseMoved, MouseLocation, 0 );
	CGEventPost( kCGHIDEventTap, MouseMoveCommand );
	CFRelease( MouseMoveCommand );
}

#endif		// WITH_ENGINE

GenericApplication* FMacPlatformMisc::CreateApplication()
{
	return FMacApplication::CreateMacApplication();
}

void FMacPlatformMisc::GetEnvironmentVariable(const TCHAR* InVariableName, TCHAR* Result, int32 ResultLength)
{
	FString VariableName = InVariableName;
	VariableName.ReplaceInline(TEXT("-"), TEXT("_"));
	ANSICHAR *AnsiResult = getenv(TCHAR_TO_ANSI(*VariableName));
	if (AnsiResult)
	{
		wcsncpy(Result, ANSI_TO_TCHAR(AnsiResult), ResultLength);
	}
	else
	{
		*Result = 0;
	}
}

TArray<uint8> FMacPlatformMisc::GetMacAddress()
{
	TArray<uint8> Result;

	io_iterator_t InterfaceIterator;
	{
		CFMutableDictionaryRef MatchingDict = IOServiceMatching(kIOEthernetInterfaceClass);

		if (!MatchingDict)
		{
			UE_LOG(LogMac, Warning, TEXT("GetMacAddress failed - no Ethernet interfaces"));
			return Result;
		}

		CFMutableDictionaryRef PropertyMatchDict =
			CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
			&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

		if (!PropertyMatchDict)
		{
			UE_LOG(LogMac, Warning, TEXT("GetMacAddress failed - can't create CoreFoundation mutable dictionary!"));
			return Result;
		}

		CFDictionarySetValue(PropertyMatchDict, CFSTR(kIOPrimaryInterface), kCFBooleanTrue);
		CFDictionarySetValue(MatchingDict, CFSTR(kIOPropertyMatchKey), PropertyMatchDict);
		CFRelease(PropertyMatchDict);

		if (IOServiceGetMatchingServices(kIOMasterPortDefault, MatchingDict, &InterfaceIterator) != KERN_SUCCESS)
		{
			UE_LOG(LogMac, Warning, TEXT("GetMacAddress failed - error getting matching services"));
			return Result;
		}
	}

	io_object_t InterfaceService;
	while ( (InterfaceService = IOIteratorNext(InterfaceIterator)) != NULL)
	{
		io_object_t ControllerService;
		if (IORegistryEntryGetParentEntry(InterfaceService, kIOServicePlane, &ControllerService) == KERN_SUCCESS)
		{
			CFTypeRef MACAddressAsCFData = IORegistryEntryCreateCFProperty(
				ControllerService, CFSTR(kIOMACAddress), kCFAllocatorDefault, 0);
			if (MACAddressAsCFData)
			{
				Result.AddZeroed(kIOEthernetAddressSize);
				CFDataGetBytes((CFDataRef)MACAddressAsCFData, CFRangeMake(0, kIOEthernetAddressSize), Result.GetData());
				break;
				CFRelease(MACAddressAsCFData);
            }
			IOObjectRelease(ControllerService);
        }
		IOObjectRelease(InterfaceService);
	}
	IOObjectRelease(InterfaceIterator);

	return Result;
}


void FMacPlatformMisc::SubmitErrorReport( const TCHAR* InErrorHist, EErrorReportMode::Type InMode )
{
	if ( !FPlatformMisc::IsDebuggerPresent() || GAlwaysReportCrash )
	{
		if (GUseCrashReportClient)
		{
			int32 FromCommandLine = 0;
			FParse::Value( FCommandLine::Get(), TEXT("AutomatedPerfTesting="), FromCommandLine );
			if(( FApp::IsUnattended() == true ) && ( FromCommandLine != 0 ) && ( FParse::Param(FCommandLine::Get(), TEXT("KillAllPopUpBlockingWindows")) == true ))
			{
				abort();
			}
			return;
		}
		
		TCHAR ReportDumpVersion[] = TEXT("3");
        
		FString ReportDumpPath;
		{
			const TCHAR ReportDumpFilename[] = TEXT("UnrealAutoReportDump");
			ReportDumpPath = FPaths::CreateTempFilename( *FPaths::GameLogDir(), ReportDumpFilename, TEXT( ".txt" ) );
		}
        
		FString IniDumpPath;
		if (GGameName[0])
		{
			const TCHAR IniDumpFilename[] = TEXT("UnrealAutoReportIniDump");
			IniDumpPath = FPaths::CreateTempFilename( *FPaths::GameLogDir(), IniDumpFilename, TEXT( ".txt" ) );
			//build the ini dump
			FOutputDeviceFile AutoReportIniFile(*IniDumpPath);
			GConfig->Dump(AutoReportIniFile);
			AutoReportIniFile.Flush();
			AutoReportIniFile.TearDown();
		}
        
		TCHAR AutoReportExe[] = TEXT("../DotNET/AutoReporter.exe");
        
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
			FCString::Strcpy(PlatformName, TEXT("Mac 64-bit"));
#else	//PLATFORM_64BITS
			FCString::Strcpy(PlatformName, TEXT("Mac 32-bit"));
#endif	//PLATFORM_64BITS
			TCHAR CultureName[10];
			FCString::Strcpy(CultureName, *FInternationalization::GetCurrentCulture()->GetName());
			TCHAR SystemTime[256];
			FCString::Strcpy(SystemTime, *FDateTime::Now().ToString());
			TCHAR EngineVersionStr[32];
			FCString::Strcpy(EngineVersionStr, *FString::FromInt(GEngineVersion.GetChangelist()));
            
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
			else if( GIsServer && !GIsClient )
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
			
			FString CrashVideoPath = FPaths::GameLogDir() + TEXT("CrashVideo.avi");
            
			// Get the paths that the files will actually have been saved to
			FString UserIniDumpPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*IniDumpPath);
			FString LogDirectory = FPaths::GameLogDir();
			TCHAR CommandlineLogFile[MAX_SPRINTF]=TEXT("");
            
			// Use the log file specified on the commandline if there is one
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
            
			// Start up the auto reporting app, passing the report dump file path, the games' log file,
			// the ini dump path, the minidump path, and the crashvideo path
			// protect against spaces in paths breaking them up on the commandline
            // On Mac, AutoReporter.exe is a command line argument (for Mono)
			FString CallingCommandLine = FString::Printf(TEXT("\"%s\" %d \"%s\" \"%s\" \"%s\" \"%s\" \"%s\""),
														 AutoReportExe, (uint32)(getpid()), *UserReportDumpPath, *UserLogFile, *UserIniDumpPath,
														 MiniDumpFilenameW, *UserCrashVideoPath);
            
			// Only unattended mode is supported on Mac/Mono
			CallingCommandLine += TEXT( " -unattended" );

            // Start Mono
			if (!FPlatformProcess::CreateProc(TEXT("/usr/bin/mono"), *CallingCommandLine, true, false, false, NULL, 0, NULL, NULL).IsValid())
			{
				UE_LOG(LogWindows, Warning, TEXT("Couldn't start up the Auto Reporting process!"));
				FMessageDialog::Open( EAppMsgType::Ok, FText::FromString( InErrorHist ) );
			}
		}
	}
}

void FMacPlatformMisc::PumpMessages( bool bFromMainLoop )
{
	if( bFromMainLoop )
	{
		SCOPED_AUTORELEASE_POOL;

		while( NSEvent *Event = [NSApp nextEventMatchingMask: NSAnyEventMask untilDate: nil inMode: NSDefaultRunLoopMode dequeue: true] )
		{
			const bool bIsMouseClickEvent = [Event type] == NSLeftMouseDown || [Event type] == NSLeftMouseUp
										 || [Event type] == NSRightMouseDown || [Event type] == NSRightMouseUp
										 || [Event type] == NSOtherMouseDown || [Event type] == NSOtherMouseUp;

			if( MacApplication )
			{
				if( !bIsMouseClickEvent || [Event window] == NULL )
				{
					MacApplication->AddPendingEvent( Event );
				}

				if( [Event type] == NSLeftMouseUp )
				{
					MacApplication->OnWindowDraggingFinished();
				}
			}

			[NSApp sendEvent: Event];
		}
	}
}

uint32 FMacPlatformMisc::GetCharKeyMap(uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
	return FGenericPlatformMisc::GetStandardPrintableKeyMap(KeyCodes, KeyNames, MaxMappings, false, true);
}

uint32 FMacPlatformMisc::GetKeyMap( uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings )
{
#define ADDKEYMAP(KeyCode, KeyName)		if (NumMappings<MaxMappings) { KeyCodes[NumMappings]=KeyCode; KeyNames[NumMappings]=KeyName; ++NumMappings; };

	uint32 NumMappings = 0;

	if ( KeyCodes && KeyNames && (MaxMappings > 0) )
	{
		ADDKEYMAP( kVK_Delete, TEXT("BackSpace") );
		ADDKEYMAP( kVK_Tab, TEXT("Tab") );
		ADDKEYMAP( kVK_Return, TEXT("Enter") );
		ADDKEYMAP( kVK_ANSI_KeypadEnter, TEXT("Enter") );

		ADDKEYMAP( kVK_CapsLock, TEXT("CapsLock") );
		ADDKEYMAP( kVK_Escape, TEXT("Escape") );
		ADDKEYMAP( kVK_Space, TEXT("SpaceBar") );
		ADDKEYMAP( kVK_PageUp, TEXT("PageUp") );
		ADDKEYMAP( kVK_PageDown, TEXT("PageDown") );
		ADDKEYMAP( kVK_End, TEXT("End") );
		ADDKEYMAP( kVK_Home, TEXT("Home") );

		ADDKEYMAP( kVK_LeftArrow, TEXT("Left") );
		ADDKEYMAP( kVK_UpArrow, TEXT("Up") );
		ADDKEYMAP( kVK_RightArrow, TEXT("Right") );
		ADDKEYMAP( kVK_DownArrow, TEXT("Down") );

		ADDKEYMAP( kVK_ForwardDelete, TEXT("Delete") );

		ADDKEYMAP( kVK_ANSI_Keypad0, TEXT("NumPadZero") );
		ADDKEYMAP( kVK_ANSI_Keypad1, TEXT("NumPadOne") );
		ADDKEYMAP( kVK_ANSI_Keypad2, TEXT("NumPadTwo") );
		ADDKEYMAP( kVK_ANSI_Keypad3, TEXT("NumPadThree") );
		ADDKEYMAP( kVK_ANSI_Keypad4, TEXT("NumPadFour") );
		ADDKEYMAP( kVK_ANSI_Keypad5, TEXT("NumPadFive") );
		ADDKEYMAP( kVK_ANSI_Keypad6, TEXT("NumPadSix") );
		ADDKEYMAP( kVK_ANSI_Keypad7, TEXT("NumPadSeven") );
		ADDKEYMAP( kVK_ANSI_Keypad8, TEXT("NumPadEight") );
		ADDKEYMAP( kVK_ANSI_Keypad9, TEXT("NumPadNine") );

		ADDKEYMAP( kVK_ANSI_KeypadMultiply, TEXT("Multiply") );
		ADDKEYMAP( kVK_ANSI_KeypadPlus, TEXT("Add") );
		ADDKEYMAP( kVK_ANSI_KeypadMinus, TEXT("Subtract") );
		ADDKEYMAP( kVK_ANSI_KeypadDecimal, TEXT("Decimal") );
		ADDKEYMAP( kVK_ANSI_KeypadDivide, TEXT("Divide") );

		ADDKEYMAP( kVK_F1, TEXT("F1") );
		ADDKEYMAP( kVK_F2, TEXT("F2") );
		ADDKEYMAP( kVK_F3, TEXT("F3") );
		ADDKEYMAP( kVK_F4, TEXT("F4") );
		ADDKEYMAP( kVK_F5, TEXT("F5") );
		ADDKEYMAP( kVK_F6, TEXT("F6") );
		ADDKEYMAP( kVK_F7, TEXT("F7") );
		ADDKEYMAP( kVK_F8, TEXT("F8") );
		ADDKEYMAP( kVK_F9, TEXT("F9") );
		ADDKEYMAP( kVK_F10, TEXT("F10") );
		ADDKEYMAP( kVK_F11, TEXT("F11") );
		ADDKEYMAP( kVK_F12, TEXT("F12") );

        //Mac pretends the Command key is Ctrl
        ADDKEYMAP( MMK_RightCommand, TEXT("RightControl") );
        ADDKEYMAP( MMK_LeftCommand, TEXT("LeftControl") );
        ADDKEYMAP( MMK_LeftShift, TEXT("LeftShift") );
        ADDKEYMAP( MMK_CapsLock, TEXT("CapsLock") );
        ADDKEYMAP( MMK_LeftAlt, TEXT("LeftAlt") );
        ADDKEYMAP( MMK_LeftControl, TEXT("LeftControl") );
        ADDKEYMAP( MMK_RightShift, TEXT("RightShift") );
        ADDKEYMAP( MMK_RightAlt, TEXT("RightAlt") );
        ADDKEYMAP( MMK_RightControl, TEXT("RightControl") );
	}

	check(NumMappings < MaxMappings);
	return NumMappings;
}

void FMacPlatformMisc::RequestExit( bool Force )
{
	UE_LOG(LogMac, Log,  TEXT("FPlatformMisc::RequestExit(%i)"), Force );
	if( Force )
	{
		// Abort allows signal handler to know we aborted.
		abort();
	}
	else
	{
		// Tell the platform specific code we want to exit cleanly from the main loop.
		GIsRequestingExit = 1;
	}
}

const TCHAR* FMacPlatformMisc::GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error)
{
	// There's no Mac equivalent for GetLastError()
	check(OutBuffer && BufferCount);
	*OutBuffer = TEXT('\0');
	return OutBuffer;
}

void FMacPlatformMisc::ClipboardCopy(const TCHAR* Str)
{
	SCOPED_AUTORELEASE_POOL;

	CFStringRef CocoaString = FPlatformString::TCHARToCFString(Str);
	NSPasteboard *Pasteboard = [NSPasteboard generalPasteboard];
	[Pasteboard clearContents];
	NSPasteboardItem *Item = [[[NSPasteboardItem alloc] init] autorelease];
	[Item setString: (NSString *)CocoaString forType: NSPasteboardTypeString];
	[Pasteboard writeObjects:[NSArray arrayWithObject:Item]];
	CFRelease(CocoaString);
}

void FMacPlatformMisc::ClipboardPaste(class FString& Result)
{
	SCOPED_AUTORELEASE_POOL;

	NSPasteboard *Pasteboard = [NSPasteboard generalPasteboard];
	NSString *CocoaString = [Pasteboard stringForType: NSPasteboardTypeString];
	if (CocoaString)
	{
		TArray<TCHAR> Ch;
		Ch.AddUninitialized([CocoaString length] + 1);
		FPlatformString::CFStringToTCHAR((CFStringRef)CocoaString, Ch.GetTypedData());
		Result = Ch.GetTypedData();
	}
	else
	{
		Result = TEXT("");
	}
}

void FMacPlatformMisc::CreateGuid(FGuid& Result)
{
	int32 Year = 0, Month = 0, DayOfWeek = 0, Day = 0, Hour = 0, Min = 0, Sec = 0, MSec = 0;
	FPlatformTime::SystemTime(Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec);

	FGuid   GUID;
	GUID.A = Day | (Hour << 16);
	GUID.B = Month | (Sec << 16);
	GUID.C = MSec | (Min << 16);
	GUID.D = Year ^ FPlatformTime::Cycles();

	Result = GUID;
}

static int32 ShowExtendedAlertWindow( EAppMsgType::Type MsgType, NSString* Text, NSString* Caption )
{
	SCOPED_AUTORELEASE_POOL;

	checkf(GExtendedAlertWindow);

	// Update window contents
	NSArray* SubViews = [[GExtendedAlertWindow contentView] subviews];
	for (NSView* SubView in SubViews)
	{
		if ([SubView isMemberOfClass:[NSTextField class]])
		{
			// Set the caption and message text
			NSTextField* TextField = (NSTextField*)SubView;
			if ([[TextField stringValue] isEqualToString: @"Caption"])
			{
				[TextField setStringValue: Caption];
			}
			else if ([[TextField stringValue] isEqualToString: @"Message"])
			{
				[TextField setStringValue: Text];
			}
		}
		else if (MsgType == EAppMsgType::YesNoYesAllNoAll && [SubView isMemberOfClass:[NSButton class]])
		{
			// Remove Cancel button if it's not needed
			NSButton* Button = (NSButton*)SubView;
			if ([[Button title] isEqualToString: @"Cancel"])
			{
				[SubView setHidden: true];
			}
		}
	}

	// UE4AppDelegate's OnAlertWindowButtonPress will close the window and set the correct return code
	return [NSApp runModalForWindow: GExtendedAlertWindow];
}

EAppReturnType::Type FMacPlatformMisc::MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption )
{
	SCOPED_AUTORELEASE_POOL;

	NSString* CocoaText = (NSString*)FPlatformString::TCHARToCFString(Text);
	NSString* CocoaCaption = (NSString*)FPlatformString::TCHARToCFString(Caption);

	EAppReturnType::Type RetValue = EAppReturnType::Cancel;
	NSInteger Result;

	if(MacApplication)
	{
		MacApplication->UseMouseCaptureWindow(false);
	}
	
	switch (MsgType)
	{
	case EAppMsgType::YesNo:
		Result = NSRunAlertPanel(CocoaCaption, CocoaText, @"Yes", @"No", NULL);
		if (Result == NSAlertDefaultReturn)
		{
			RetValue = EAppReturnType::Yes;
		}
		else if (Result == NSAlertAlternateReturn)
		{
			RetValue = EAppReturnType::No;
		}
		break;
	case EAppMsgType::OkCancel:
		// return 1 for Ok, 0 for Cancel
		Result = NSRunAlertPanel(CocoaCaption, CocoaText, @"OK", @"Cancel", NULL);
		if (Result == NSAlertDefaultReturn)
		{
			RetValue = EAppReturnType::Ok;
		}
		else if (Result == NSAlertAlternateReturn)
		{
			RetValue = EAppReturnType::Cancel;
		}
		break;
	case EAppMsgType::YesNoCancel:
		// return 0 for Yes, 1 for No, 2 for Cancel
		Result = NSRunAlertPanel(CocoaCaption, CocoaText, @"Yes", @"No", @"Cancel");
		if (Result == NSAlertDefaultReturn)
		{
			RetValue = EAppReturnType::Yes;
		}
		else if (Result == NSAlertAlternateReturn)
		{
			RetValue = EAppReturnType::No;
		}
		else
		{
			RetValue = EAppReturnType::Cancel;
		}
		break;
	case EAppMsgType::CancelRetryContinue:
		// return 0 for Cancel, 1 for Retry, 2 for Continue
		Result = NSRunAlertPanel(CocoaCaption, CocoaText, @"Continue", @"Retry", @"Cancel");
		if (Result == NSAlertDefaultReturn)
		{
			RetValue = EAppReturnType::Continue;
		}
		else if (Result == NSAlertAlternateReturn)
		{
			RetValue = EAppReturnType::Retry;
		}
		else
		{
			RetValue = EAppReturnType::Cancel;
		}
		break;
	case EAppMsgType::YesNoYesAllNoAll:
		// return 0 for No, 1 for Yes, 2 for YesToAll, 3 for NoToAll
		Result = ShowExtendedAlertWindow(MsgType, CocoaText, CocoaCaption);
		if (Result == 0)
		{
			RetValue = EAppReturnType::No;
		}
		else if (Result == 1)
		{
			RetValue = EAppReturnType::Yes;
		}
		else if (Result == 2)
		{
			RetValue = EAppReturnType::YesAll;
		}
		else
		{
			RetValue = EAppReturnType::NoAll;
		}
		break;
	case EAppMsgType::YesNoYesAllNoAllCancel:
		// return 0 for No, 1 for Yes, 2 for YesToAll, 3 for NoToAll, 4 for Cancel
		Result = ShowExtendedAlertWindow(MsgType, CocoaText, CocoaCaption);
		if (Result == 0)
		{
			RetValue = EAppReturnType::No;
		}
		else if (Result == 1)
		{
			RetValue = EAppReturnType::Yes;
		}
		else if (Result == 2)
		{
			RetValue = EAppReturnType::YesAll;
		}
		else if (Result == 3)
		{
			RetValue = EAppReturnType::NoAll;
		}
		else
		{
			RetValue = EAppReturnType::Cancel;
		}
		break;
	default:
		NSRunAlertPanel(CocoaCaption, CocoaText, @"OK", NULL, NULL);
		break;
	}
	
	if(MacApplication)
	{
		MacApplication->UseMouseCaptureWindow(true);
	}

	CFRelease((CFStringRef)CocoaCaption);
	CFRelease((CFStringRef)CocoaText);

	return RetValue;
}

static bool HandleFirstInstall()
{
	if (FParse::Param( FCommandLine::Get(), TEXT("firstinstall")))
	{
		GLog->Flush();

		// Flush config to ensure language changes are written to disk.
		GConfig->Flush(false);

		return false; // terminate the game
	}
	return true; // allow the game to continue;
}

bool FMacPlatformMisc::CommandLineCommands()
{
	return HandleFirstInstall();
}

int32 FMacPlatformMisc::NumberOfCores()
{	
	static int32 NumberOfCores = -1;
	if (NumberOfCores == -1)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("usehyperthreading")))
		{
			NumberOfCores = NumberOfCoresIncludingHyperthreads();
		}
		else
		{
			SIZE_T Size = sizeof(int32);
		
			if (sysctlbyname("hw.physicalcpu", &NumberOfCores, &Size, NULL, 0) != 0)
			{
				NumberOfCores = 1;
			}
		}
	}
	return NumberOfCores;
}

int32 FMacPlatformMisc::NumberOfCoresIncludingHyperthreads()
{
	static int32 NumberOfCores = -1;
	if (NumberOfCores == -1)
	{
		SIZE_T Size = sizeof(int32);
		
		if (sysctlbyname("hw.ncpu", &NumberOfCores, &Size, NULL, 0) != 0)
		{
			NumberOfCores = 1;
		}
	}
	return NumberOfCores;
}

#include "ModuleManager.h"

void FMacPlatformMisc::LoadPreInitModules()
{
	FModuleManager::Get().LoadModule(TEXT("OpenGLDrv"));
	FModuleManager::Get().LoadModule(TEXT("CoreAudio"));
}

FLinearColor FMacPlatformMisc::GetScreenPixelColor(const FVector2D& InScreenPos, float InGamma)
{
	SCOPED_AUTORELEASE_POOL;

	CGImageRef ScreenImage = CGWindowListCreateImage(CGRectMake(InScreenPos.X, InScreenPos.Y, 1, 1), kCGWindowListOptionOnScreenBelowWindow, kCGNullWindowID, kCGWindowImageDefault);
    NSBitmapImageRep *BitmapRep = [[[NSBitmapImageRep alloc] initWithCGImage: ScreenImage] autorelease];
    NSImage *Image = [[[NSImage alloc] init] autorelease];
    [Image addRepresentation: BitmapRep];
	[Image lockFocus];
	NSColor* PixelColor = NSReadPixel(NSMakePoint(0.0f, 0.0f));
	[Image unlockFocus];
	CGImageRelease(ScreenImage);

	FLinearColor ScreenColor([PixelColor redComponent], [PixelColor greenComponent], [PixelColor blueComponent], 1.0f);

	if (InGamma > 1.0f)
	{
		// Correct for render gamma
		ScreenColor.R = FMath::Pow(ScreenColor.R, InGamma);
		ScreenColor.G = FMath::Pow(ScreenColor.G, InGamma);
		ScreenColor.B = FMath::Pow(ScreenColor.B, InGamma);
	}
	
	return ScreenColor;
}

FString FMacPlatformMisc::GetCPUVendor()
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


	int32 Args[4];
	asm( "cpuid" : "=a" (Args[0]), "=b" (Args[1]), "=c" (Args[2]), "=d" (Args[3]) : "a" (0));

	VendorResult.Dw.dw0 = Args[1];
	VendorResult.Dw.dw1 = Args[3];
	VendorResult.Dw.dw2 = Args[2];
	VendorResult.Buffer[12] = 0;

	return ANSI_TO_TCHAR(VendorResult.Buffer);
}

int32 FMacPlatformMisc::ConvertSlateYPositionToCocoa(int32 YPosition)
{
	NSScreen* PrimaryScreen = [[NSScreen screens] objectAtIndex: 0];
	const float PrimaryScreenHeight = [PrimaryScreen frame].size.height;
	return -(YPosition - PrimaryScreenHeight + 1);
}


FString FMacPlatformMisc::GetDefaultLocale()
{
	CFLocaleRef loc = CFLocaleCopyCurrent();

	TCHAR langCode[20];
	CFArrayRef langs = CFLocaleCopyPreferredLanguages();
	CFStringRef langCodeStr = (CFStringRef)CFArrayGetValueAtIndex(langs, 0);
	FPlatformString::CFStringToTCHAR(langCodeStr, langCode);

	TCHAR countryCode[20];
	CFStringRef countryCodeStr = (CFStringRef)CFLocaleGetValue(loc, kCFLocaleCountryCode);
	FPlatformString::CFStringToTCHAR(countryCodeStr, countryCode);

	return FString::Printf(TEXT("%s_%s"), langCode, countryCode);
}

/** Global pointer to crash handler */
void (* GCrashHandlerPointer)(const FGenericCrashContext & Context) = NULL;

/**
 * Good enough default crash reporter.
 */
static void DefaultCrashHandler(FMacCrashContext const& Context)
{
	Context.ReportCrash();
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
		GError->HandleError();
	}
	
	return Context.GenerateCrashInfoAndLaunchReporter();
}

/** True system-specific crash handler that gets called first */
static void PlatformCrashHandler(int32 Signal, siginfo_t* Info, void* Context)
{
	FMacCrashContext CrashContext;
	CrashContext.InitFromSignal(Signal, Info, Context);
	
	if (GCrashHandlerPointer)
	{
		GCrashHandlerPointer(CrashContext);
	}
	else
	{
		// call default one
		DefaultCrashHandler(CrashContext);
	}
}

/**
 * Handles graceful termination. Gives time to exit gracefully, but second signal will quit immediately.
 */
static void GracefulTerminationHandler(int32 Signal, siginfo_t* Info, void* Context)
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
		GIsRequestingExit = 1;
	}
	else
	{
		_Exit(0);
	}
}

void FMacPlatformMisc::SetGracefulTerminationHandler()
{
	struct sigaction Action;
	FMemory::Memzero(&Action, sizeof(struct sigaction));
	Action.sa_sigaction = GracefulTerminationHandler;
	sigemptyset(&Action.sa_mask);
	Action.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
	sigaction(SIGINT, &Action, NULL);
	sigaction(SIGTERM, &Action, NULL);
	sigaction(SIGHUP, &Action, NULL);	//  this should actually cause the server to just re-read configs (restart?)
}

void FMacPlatformMisc::SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext & Context))
{
	GCrashHandlerPointer = CrashHandler;
	
	struct sigaction Action;
	FMemory::Memzero(&Action, sizeof(struct sigaction));
	Action.sa_sigaction = PlatformCrashHandler;
	sigemptyset(&Action.sa_mask);
	Action.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
	sigaction(SIGQUIT, &Action, NULL);	// SIGQUIT is a user-initiated "crash".
	sigaction(SIGILL, &Action, NULL);
	sigaction(SIGEMT, &Action, NULL);
	sigaction(SIGFPE, &Action, NULL);
	sigaction(SIGBUS, &Action, NULL);
	sigaction(SIGSEGV, &Action, NULL);
	sigaction(SIGSYS, &Action, NULL);
	sigaction(SIGABRT, &Action, NULL);
}

void FMacCrashContext::GenerateReport(char const* DiagnosticsPath) const
{
	int ReportFile = open(DiagnosticsPath, O_CREAT|O_WRONLY, 0766);
	if (ReportFile != NULL)
	{
		char Line[PATH_MAX] = {};
		
		WriteLine(ReportFile, "Generating report for minidump");
		WriteLine(ReportFile);
		
		FCStringAnsi::Strncpy(Line, "Application version 4.0.", PATH_MAX);
		FCStringAnsi::Strcat(Line, PATH_MAX, ItoANSI(ENGINE_VERSION_HIWORD, 10));
		FCStringAnsi::Strcat(Line, PATH_MAX, ".");
		FCStringAnsi::Strcat(Line, PATH_MAX, ItoANSI(ENGINE_VERSION_LOWORD, 10));
		WriteLine(ReportFile, Line);
		
		FCStringAnsi::Strncpy(Line, " ... built from changelist ", PATH_MAX);
		FCStringAnsi::Strcat(Line, PATH_MAX, ItoANSI(ENGINE_VERSION, 10));
		WriteLine(ReportFile, Line);
		WriteLine(ReportFile);
		
		FCStringAnsi::Strncpy(Line, "OS version Mac OS X ", PATH_MAX);
		FCStringAnsi::Strcat(Line, PATH_MAX, GMacAppInfo.OSVersionUTF8);
		FCStringAnsi::Strcat(Line, PATH_MAX, " (network name: ");
		FCStringAnsi::Strcat(Line, PATH_MAX, GMacAppInfo.MachineName);
		FCStringAnsi::Strcat(Line, PATH_MAX, ")");
		WriteLine(ReportFile, Line);
		
		FCStringAnsi::Strncpy(Line, "Running ", PATH_MAX);
		FCStringAnsi::Strcat(Line, PATH_MAX, ItoANSI(FPlatformMisc::NumberOfCores(), 10));
		FCStringAnsi::Strcat(Line, PATH_MAX, " ");
		FCStringAnsi::Strcat(Line, PATH_MAX, GMacAppInfo.MachineCPUString);
		FCStringAnsi::Strcat(Line, PATH_MAX, "processors (");
		FCStringAnsi::Strcat(Line, PATH_MAX, ItoANSI(FPlatformMisc::NumberOfCoresIncludingHyperthreads(), 10));
		FCStringAnsi::Strcat(Line, PATH_MAX, " logical cores)");
		WriteLine(ReportFile, Line);

		FCStringAnsi::Strncpy(Line, "Exception was \"", PATH_MAX);
		FCStringAnsi::Strcat(Line, PATH_MAX, SignalDescription);
		FCStringAnsi::Strcat(Line, PATH_MAX, "\"");
		WriteLine(ReportFile, Line);
		WriteLine(ReportFile);
		
		WriteLine(ReportFile, "<SOURCE START>");
		WriteLine(ReportFile, "<SOURCE END>");
		WriteLine(ReportFile);
		
		WriteLine(ReportFile, "<CALLSTACK START>");
		WriteLine(ReportFile, MinidumpCallstackInfo);
		WriteLine(ReportFile, "<CALLSTACK END>");
		WriteLine(ReportFile);
		
		// Technically _dyld_image_count & _dyld_get_image_name aren't async handler safe
		// however, I imagine they actually are, since they merely access an internal list
		// which isn't even thread safe...
		uint32 ModuleCount = _dyld_image_count();
		FCStringAnsi::Strncpy(Line, ItoANSI(ModuleCount, 10), PATH_MAX);
		FCStringAnsi::Strcat(Line, PATH_MAX, " loaded modules");
		WriteLine(ReportFile, Line);
		WriteLine(ReportFile);
		
		WriteLine(ReportFile, "<MODULES START>");
		for(uint32 Index = 0; Index < ModuleCount; Index++)
		{
			char const* ModulePath = _dyld_get_image_name(Index);
			WriteLine(ReportFile, ModulePath);
		}
		WriteLine(ReportFile, "<MODULES END>");
		WriteLine(ReportFile);
		
		WriteLine(ReportFile, "Report end!");
		
		close(ReportFile);
	}
}

void FMacCrashContext::GenerateWindowsErrorReport(char const* WERPath) const
{
	int ReportFile = open(WERPath, O_CREAT|O_WRONLY, 0766);
	if (ReportFile != NULL)
	{
		// write BOM
		static uint16 ByteOrderMarker = 0xFEFF;
		write(ReportFile, &ByteOrderMarker, sizeof(ByteOrderMarker));
		
		WriteLine(ReportFile, TEXT("<?xml version=\"1.0\" encoding=\"UTF-16\"?>"));
		WriteLine(ReportFile, TEXT("<WERReportMetadata>"));
		
		WriteLine(ReportFile, TEXT("\t<OSVersionInformation>"));
		WriteUTF16String(ReportFile, TEXT("\t\t<WindowsNTVersion>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.OSVersion);
		WriteLine(ReportFile, TEXT("</WindowsNTVersion>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<Build>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.OSVersion);
		WriteUTF16String(ReportFile, TEXT(" ("));
		WriteUTF16String(ReportFile, *GMacAppInfo.OSBuild);
		WriteLine(ReportFile, TEXT(")</Build>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<Product>(0x30): Mac OS X "));
		WriteUTF16String(ReportFile, *GMacAppInfo.OSVersion);
		WriteLine(ReportFile, TEXT("</Product>"));
		
		WriteLine(ReportFile, TEXT("\t\t<Edition>Mac OS X</Edition>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<BuildString>Mac OS X "));
		WriteUTF16String(ReportFile, *GMacAppInfo.OSVersion);
		WriteUTF16String(ReportFile, TEXT(" ("));
		WriteUTF16String(ReportFile, *GMacAppInfo.OSBuild);
		WriteLine(ReportFile, TEXT(")</BuildString>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<Revision>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.OSBuild);
		WriteLine(ReportFile, TEXT("</Revision>"));
		
		WriteLine(ReportFile, TEXT("\t\t<Flavor>Multiprocessor Free</Flavor>"));
		WriteLine(ReportFile, TEXT("\t\t<Architecture>X64</Architecture>"));
		WriteUTF16String(ReportFile, TEXT("\t\t<LCID>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.LCID);
		WriteLine(ReportFile, TEXT("</LCID>"));
		WriteLine(ReportFile, TEXT("\t</OSVersionInformation>"));
		
		WriteLine(ReportFile, TEXT("\t<ParentProcessInformation>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<ParentProcessId>"));
		WriteUTF16String(ReportFile, ItoTCHAR(getppid(), 10));
		WriteLine(ReportFile, TEXT("</ParentProcessId>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<ParentProcessPath>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.ParentProcess);
		WriteLine(ReportFile, TEXT("</ParentProcessPath>"));
		
		WriteLine(ReportFile, TEXT("\t\t<ParentProcessCmdLine></ParentProcessCmdLine>"));	// FIXME: supply valid?
		WriteLine(ReportFile, TEXT("\t</ParentProcessInformation>"));
		
		WriteLine(ReportFile, TEXT("\t<ProblemSignatures>"));
		WriteLine(ReportFile, TEXT("\t\t<EventType>APPCRASH</EventType>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<Parameter0>UE4-"));
		WriteUTF16String(ReportFile, *GMacAppInfo.AppName);
		WriteLine(ReportFile, TEXT("</Parameter0>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<Parameter1>1.0."));
		WriteUTF16String(ReportFile, ItoTCHAR(ENGINE_VERSION_HIWORD, 10));
		WriteUTF16String(ReportFile, TEXT("."));
		WriteUTF16String(ReportFile, ItoTCHAR(ENGINE_VERSION_LOWORD, 10));
		WriteLine(ReportFile, TEXT("</Parameter1>"));

		WriteLine(ReportFile, TEXT("\t\t<Parameter2>528f2d37</Parameter2>"));													// FIXME: supply valid?
		WriteLine(ReportFile, TEXT("\t\t<Parameter3>KERNELBASE.dll</Parameter3>"));												// FIXME: supply valid?
		WriteLine(ReportFile, TEXT("\t\t<Parameter4>6.1.7601.18015</Parameter4>"));												// FIXME: supply valid?
		WriteLine(ReportFile, TEXT("\t\t<Parameter5>50b8479b</Parameter5>"));													// FIXME: supply valid?
		WriteLine(ReportFile, TEXT("\t\t<Parameter6>00000001</Parameter6>"));													// FIXME: supply valid?
		WriteLine(ReportFile, TEXT("\t\t<Parameter7>0000000000009E5D</Parameter7>"));											// FIXME: supply valid?
		WriteLine(ReportFile, TEXT("\t\t<Parameter8>!!</Parameter8>"));															// FIXME: supply valid?
		
		WriteUTF16String(ReportFile, TEXT("\t\t<Parameter9>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.BranchBaseDir);
		WriteLine(ReportFile, TEXT("</Parameter9>"));
		
		WriteLine(ReportFile, TEXT("\t</ProblemSignatures>"));
		
		WriteLine(ReportFile, TEXT("\t<DynamicSignatures>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<Parameter1>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.BiosUUID);
		WriteLine(ReportFile, TEXT("</Parameter1>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<Parameter2>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.LCID);
		WriteLine(ReportFile, TEXT("</Parameter2>"));
		WriteLine(ReportFile, TEXT("\t</DynamicSignatures>"));
		
		WriteLine(ReportFile, TEXT("\t<SystemInformation>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<MID>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.MachineUUID);
		WriteLine(ReportFile, TEXT("</MID>"));
		
		WriteLine(ReportFile, TEXT("\t\t<SystemManufacturer>Apple Inc.</SystemManufacturer>"));						// FIXME: supply valid?
		
		WriteUTF16String(ReportFile, TEXT("\t\t<SystemProductName>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.MachineModel);
		WriteLine(ReportFile, TEXT("</SystemProductName>"));
		
		WriteUTF16String(ReportFile, TEXT("\t\t<BIOSVersion>"));
		WriteUTF16String(ReportFile, *GMacAppInfo.BiosRelease);
		WriteUTF16String(ReportFile, TEXT("-"));
		WriteUTF16String(ReportFile, *GMacAppInfo.BiosRevision);
		WriteLine(ReportFile, TEXT("</BIOSVersion>"));											// FIXME: supply valid?
		WriteLine(ReportFile, TEXT("\t</SystemInformation>"));
		
		WriteLine(ReportFile, TEXT("</WERReportMetadata>"));
		
		close(ReportFile);
	}
}

void FMacCrashContext::GenerateMinidump(char const* MinidumpCallstackInfo, char const* Path) const
{
	int ReportFile = open(Path, O_CREAT|O_WRONLY, 0766);
	if (ReportFile != NULL)
	{
		// write BOM
		static uint16 ByteOrderMarker = 0xFEFF;
		write(ReportFile, &ByteOrderMarker, sizeof(ByteOrderMarker));
		
		WriteLine(ReportFile, ANSI_TO_TCHAR(MinidumpCallstackInfo));
		
		close(ReportFile);
	}
}

void FMacCrashContext::GenerateCrashInfoAndLaunchReporter() const
{
	// do not report crashes for tools (particularly for crash reporter itself)
#if !IS_PROGRAM
	
	// create a crash-specific directory
	char CrashInfoFolder[PATH_MAX] = {};
	FCStringAnsi::Strncpy(CrashInfoFolder, GMacAppInfo.CrashReportPath, PATH_MAX);
	FCStringAnsi::Strcat(CrashInfoFolder, PATH_MAX, "/CrashReport-");
	FCStringAnsi::Strcat(CrashInfoFolder, PATH_MAX, GMacAppInfo.AppNameUTF8);
	FCStringAnsi::Strcat(CrashInfoFolder, PATH_MAX, "-pid-");
	FCStringAnsi::Strcat(CrashInfoFolder, PATH_MAX, ItoANSI(getpid(), 10));
	if(!mkdir(CrashInfoFolder, 0766))
	{
		char FilePath[PATH_MAX] = {};
		FCStringAnsi::Strncpy(FilePath, CrashInfoFolder, PATH_MAX);
		FCStringAnsi::Strcat(FilePath, PATH_MAX, "/report.wer");
		int ReportFile = open(FilePath, O_CREAT|O_WRONLY, 0766);
		if (ReportFile != NULL)
		{
			// write BOM
			static uint16 ByteOrderMarker = 0xFEFF;
			write(ReportFile, &ByteOrderMarker, sizeof(ByteOrderMarker));
			
			WriteUTF16String(ReportFile, TEXT("\r\nAppPath="));
			WriteUTF16String(ReportFile, *GMacAppInfo.AppPath);
			WriteLine(ReportFile, TEXT("\r\n"));
			
			close(ReportFile);
		}
		
		// generate "minidump"
		FCStringAnsi::Strncpy(FilePath, CrashInfoFolder, PATH_MAX);
		FCStringAnsi::Strcat(FilePath, PATH_MAX, "/diagnostics.txt");
		GenerateReport(FilePath);
		
		// generate "WER"
		FCStringAnsi::Strncpy(FilePath, CrashInfoFolder, PATH_MAX);
		FCStringAnsi::Strcat(FilePath, PATH_MAX, "/wermeta.xml");
		GenerateWindowsErrorReport(FilePath);
		
		// generate "minidump" (just >1 byte)
		FCStringAnsi::Strncpy(FilePath, CrashInfoFolder, PATH_MAX);
		FCStringAnsi::Strcat(FilePath, PATH_MAX, "/minidump.dmp");
		GenerateMinidump(MinidumpCallstackInfo, FilePath);
		
		// generate "info.txt" custom data for our server
		FCStringAnsi::Strncpy(FilePath, CrashInfoFolder, PATH_MAX);
		FCStringAnsi::Strcat(FilePath, PATH_MAX, "/info.txt");
		ReportFile = open(FilePath, O_CREAT|O_WRONLY, 0766);
		if (ReportFile != NULL)
		{
			WriteUTF16String(ReportFile, TEXT("GameName UE4-"));
			WriteLine(ReportFile, *GMacAppInfo.AppName);
			
			WriteUTF16String(ReportFile, TEXT("BuildVersion 1.0."));
			WriteUTF16String(ReportFile, ItoTCHAR(ENGINE_VERSION_HIWORD, 10));
			WriteUTF16String(ReportFile, TEXT("."));
			WriteLine(ReportFile, ItoTCHAR(ENGINE_VERSION_LOWORD, 10));
			
			WriteUTF16String(ReportFile, TEXT("CommandLine "));
			WriteLine(ReportFile, *GMacAppInfo.CommandLine);
			
			WriteUTF16String(ReportFile, TEXT("BaseDir "));
			WriteLine(ReportFile, *GMacAppInfo.BranchBaseDir);
			
			WriteUTF16String(ReportFile, TEXT("MachineGuid "));
			WriteLine(ReportFile, *GMacAppInfo.MachineUUID);
			
			close(ReportFile);
		}
		
		// copy log
		FCStringAnsi::Strncpy(FilePath, CrashInfoFolder, PATH_MAX);
		FCStringAnsi::Strcat(FilePath, PATH_MAX, "/");
		FCStringAnsi::Strcat(FilePath, PATH_MAX, GMacAppInfo.AppNameUTF8);
		FCStringAnsi::Strcat(FilePath, PATH_MAX, ".log");
		int LogSrc = open(GMacAppInfo.AppLogPath, O_RDONLY);
		int LogDst = open(FilePath, O_CREAT|O_WRONLY, 0766);
		
		char Data[PATH_MAX] = {};
		int Bytes = 0;
		while((Bytes = read(LogSrc, Data, PATH_MAX)) > 0)
		{
			write(LogDst, Data, Bytes);
		}
		close(LogDst);
		close(LogSrc);
		// best effort, so don't care about result: couldn't copy -> tough, no log
		
		// try launching the tool and wait for its exit, if at all
		// Use fork() & execl() as they are async-signal safe, CreateProc can fail in Cocoa
		int32 ReturnCode = 0;
		pid_t ForkPID = fork();
		if(ForkPID == 0)
		{
			// Child
			FCStringAnsi::Strncpy(FilePath, CrashInfoFolder, PATH_MAX);
			FCStringAnsi::Strcat(FilePath, PATH_MAX, "/");
			execl(GMacAppInfo.CrashReportClient, "CrashReportClient", FilePath, NULL);
		}
		else
		{
			// Parent
			int StatLoc = 0;
			waitpid(ForkPID, &StatLoc, 0);
		}
	}
	
#endif
	
	_Exit(0);
}
