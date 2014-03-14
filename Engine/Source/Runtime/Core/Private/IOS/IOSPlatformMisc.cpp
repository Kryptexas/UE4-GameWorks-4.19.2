// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IOSPlatformMisc.mm: iOS implementations of misc functions
=============================================================================*/

#include "CorePrivate.h"
#include "ExceptionHandling.h"
#include "SecureHash.h"
#include "IOSApplication.h"
#include "IOSAppDelegate.h"

/** Width of the primary monitor, in pixels. */
CORE_API int32 GPrimaryMonitorWidth = 0;
/** Height of the primary monitor, in pixels. */
CORE_API int32 GPrimaryMonitorHeight = 0;
/** Rectangle of the work area on the primary monitor (excluding taskbar, etc) in "virtual screen coordinates" (pixels). */
CORE_API RECT GPrimaryMonitorWorkRect;
/** Virtual screen rectangle including all monitors. */
CORE_API RECT GVirtualScreenRect;

/** Amount of free memory in MB reported by the system at starup */
CORE_API int32 GStartupFreeMemoryMB;

/** Settings for the game Window */
CORE_API UIWindow *GGameWindow = NULL;

/** Global pointer to memory warning handler */
void (* GMemoryWarningHandler)(const FGenericMemoryWarningContext& Context) = NULL;

void FIOSPlatformMisc::PlatformPreInit()
{
#if WITH_ENGINE
	CGRect ScreenFrame = [[UIScreen mainScreen] bounds];
	CGRect VisibleFrame = [[UIScreen mainScreen] applicationFrame];

	// Get the total screen size of the primary monitor.
	GPrimaryMonitorWidth = ScreenFrame.size.width;
	GPrimaryMonitorHeight = ScreenFrame.size.height;
	// @todo IOS: virtual screens
	GVirtualScreenRect.left = 0;
	GVirtualScreenRect.top = 0;
	GVirtualScreenRect.right = ScreenFrame.size.width;
	GVirtualScreenRect.bottom = ScreenFrame.size.height;

	// Get the screen rect of the primary monitor, excluding taskbar etc.
	//@todo - zombie - The y might be inverted on iOS. Will need to do verification on it. -astarkey
	GPrimaryMonitorWorkRect.left = VisibleFrame.origin.x;
	GPrimaryMonitorWorkRect.right = VisibleFrame.origin.x + VisibleFrame.size.width;
	GPrimaryMonitorWorkRect.top = ScreenFrame.size.height - (VisibleFrame.origin.y + VisibleFrame.size.height);
	GPrimaryMonitorWorkRect.bottom = ScreenFrame.size.height - VisibleFrame.origin.y;

#endif		// WITH_ENGINE
}

static int32 GetFreeMemoryMB()
{
	// get free memory
	vm_size_t PageSize;
	host_page_size(mach_host_self(), &PageSize);

	// get memory stats
	vm_statistics Stats;
	mach_msg_type_number_t StatsSize = sizeof(Stats);
	host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&Stats, &StatsSize);
	return (Stats.free_count * PageSize) / 1024 / 1024;
}

void FIOSPlatformMisc::PlatformInit()
{
	FAppEntry::PlatformInit();

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
	GStartupFreeMemoryMB = GetFreeMemoryMB();
	UE_LOG(LogInit, Log, TEXT("Free Memory at startup: %d MB"), GStartupFreeMemoryMB);
}


#if WITH_ENGINE
void FIOSPlatformMisc::PlatformPostInit()
{
	if ( FApp::IsGame() )
	{
		//Initialize the game window to the main screen.
		//Should not need to set opaque, since that should be part of the default initialization.
		// create the main landscape window object
		CGRect ApplicationRect = [[UIScreen mainScreen] bounds];
		GGameWindow = [[UIWindow alloc] initWithFrame: ApplicationRect];

		verify( GGameWindow );
	}
}
#endif		// WITH_ENGINE

GenericApplication* FIOSPlatformMisc::CreateApplication()
{
	return FIOSApplication::CreateIOSApplication();
}

