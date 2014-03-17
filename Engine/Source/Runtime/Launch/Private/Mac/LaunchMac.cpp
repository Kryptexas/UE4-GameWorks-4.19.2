// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LaunchPrivatePCH.h"
#include "ExceptionHandling.h"

#if WITH_EDITOR
	#include "MainFrame.h"
#endif

#include <signal.h>


static FString GSavedCommandLine;
extern CORE_API NSWindow* GExtendedAlertWindow;
extern CORE_API NSWindow* GEULAWindow;
extern int32 GuardedMain( const TCHAR* CmdLine );
extern void LaunchStaticShutdownAfterError();

/**
 * Game-specific crash reporter
 */
void EngineCrashHandler(const FGenericCrashContext & GenericContext)
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
	LaunchStaticShutdownAfterError();
	return Context.GenerateCrashInfoAndLaunchReporter();
}

@interface UE4AppDelegate : NSObject <NSApplicationDelegate>
{
	IBOutlet NSWindow *AlertWindow;
	IBOutlet NSWindow *EULAWindow;
#if WITH_EDITOR
	NSString* Filename;
	bool bHasFinishedLaunching;
#endif
}

#if WITH_EDITOR
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
#endif
- (IBAction)OnAlertWindowButtonPress:(id)Sender;
- (IBAction)OnEULAAccepted:(id)Sender;
- (IBAction)OnEULADeclined:(id)Sender;

@end

@implementation UE4AppDelegate

- (void)awakeFromNib
{
#if WITH_EDITOR
	Filename = nil;
	bHasFinishedLaunching = false;
#endif
	GExtendedAlertWindow = AlertWindow;
	GEULAWindow = EULAWindow;
}

#if WITH_EDITOR
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	if(!bHasFinishedLaunching && (GSavedCommandLine.IsEmpty() || GSavedCommandLine.Contains(FString(filename))))
	{
		Filename = filename;
		return YES;
	}
	else
	{
		NSString* ProjectName = [[filename stringByDeletingPathExtension] lastPathComponent];
		
		NSURL* BundleURL = [[NSRunningApplication currentApplication] bundleURL];
		
		NSDictionary* Configuration = [NSDictionary dictionaryWithObject: [NSArray arrayWithObject: ProjectName] forKey: NSWorkspaceLaunchConfigurationArguments];
		
		NSError* Error = nil;
		
		NSRunningApplication* NewInstance = [[NSWorkspace sharedWorkspace] launchApplicationAtURL:BundleURL options:(NSWorkspaceLaunchOptions)(NSWorkspaceLaunchAsync|NSWorkspaceLaunchNewInstance) configuration:Configuration error:&Error];
		
		return (NewInstance != nil);
	}
}
#endif

- (IBAction)OnAlertWindowButtonPress:(id)Sender
{
	static NSString* AllNames[] = { @"No", @"Yes", @"Yes to all", @"No to all", @"Cancel" };

	NSString* ButtonName = [(NSButton*)Sender title];

	for (int32 Index = 0; Index < sizeof(AllNames) / sizeof(AllNames[0]); Index++)
	{
		if ([ButtonName isEqualToString: AllNames[Index]])
		{
			[NSApp stopModalWithCode: Index];
		}
	}
}

- (IBAction)OnEULAAccepted:(id)Sender
{
	[NSApp stopModalWithCode:1];
}

- (IBAction)OnEULADeclined:(id)Sender
{
	_Exit(0);
}

- (IBAction)OnQuitRequest:(id)Sender
{
	if (GEngine)
	{
		if (GIsEditor)
		{
			GEngine->DeferredCommands.Add( TEXT("CLOSE_SLATE_MAINFRAME") );
		}
		else
		{
			GEngine->DeferredCommands.Add( TEXT("EXIT") );
		}
	}
}

- (IBAction)OnShowAboutWindow:(id)Sender
{
#if WITH_EDITOR
	FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame")).ShowAboutWindow();
#else
	[NSApp orderFrontStandardAboutPanel: Sender];
#endif
}

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
#if !(UE_BUILD_SHIPPING && WITH_EDITOR) && WITH_EDITORONLY_DATA
	if ( FParse::Param( *GSavedCommandLine,TEXT("crashreports") ) )
	{
		GAlwaysReportCrash = true;
	}
#endif

	// OS X always uses the CrashReportClient, since the other paths aren't reliable
	GUseCrashReportClient = true;
	
#if WITH_EDITOR
	bHasFinishedLaunching = true;
	
	if(Filename != nil && !GSavedCommandLine.Contains(FString(Filename)))
	{
		GSavedCommandLine += TEXT(" ");
		FString Argument(Filename);
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
#endif

	bool bIsBuildMachine = false;
#if !UE_BUILD_SHIPPING
	if (FParse::Param(*GSavedCommandLine, TEXT("BUILDMACHINE")))
	{
		bIsBuildMachine = true;
	}
#endif

#if UE_BUILD_DEBUG
	if( true && !GAlwaysReportCrash )
#else
	if( FPlatformMisc::IsDebuggerPresent() && !GAlwaysReportCrash )
#endif
	{
		// Don't use exception handling when a debugger is attached to exactly trap the crash. This does NOT check
		// whether we are the first instance or not!
		GuardedMain( *GSavedCommandLine );
	}
	else
	{
		if (!bIsBuildMachine)
		{
			FPlatformMisc::SetCrashHandler(EngineCrashHandler);
		}
		GIsGuarded = 1;
		// Run the guarded code.
		GuardedMain( *GSavedCommandLine );
		GIsGuarded = 0;
	}

	FEngineLoop::AppExit();
	
	[NSApp terminate: nil];
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
