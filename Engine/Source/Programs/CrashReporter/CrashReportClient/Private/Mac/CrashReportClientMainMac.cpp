// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"

/**
 * Because crash reporters can crash, too
 */
void CrashReporterCrashHandler(const FGenericCrashContext & GenericContext)
{
	const FMacCrashContext& Context = static_cast< const FMacCrashContext& >( GenericContext );

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
    [self OnQuitRequest:self];
}

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
	//install the custom quit event handler
    NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
    [appleEventManager setEventHandler:self andSelector:@selector(handleQuitEvent:withReplyEvent:) forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
	
	// OS X always uses the CrashReportClient, since the other paths aren't reliable
	GUseCrashReportClient = true;
	
	FPlatformMisc::SetGracefulTerminationHandler();
	FPlatformMisc::SetCrashHandler(CrashReporterCrashHandler);
	
	RunCrashReportClient(*GSavedCommandLine);
	
	[NSApp terminate: self];
}

- (IBAction)OnQuitRequest:(id)Sender
{
	[NSApp terminate: self];
}

- (IBAction)OnShowAboutWindow:(id)Sender
{
	[NSApp orderFrontStandardAboutPanel: Sender];
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
	
	return NSApplicationMain(argc, (const char **)argv);
}