void FIOSPlatformMisc::GetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, int32 ResultLength)
{
	ANSICHAR *AnsiResult = getenv(TCHAR_TO_ANSI(VariableName));
	if (AnsiResult)
	{
		wcsncpy(Result, ANSI_TO_TCHAR(AnsiResult), ResultLength);
	}
	else
	{
		*Result = 0;
	}
}

//void FIOSPlatformMisc::LowLevelOutputDebugStringf(const TCHAR *Fmt, ... )
//{
//
//}

void FIOSPlatformMisc::LowLevelOutputDebugString( const TCHAR *Message )
{
	//NsLog will out to all iOS output consoles, instead of just the Xcode console.
	NSLog(@"%s", TCHAR_TO_ANSI(Message));
}

const TCHAR* FIOSPlatformMisc::GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error)
{
	// There's no iOS equivalent for GetLastError()
	check(OutBuffer && BufferCount);
	*OutBuffer = TEXT('\0');
	return OutBuffer;
}

void FIOSPlatformMisc::ClipboardCopy(const TCHAR* Str)
{
	CFStringRef CocoaString = FPlatformString::TCHARToCFString(Str);
	UIPasteboard* Pasteboard = [UIPasteboard generalPasteboard];
	[Pasteboard setString:(NSString*)CocoaString];
}

void FIOSPlatformMisc::ClipboardPaste(class FString& Result)
{
	UIPasteboard* Pasteboard = [UIPasteboard generalPasteboard];
	NSString* CocoaString = [Pasteboard string];
	if(CocoaString)
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

EAppReturnType::Type FIOSPlatformMisc::MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption )
{
	NSString* CocoaText = (NSString*)FPlatformString::TCHARToCFString(Text);
	NSString* CocoaCaption = (NSString*)FPlatformString::TCHARToCFString(Caption);

	NSMutableArray* StringArray = [NSMutableArray arrayWithCapacity:7];

	[StringArray addObject:CocoaCaption];
	[StringArray addObject:CocoaText];

	// Figured that the order of all of these should be the same as their enum name.
	switch (MsgType)
	{
		case EAppMsgType::YesNo:
			[StringArray addObject:@"Yes"];
			[StringArray addObject:@"No"];
			break;
		case EAppMsgType::OkCancel:
			[StringArray addObject:@"Ok"];
			[StringArray addObject:@"Cancel"];
			break;
		case EAppMsgType::YesNoCancel:
			[StringArray addObject:@"Yes"];
			[StringArray addObject:@"No"];
			[StringArray addObject:@"Cancel"];
			break;
		case EAppMsgType::CancelRetryContinue:
			[StringArray addObject:@"Cancel"];
			[StringArray addObject:@"Retry"];
			[StringArray addObject:@"Continue"];
			break;
		case EAppMsgType::YesNoYesAllNoAll:
			[StringArray addObject:@"Yes"];
			[StringArray addObject:@"No"];
			[StringArray addObject:@"Yes To All"];
			[StringArray addObject:@"No To All"];
			break;
		case EAppMsgType::YesNoYesAllNoAllCancel:
			[StringArray addObject:@"Yes"];
			[StringArray addObject:@"No"];
			[StringArray addObject:@"Yes To All"];
			[StringArray addObject:@"No To All"];
			[StringArray addObject:@"Cancel"];
			break;
		default:
			[StringArray addObject:@"Ok"];
			break;
	}

	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	
	// reset our response to unset
	AppDelegate.AlertResponse = -1;

	[AppDelegate performSelectorOnMainThread:@selector(ShowAlert:) withObject:StringArray waitUntilDone:NO];

	while (AppDelegate.AlertResponse == -1)
	{
		FPlatformProcess::Sleep(.1);
	}

	EAppReturnType::Type Result = (EAppReturnType::Type)AppDelegate.AlertResponse;

	// Need to remap the return type to the correct one, since AlertResponse actually returns a button index.
	switch (MsgType)
	{
	case EAppMsgType::YesNo:
		Result = (Result == EAppReturnType::No ? EAppReturnType::Yes : EAppReturnType::No);
		break;
	case EAppMsgType::OkCancel:
		// return 1 for Ok, 0 for Cancel
		Result = (Result == EAppReturnType::No ? EAppReturnType::Ok : EAppReturnType::Cancel);
		break;
	case EAppMsgType::YesNoCancel:
		// return 0 for Yes, 1 for No, 2 for Cancel
		if(Result == EAppReturnType::No)
		{
			Result = EAppReturnType::Yes;
		}
		else if(Result == EAppReturnType::Yes)
		{
			Result = EAppReturnType::No;
		}
		else
		{
			Result = EAppReturnType::Cancel;
		}
		break;
	case EAppMsgType::CancelRetryContinue:
		// return 0 for Cancel, 1 for Retry, 2 for Continue
		if(Result == EAppReturnType::No)
		{
			Result = EAppReturnType::Cancel;
		}
		else if(Result == EAppReturnType::Yes)
		{
			Result = EAppReturnType::Retry;
		}
		else
		{
			Result = EAppReturnType::Continue;
		}
		break;
	case EAppMsgType::YesNoYesAllNoAll:
		// return 0 for No, 1 for Yes, 2 for YesToAll, 3 for NoToAll
		break;
	case EAppMsgType::YesNoYesAllNoAllCancel:
		// return 0 for No, 1 for Yes, 2 for YesToAll, 3 for NoToAll, 4 for Cancel
		break;
	default:
		Result = EAppReturnType::Ok;
		break;
	}

	CFRelease((CFStringRef)CocoaCaption);
	CFRelease((CFStringRef)CocoaText);

	return Result;
}

