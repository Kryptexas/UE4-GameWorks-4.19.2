// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"
#include "ExceptionHandling.h"
#include "CocoaThread.h"

/**
 * Because crash reporters can crash, too
 */
void CrashReporterCrashHandler(const FGenericCrashContext & GenericContext)
{
	const FMacCrashContext& Context = static_cast< const FMacCrashContext& >( GenericContext );

	Context.ReportCrash();
	if (GLog)
	{
		GLog->SetCurrentThreadAsMasterThread();
		GLog->Flush();
	}
	if (GWarn)
	{
		GWarn->Flush();
	}
	// @todo Disable calls to functions which eventually try and allocated memory, resulting in the application hanging before the crash reporter can launch
	// We need to setup the memory allocator system to handle the case where we can't call the system malloc, free etc. to re-enable this.
	if (GError)
	{
		GError->Flush();
		//GError->HandleError();
	}
	FPlatformMisc::RequestExit(true);
}

static FString GSavedCommandLine;

@interface UE4AppDelegate : NSObject <NSApplicationDelegate>
{
}

@end

@implementation UE4AppDelegate

//handler for the quit apple event used by the Dock menu
- (void)handleQuitEvent:(NSAppleEventDescriptor*)Event withReplyEvent:(NSAppleEventDescriptor*)ReplyEvent
{
    [self requestQuit:self];
}

- (IBAction)requestQuit:(id)Sender
{
	GIsRequestingExit = true;
}

- (void) runGameThread:(id)Arg
{
	FPlatformMisc::SetGracefulTerminationHandler();
	FPlatformMisc::SetCrashHandler(&CrashReporterCrashHandler);
	
	RunCrashReportClient(*GSavedCommandLine);
	
	[NSApp terminate: self];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)Sender;
{
	if(!GIsRequestingExit)
	{
		[self requestQuit:self];
		return NSTerminateCancel;
	}
	return NSTerminateNow;
}

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
	//install the custom quit event handler
    NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
    [appleEventManager setEventHandler:self andSelector:@selector(handleQuitEvent:withReplyEvent:) forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
	
	RunGameThread(self, @selector(runGameThread:));
}

@end

int main(int argc, char *argv[])
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
