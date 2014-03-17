// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacUnrealFrontendMain.mm: Implements the main entry point for MacOS.
=============================================================================*/

#include "UnrealFrontendMain.h"


static FString GSavedCommandLine;


@interface UE4AppDelegate : NSObject<NSApplicationDelegate>
{
}

@end


@implementation UE4AppDelegate

//handler for the quit apple event used by the Dock menu
- (void)handleQuitEvent:(NSAppleEventDescriptor*)Event withReplyEvent:(NSAppleEventDescriptor*)ReplyEvent
{
    [self OnQuitRequest:self];
}

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
	//install the custom quit event handler
    NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
    [appleEventManager setEventHandler:self andSelector:@selector(handleQuitEvent:withReplyEvent:) forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
	
	FPlatformMisc::SetGracefulTerminationHandler();

#if !UE_BUILD_SHIPPING
	if (FParse::Param(*GSavedCommandLine,TEXT("crashreports")))
	{
		GAlwaysReportCrash = true;
	}
#endif

	GUseCrashReportClient = true;
	
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

- (IBAction)OnQuitRequest:(id)Sender
{
	GIsRequestingExit = true;
}

- (IBAction)OnShowAboutWindow:(id)Sender
{
	[NSApp orderFrontStandardAboutPanel: Sender];
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

	return NSApplicationMain(argc, (const char**)argv);
}
