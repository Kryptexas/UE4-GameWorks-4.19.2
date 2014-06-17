//
//  FMTLView.h
//  UE4
//
//  Created by Apple on 3/31/14.
//  Copyright (c) 2014 EpicGames. All rights reserved.
//

#import <UIKit/UIKit.h>

#if HAS_METAL
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface FMetalView : UIView
{
@public
	bool bIsInitialized;

	id<MTLDevice> Device;

	// keeps track of the number of active touches
	// used to bring up the three finger touch debug console after 3 active touches are registered
	int NumActiveTouches;

	// track the touches by pointer (which will stay the same for a given finger down) - note we don't deref the pointers in this array
	UITouch* AllTouches[10];
}

- (bool)CreateFramebuffer:(bool)bIsForOnDevice;

- (id<CAMetalDrawable>)MakeDrawable;

@end

#endif
