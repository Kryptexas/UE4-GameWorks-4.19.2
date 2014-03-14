// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#import <UIKit/UIKit.h>

#include "LaunchPrivatePCH.h"
#include "IOSAppDelegate.h"
#include "EAGLView.h"
#include "IOSCommandLineHelper.h"
#include "GameLaunchDaemonMessageHandler.h"

FEngineLoop GEngineLoop;
FGameLaunchDaemonMessageHandler GCommandSystem;

void FAppEntry::PreInit(IOSAppDelegate* AppDelegate, UIApplication* Application)
{
	// make a controller object
	AppDelegate.IOSController = [[IOSViewController alloc] init];
	
	// property owns it now
	[AppDelegate.IOSController release];

	// point to the GL view we want to use
	AppDelegate.RootView = [AppDelegate.IOSController view];

	if (AppDelegate.OSVersion >= 6.0f)
	{
		// this probably works back to OS4, but would need testing
		[AppDelegate.Window setRootViewController:AppDelegate.IOSController];
	}
	else
	{
		[AppDelegate.Window addSubview:AppDelegate.RootView];
	}

	// reset badge count on launch
	Application.applicationIconBadgeNumber = 0;
}

static void MainThreadInit()
{
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];

	// Size the view appropriately for any potentially dynamically attached displays,
	// prior to creating any framebuffers
	CGRect MainFrame = [[UIScreen mainScreen] bounds];
	if (!AppDelegate.bDeviceInPortraitMode)
	{
		Swap<float>(MainFrame.size.width, MainFrame.size.height);
	}
	
	// @todo: use code similar for presizing for secondary screens
// 	CGRect FullResolutionRect =
// 		CGRectMake(
// 		0.0f,
// 		0.0f,
// 		GSystemSettings.bAllowSecondaryDisplays ?
// 		Max<float>(MainFrame.size.width, GSystemSettings.SecondaryDisplayMaximumWidth)	:
// 		MainFrame.size.width,
// 		GSystemSettings.bAllowSecondaryDisplays ?
// 		Max<float>(MainFrame.size.height, GSystemSettings.SecondaryDisplayMaximumHeight) :
// 		MainFrame.size.height
// 		);
	CGRect FullResolutionRect = MainFrame;

	// create the fullscreen EAGLView
	AppDelegate.GLView = [[EAGLView alloc] initWithFrame:FullResolutionRect];
	AppDelegate.GLView.clearsContextBeforeDrawing = NO;
	AppDelegate.GLView.multipleTouchEnabled = YES;

	// add it to the window
	[AppDelegate.RootView addSubview:AppDelegate.GLView];

	// initialize the backbuffer of the view (so the RHI can use it)
	[AppDelegate.GLView CreateFramebuffer:YES];

	// Final adjustment to the viewport (this is deferred and won't run until the first engine tick)
	[FIOSAsyncTask CreateTaskWithBlock:^ bool (void)
		{
			// @todo ios: Figure out how to resize the viewport!!!!

// 			if (GEngine->GameViewport &&
// 				GEngine->GameViewport->Viewport &&
// 				GEngine->GameViewport->ViewportFrame)
// 			{
// 				CGFloat CSF = self.GLView.contentScaleFactor;
// 				GEngine->GameViewport->ViewportFrame->Resize(
// 					(INT)(CSF * self.GLView.superview.bounds.size.width),
// 					(INT)(CSF * self.GLView.superview.bounds.size.height),
// 					TRUE);
// 			}
			return true;
		}];
}



void FAppEntry::PlatformInit()
{

	// call a function in the main thread to do some processing that needs to happen there, now that the .ini files are loaded
	dispatch_async(dispatch_get_main_queue(), ^{ MainThreadInit(); });

	// wait until the GLView is fully initialized, so the RHI can be initialized
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	while (!AppDelegate.GLView || !AppDelegate.GLView->bIsInitialized)
	{
		FPlatformProcess::Sleep(0.001f);
	}

	// set the GL context to this thread
	[AppDelegate.GLView MakeCurrent];

}

void FAppEntry::Init()
{
	//extern TCHAR GCmdLine[16384];
	GEngineLoop.PreInit(FCommandLine::Get());

	// initialize messaging subsystem
	FModuleManager::LoadModuleChecked<IMessagingModule>("Messaging");

	//Set up the message handling to interface with other endpoints on our end.
	NSLog(@"%s", "Initializing ULD Communications in game mode\n");
	GCommandSystem.Init();

	// at this point, we can let the Controller/EAGLView 
	GLog->SetCurrentThreadAsMasterThread();

	// start up the engine
	GEngineLoop.Init();
}

static FSuspendRenderingThread* SuspendThread = NULL;

void FAppEntry::Tick()
{
    if (SuspendThread != NULL)
    {
        delete SuspendThread;
        SuspendThread = NULL;
        FPlatformProcess::SetRealTimeMode();
    }
    
	// tick the engine
	GEngineLoop.Tick();
}

void FAppEntry::SuspendTick()
{
    if (!SuspendThread)
    {
        SuspendThread = new FSuspendRenderingThread(true);
    }
    
    FPlatformProcess::Sleep(0.1f);
}

void FAppEntry::Shutdown()
{
	NSLog(@"%s", "Shutting down Game ULD Communications\n");
	GCommandSystem.Shutdown();
}

FString GSavedCommandLine;

int main(int argc, char *argv[])
{
	for(int Option = 1; Option < argc; Option++)
	{
		GSavedCommandLine += TEXT(" ");
		GSavedCommandLine += ANSI_TO_TCHAR(argv[Option]);
	}

	FIOSCommandLineHelper::InitCommandArgs(FString());
	
	@autoreleasepool {
	    return UIApplicationMain(argc, argv, nil, NSStringFromClass([IOSAppDelegate class]));
	}
}