// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SymbolDebuggerApp.h"

static FString GSavedCommandLine;

@interface UE4AppDelegate : NSObject <NSApplicationDelegate>
{
}

@end

@implementation UE4AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)Notification
{
	RunSymbolDebugger(*GSavedCommandLine);

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
			Argument = FString::Printf(TEXT("\"%s\""), *Argument);
		}
		GSavedCommandLine += Argument;
	}

	return NSApplicationMain(argc, (const char **)argv);
}
