//  Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
//
//  UDKRemoteAppDelegate.m
//  UDKRemote
//
//  Created by jadams on 7/28/10.
//

#import "UDKRemoteAppDelegate.h"
#import "MainViewController.h"

@implementation UDKRemoteAppDelegate

/** Default Ports to send to */
#define DEFAULT_PIP_PORT_NUMBER 41765
#define DEFAULT_PIE_PORT_NUMBER 41766


@synthesize window;
@synthesize mainViewController;
@synthesize PCAddress;
@synthesize bShouldIgnoreTilt;
@synthesize bShouldIgnoreTouch;
@synthesize bLockOrientation;
@synthesize LockedOrientation;
@synthesize RecentComputers;
@synthesize Ports;

extern time_t GAppInvokeTime;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions 
{    
	NSUserDefaults* Defaults = [NSUserDefaults standardUserDefaults];
    // initialize the (saved) PC address	
	self.PCAddress = [Defaults stringForKey:@"PCAddress"];
	
	// load tilt setting
	self.bShouldIgnoreTilt = [Defaults boolForKey:@"bShouldIgnoreTilt"];
	self.bShouldIgnoreTouch = [Defaults boolForKey:@"bShouldIgnoreTouch"];
	self.bLockOrientation = [Defaults boolForKey:@"bLockOrientation"];
	self.LockedOrientation = (UIInterfaceOrientation)[Defaults integerForKey:@"LockedOrientation"];
	
	self.RecentComputers = [NSMutableArray arrayWithArray:[Defaults stringArrayForKey:@"RecentComputers"]];
	self.Ports = [NSMutableArray arrayWithArray:[Defaults stringArrayForKey:@"Ports"]];
	if( [self.Ports count] == 0 )
	{
		//If default ports list was empty, add 2 default ports...
		NSString* DefaultPIEPort = [NSString stringWithFormat:@"%d", DEFAULT_PIE_PORT_NUMBER];
		[self.Ports addObject:DefaultPIEPort];
		NSString* DefaultPIPPort = [NSString stringWithFormat:@"%d", DEFAULT_PIP_PORT_NUMBER];
		[self.Ports addObject:DefaultPIPPort];
	}

	// load either iphone or ipad interface
	MainViewController *aController;
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		aController = [[MainViewController alloc] initWithNibName:@"MainViewLarge" bundle:nil];
	}
	else
	{
		aController = [[MainViewController alloc] initWithNibName:@"MainView" bundle:nil];
	}

	// set the mainviewcontroller to the one loaded from a .nib
	self.mainViewController = aController;
	self.window.rootViewController = aController;
	[aController release];
	
    mainViewController.view.frame = [UIScreen mainScreen].applicationFrame;
	[window addSubview:[mainViewController view]];
    [window makeKeyAndVisible];

	if (self.PCAddress == nil || [self.PCAddress compare:@""] == NSOrderedSame)
	{
		[aController FlipController:NO sender:aController.InfoButton];
	}
	
	return YES;
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
}

- (void)applicationWillResignActive:(UIApplication *)application
{
}

- (void)applicationWillTerminate:(UIApplication *)application
{
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	[mainViewController UpdateSocketAddr];
}

- (void)dealloc {
    [mainViewController release];
    [window release];
    [super dealloc];
}

@end