int32 FIOSPlatformMisc::NumberOfCores()
{
	// cache the number of cores
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

void FIOSPlatformMisc::LoadPreInitModules()
{
	FModuleManager::Get().LoadModule(TEXT("OpenGLDrv"));
	FModuleManager::Get().LoadModule(TEXT("IOSAudio"));
}

void* FIOSPlatformMisc::CreateAutoreleasePool()
{
	return [[NSAutoreleasePool alloc] init];
}

void FIOSPlatformMisc::ReleaseAutoreleasePool(void *Pool)
{
	[(NSAutoreleasePool*)Pool release];
}

void* FIOSPlatformMisc::GetHardwareWindow()
{
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	return AppDelegate.GLView;
}

FIOSPlatformMisc::EIOSDevice FIOSPlatformMisc::GetIOSDeviceType()
{
	// default to unknown
	static EIOSDevice DeviceType = IOS_Unknown;

	// if we've already figured it out, return it
	if (DeviceType != IOS_Unknown)
	{
		return DeviceType;
	}

	// get the device hardware type string length
	size_t DeviceIDLen;
	sysctlbyname("hw.machine", NULL, &DeviceIDLen, NULL, 0);

	// get the device hardware type
	char* DeviceID = (char*)malloc(DeviceIDLen);
	sysctlbyname("hw.machine", DeviceID, &DeviceIDLen, NULL, 0);

	// convert to NSStringt
	NSString *DeviceIDString= [NSString stringWithCString:DeviceID encoding:NSUTF8StringEncoding];
	free(DeviceID);

	// iPods
	if ([DeviceIDString hasPrefix:@"iPod"])
	{
		// get major revision number
		int Major = [DeviceIDString characterAtIndex:4] - '0';

		// iPod4,1 is iPod Touch 4th gen
		if (Major == 4)
		{
			DeviceType = IOS_IPodTouch4;
		}
		// iPod5 is iPod Touch 5th gen, anything higher will use 5th gen settings until released
		else if (Major >= 5)
		{
			DeviceType = IOS_IPodTouch5;
		}
	}
	// iPads
	else if ([DeviceIDString hasPrefix:@"iPad"])
	{
		// get major revision number
		int Major = [DeviceIDString characterAtIndex:4] - '0';
		int Minor = [DeviceIDString characterAtIndex:6] - '0';

		// iPad2,[1|2|3] is iPad 2 (1 - wifi, 2 - gsm, 3 - cdma)
		if (Major == 2)
		{
			// iPad2,5+ is the new iPadMini, anything higher will use these settings until released
			if (Minor >= 5)
			{
				DeviceType = IOS_IPadMini;
			}
			else
			{
				DeviceType = IOS_IPad2;
			}
		}
		// iPad3,[1|2|3] is iPad 3 and iPad3,4+ is iPad (4th generation)
		else if (Major == 3)
		{
			if (Minor <= 3)
			{
				DeviceType = IOS_IPad3;
			}
			// iPad3,4+ is the new iPad, anything higher will use these settings until released
			else if (Minor >= 4)
			{
				DeviceType = IOS_IPad4;
			}
		}
		// iPadAir
		else if (Major == 4)
		{
			DeviceType = IOS_IPadAir;
		}
		// Default to highest settings currently available for any future device
		else if (Major > 4)
		{
			DeviceType = IOS_IPadAir;
		}
	}
	// iPhones
	else if ([DeviceIDString hasPrefix:@"iPhone"])
	{
		int Major = [DeviceIDString characterAtIndex:6] - '0';

		if (Major == 3)
		{
			DeviceType = IOS_IPhone4;
		}
		else if (Major == 4)
		{
			DeviceType = IOS_IPhone4S;
		}
		else if (Major == 5)
		{
			DeviceType = IOS_IPhone5;
		}
		else if (Major >= 6)
		{
			DeviceType = IOS_IPhone5S;
		}
	}
	// simulator
	else if ([DeviceIDString hasPrefix:@"x86"])
	{
		// iphone
		if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
		{
			CGSize result = [[UIScreen mainScreen] bounds].size;
			if(result.height >= 586)
			{
				DeviceType = IOS_IPhone5;
			}
			else
			{
				DeviceType = IOS_IPhone4S;
			}
		}
		else
		{
			if ([[UIScreen mainScreen] scale] > 1.0f)
			{
				DeviceType = IOS_IPad4;
			}
			else
			{
				DeviceType = IOS_IPad2;
			}
		}
	}


	// if this is unknown at this point, we have a problem
	if (DeviceType == IOS_Unknown)
	{
		UE_LOG(LogInit, Fatal, TEXT("This IOS device type is not supported by UE4 [%s]\n"), *FString(DeviceIDString));
	}

	return DeviceType;
}

void FIOSPlatformMisc::SetMemoryWarningHandler(void (* InHandler)(const FGenericMemoryWarningContext & Context))
{
	GMemoryWarningHandler = InHandler;
}

void FIOSPlatformMisc::HandleLowMemoryWarning()
{
	UE_LOG(LogInit, Log, TEXT("Free Memory at Startup: %d MB"), GStartupFreeMemoryMB);
	UE_LOG(LogInit, Log, TEXT("Free Memory Now       : %d MB"), GetFreeMemoryMB());

	if(GMemoryWarningHandler != NULL)
	{
		FGenericMemoryWarningContext Context;
		GMemoryWarningHandler(Context);
	}
}

bool FIOSPlatformMisc::IsPackagedForDistribution()
{
#if !UE_BUILD_SHIPPING
	static bool PackagingModeCmdLine = FParse::Param(FCommandLine::Get(), TEXT("PACKAGED_FOR_DISTRIBUTION"));
	if (PackagingModeCmdLine)
	{
		return true;
	}
#endif
	NSString* PackagingMode = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"EpicPackagingMode"];
	return PackagingMode != nil && [PackagingMode isEqualToString : @"Distribution"];
}

/**
 * Returns a unique string for device identification
 * 
 * @return the unique string generated by this platform for this device
 */
FString FIOSPlatformMisc::GetUniqueDeviceId()
{
	// Check to see if this OS has this function
	if ([[UIDevice currentDevice] respondsToSelector:@selector(identifierForVendor)])
	{
		NSUUID* Id = [[UIDevice currentDevice] identifierForVendor];
		if (Id != nil)
		{
			NSString* IdfvString = [Id UUIDString];
			FString IDFV(IdfvString);
			return IDFV;
		}
	}
	return FPlatformMisc::GetHashedMacAddressString();
}
