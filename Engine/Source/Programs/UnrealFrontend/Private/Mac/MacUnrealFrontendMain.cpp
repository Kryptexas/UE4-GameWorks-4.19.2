// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealFrontendMain.h"
#include "ExceptionHandling.h"
#include "RequiredProgramMainCPPInclude.h"


static FString GSavedCommandLine;


@interface UE4AppDelegate : NSObject<NSApplicationDelegate>
{
}

@end


@implementation UE4AppDelegate

//handler for the quit apple event used by the Dock menu
- (void)handleQuitEvent:(NSAppleEventDescriptor*)Event withReplyEvent:(NSAppleEventDescriptor*)ReplyEvent
{
    [self requestQuit:self];
}

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
	//install the custom quit event handler
    NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
    [appleEventManager setEventHandler:self andSelector:@selector(handleQuitEvent:withReplyEvent:) forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
	
	// OS X always uses the CrashReportClient, since the other paths aren't reliable
	GUseCrashReportClient = true;
	FPlatformMisc::SetGracefulTerminationHandler();
	FPlatformMisc::SetCrashHandler(NULL);
	
#if !UE_BUILD_SHIPPING
	if (FParse::Param(*GSavedCommandLine,TEXT("crashreports")))
	{
		GAlwaysReportCrash = true;
	}
#endif
	
#if UE_BUILD_DEBUG
	if (!GAlwaysReportCrash)
#else
	if (FPlatformMisc::IsDebuggerPresent() && !GAlwaysReportCrash)
#endif
	{
		UnrealFrontendMain(*GSavedCommandLine);
	}
	else
	{
		GIsGuarded = 1;
		UnrealFrontendMain(*GSavedCommandLine);
		GIsGuarded = 0;
	}

	FEngineLoop::AppExit();

	[NSApp terminate: self];
}

- (IBAction)requestQuit:(id)Sender
{
	GIsRequestingExit = true;
}

@end


int main( int argc, char *argv[] )
{
	for (int32 Option = 1; Option < argc; Option++)
	{
		GSavedCommandLine += TEXT(" ");
		FString Argument(ANSI_TO_TCHAR(argv[Option]));
		if (Argument.Contains(TEXT(" ")))
		{
			if (Argument.Contains(TEXT("=")))
			{
				FString ArgName;
				FString ArgValue;
				Argument.Split( TEXT("="), &ArgName, &ArgValue );
				Argument = FString::Printf( TEXT("%s=\"%s\""), *ArgName, *ArgValue );
			}
			else
			{
				Argument = FString::Printf(TEXT("\"%s\""), *Argument);
			}
		}
		GSavedCommandLine += Argument;
	}

	SCOPED_AUTORELEASE_POOL;
	[NSApplication sharedApplication];
	[NSApp setDelegate:[UE4AppDelegate new]];
	[NSApp run];
	return 0;
}
