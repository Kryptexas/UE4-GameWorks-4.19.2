// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IOSAdvertising.h"

DEFINE_LOG_CATEGORY_STATIC( LogAdvertising, Display, All );

IMPLEMENT_MODULE( FIOSAdvertisingProvider, IOSAdvertising );

#if 0
#import <iAd/ADBannerView.h>

@interface IOSAdvertising : UIResponder <ADBannerViewDelegate>
	/** iAd banner view, if open */
	@property(retain) ADBannerView* BannerView;

	/**
	* Will show an iAd on the top or bottom of screen, on top of the GL view (doesn't resize
	* the view)
	*
	* @param bShowOnBottomOfScreen If true, the iAd will be shown at the bottom of the screen, top otherwise
	*/
	-(void)ShowAdBanner:(NSNumber*)bShowOnBottomOfScreen;
@end

@implementation IOSAdvertising

@synthesize BannerView;

/** TRUE if the iAd banner should be on the bottom of the screen */
BOOL bDrawOnBottom;

/** true if the user wants the banner to be displayed */
bool bWantVisibleBanner = false;

/**
* Will show an iAd on the top or bottom of screen, on top of the GL view (doesn't resize
* the view)
*
* @param bShowOnBottomOfScreen If true, the iAd will be shown at the bottom of the screen, top otherwise
*/
-(void)ShowAdBanner:(NSNumber*)bShowOnBottomOfScreen
{
	bDrawOnBottom = [bShowOnBottomOfScreen boolValue];
	bWantVisibleBanner = true;

	bool bNeedsToAddSubview = false;
	if (self.BannerView == nil)
	{
		self.BannerView = [[ADBannerView alloc] initWithAdType:ADAdTypeBanner];
		self.BannerView.delegate = self;
		bNeedsToAddSubview = true;
	}
	CGRect BannerFrame = CGRectZero;
	BannerFrame.size = [self.BannerView sizeThatFits : self.RootView.bounds.size];

	if (bDrawOnBottom)
	{
		// move to off the bottom
		BannerFrame.origin.y = self.RootView.bounds.size.height - BannerFrame.size.height;
	}

	self.BannerView.frame = BannerFrame;

	// start out hidden, will fade in when ad loads
	self.BannerView.hidden = YES;
	self.BannerView.alpha = 0.0f;

	if (bNeedsToAddSubview)
	{
		[self.RootView addSubview : self.BannerView];
	}
	else// if (self.BannerView.bannerLoaded)
	{
		[self bannerViewDidLoadAd : self.BannerView];
	}
}
@end
#endif

// Just call these implementations that are still in the engine for now
// We will eventually move all of this code to this file
extern CORE_API void IOSShowAdBanner(bool bShowOnBottomOfScreen);
extern CORE_API void IOSHideAdBanner();
extern CORE_API void IOSCloseAd();

void FIOSAdvertisingProvider::ShowAdBanner( bool bShowOnBottomOfScreen ) 
{
	//IOSShowAdBanner( bShowOnBottomOfScreen );
}

void FIOSAdvertisingProvider::HideAdBanner() 
{
	//IOSHideAdBanner();
}

void FIOSAdvertisingProvider::CloseAdBanner() 
{
	//IOSCloseAd();
}
