// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IOSTapJoyPrivatePCH.h"

#import "../ThirdPartyFrameworks/Tapjoy.embeddedframework/Tapjoy.framework/Headers/Tapjoy.h"

class FTapJoyProvider : public IAdvertisingProvider
{
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** IAdvertisingProvider implementation */
	virtual void ShowAdBanner( bool bShowOnBottomOfScreen ) OVERRIDE;
	virtual void HideAdBanner() OVERRIDE;
	virtual void CloseAdBanner() OVERRIDE;
};

IMPLEMENT_MODULE( FTapJoyProvider, IOSTapJoy )

@interface IOSTapJoy : UIResponder
@end

@implementation IOSTapJoy
+ (IOSTapJoy*)GetDelegate
{
	static IOSTapJoy * Singleton = [[IOSTapJoy alloc] init];
	return Singleton;
}

-(void)tjcConnectSuccess:(NSNotification*)notifyObj
{
	NSLog(@"Tapjoy connect Succeeded");
}


- (void)tjcConnectFail:(NSNotification*)notifyObj
{
	NSLog(@"Tapjoy connect Failed");
}

- (void)StartupTapJoy
{
	// Tapjoy Connect Notifications
	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(tjcConnectSuccess:)
		name:TJC_CONNECT_SUCCESS
		object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(tjcConnectFail:)
		name:TJC_CONNECT_FAILED
		object:nil];

	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	[Tapjoy requestTapjoyConnect:@"AppID"
		secretKey:@"SecretKey"
		options:@{ TJC_OPTION_ENABLE_LOGGING : @(YES) }
	// If you are not using Tapjoy Managed currency, you would set your own user ID here.
	// TJC_OPTION_USER_ID : @"A_UNIQUE_USER_ID"

	// You can also set your event segmentation parameters here.
	// Example segmentationParams object -- NSDictionary *segmentationParams = @{@"iap" : @(YES)};
	// TJC_OPTION_SEGMENTATION_PARAMS : segmentationParams
	];
}
@end

void FTapJoyProvider::StartupModule() 
{
	[[IOSTapJoy GetDelegate] performSelectorOnMainThread:@selector(StartupTapJoy) withObject:nil waitUntilDone : NO];
}

void FTapJoyProvider::ShutdownModule()
{
}

void FTapJoyProvider::ShowAdBanner( bool bShowOnBottomOfScreen )
{
}

void FTapJoyProvider::HideAdBanner()
{

}

void FTapJoyProvider::CloseAdBanner()
{

}
