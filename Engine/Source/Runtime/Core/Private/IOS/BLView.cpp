//
//  FMetalView.m
//  UE4
//
//  Created by Apple on 3/31/14.
//  Copyright (c) 2014 EpicGames. All rights reserved.
//

#import "BLView.h"
#include "IOSAppDelegate.h"
#include "IOS/IOSInputInterface.h"


#if HAS_METAL

@implementation FMetalView


{
	CAMetalLayer* RenderingLayer;
	
}


/**
 * @return The Layer Class for the window
 */
+ (Class)layerClass
{
	return [CAMetalLayer class];
}

-(BOOL)becomeFirstResponder
{
	return YES;
}

- (id)initWithFrame:(CGRect)Frame
{
	if ((self = [super initWithFrame:Frame]))
	{
		// grab the CAALayer created by the nib
		RenderingLayer = (CAMetalLayer*)self.layer;
		RenderingLayer.presentsWithTransaction = NO;
		RenderingLayer.drawsAsynchronously = YES;
		
		// set a background color to make sure the layer appears
		CGFloat components[] = {0.0, 0.0, 0.0, 1};
		RenderingLayer.backgroundColor = CGColorCreate(CGColorSpaceCreateDeviceRGB(),components);
	
		Device = MTLCreateSystemDefaultDevice();
		assert(Device);
		
		// set the device on the rendering layer and provide a pixel format
		RenderingLayer.device = Device;
		RenderingLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
		RenderingLayer.framebufferOnly = NO;

//
//		// Initialize some variables
//		SwapCount = 0;
//		
		FMemory::Memzero(AllTouches, sizeof(AllTouches));
		
		bIsInitialized = false;
	}
	return self;
}

-(id<CAMetalDrawable>)MakeDrawable
{
	id<CAMetalDrawable> Drawable = [RenderingLayer newDrawable];
//	check(Drawable != nil);
	return Drawable;
}





- (bool)CreateFramebuffer:(bool)bIsForOnDevice
{
	if (!bIsInitialized)
	{
		
//		// make sure this is current
//		[self MakeCurrent];
//		
		// @todo-mobile
		const float NativeScale = [[UIScreen mainScreen] scale];
		
		// look up the CVar for the scale factor
		static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MobileContentScaleFactor"));
		float RequestedContentScaleFactor = CVar->GetFloat();
		
		// for TV screens, always use scale factor of 1
		self.contentScaleFactor = bIsForOnDevice ? RequestedContentScaleFactor : 1.0f;
		UE_LOG(LogIOS, Log, TEXT("Setting contentScaleFactor to %0.4f (optimal = %0.4f)"), self.contentScaleFactor, NativeScale);
		
//		if (self.contentScaleFactor == 1.0f || self.contentScaleFactor == 2.0f)
//		{
//			UE_LOG(LogIOS,Log,TEXT("Setting layer filter to NEAREST"));
//			CAEAGLLayer *EaglLayer = (CAEAGLLayer *)self.layer;
//			EaglLayer.magnificationFilter = kCAFilterNearest;
//		}
//		
//#if USE_DETAILED_IPHONE_MEM_TRACKING
//		//This value is used to allow the engine to track gl allocated memory see GetIPhoneOpenGLBackBufferSize
//		UINT singleBufferSize = OnScreenWidth * OnScreenHeight *  4/*rgba8*/;
//		IPhoneBackBufferMemSize = singleBufferSize *3/*iphone back buffer system is tripple buffered*/;
//#endif
//		
//#if USE_DETAILED_IPHONE_MEM_TRACKING
//		UE_LOG(LogIOS, Log, TEXT("IPhone Back Buffer Size: %i MB"), (IPhoneBackBufferMemSize/1024)/1024.f);
//#endif
	
		bIsInitialized = true;
	}    
	return true;
}

/**
 * Returns the unique ID for the given touch
 */
-(int32) GetTouchIndex:(UITouch*)Touch
{
	
	// look for existing touch
	for (int Index = 0; Index < ARRAY_COUNT(AllTouches); Index++)
	{
		if (AllTouches[Index] == Touch)
		{
			return Index;
		}
	}
	
	// if we get here, it's a new touch, find a slot
	for (int Index = 0; Index < ARRAY_COUNT(AllTouches); Index++)
	{
		if (AllTouches[Index] == nil)
		{
			AllTouches[Index] = Touch;
			return Index;
		}
	}
	
	// if we get here, that means we are trying to use more than 5 touches at once, which is an error
	return -1;
}

/**
 * Pass touch events to the input queue for slate to pull off of, and trigger the debug console.
 *
 * @param View The view the event happened in
 * @param Touches Set of touch events from the OS
 */
-(void) HandleTouches:(NSSet*)Touches ofType:(TouchType)Type
{
	CGFloat Scale = self.contentScaleFactor;
	
	TArray<TouchInput> TouchesArray;
	for (UITouch* Touch in Touches)
	{
		// get info from the touch
		CGPoint Loc = [Touch locationInView:self];
		CGPoint PrevLoc = [Touch previousLocationInView:self];
		
		// convert TOuch pointer to a unique 0 based index
		int32 TouchIndex = [self GetTouchIndex:Touch];
		if (TouchIndex < 0)
		{
			continue;
		}
		
		// make a new touch event struct
		TouchInput TouchMessage;
		TouchMessage.Handle = TouchIndex;
		TouchMessage.Type = Type;
		TouchMessage.Position = FVector2D(FMath::Min<float>(self.frame.size.width - 1, Loc.x), FMath::Min<float>(self.frame.size.height - 1, Loc.y)) * Scale;
		TouchMessage.LastPosition = FVector2D(FMath::Min<float>(self.frame.size.width - 1, PrevLoc.x), FMath::Min<float>(self.frame.size.height - 1, PrevLoc.y)) * Scale;
		TouchesArray.Add(TouchMessage);
		
		// clear out the touch when it ends
		if (Type == TouchEnded)
		{
			AllTouches[TouchIndex] = nil;
		}
	}
	
	FIOSInputInterface::QueueTouchInput(TouchesArray);
}

/**
 * Handle the various touch types from the OS
 *
 * @param touches Array of touch events
 * @param event Event information
 */
- (void) touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	NumActiveTouches += touches.count;
	[self HandleTouches:touches ofType:TouchBegan];
	
#if !UE_BUILD_SHIPPING
#if WITH_SIMULATOR
	// use 2 on the simulator so that Option-Click will bring up console (option-click is for doing pinch gestures, which we don't care about, atm)
	if( NumActiveTouches >= 2 )
#else
		// If there are 3 active touches, bring up the console
		if( NumActiveTouches >= 4 )
#endif
		{
			// Route the command to the main iOS thread (all UI must go to the main thread)
			[[IOSAppDelegate GetDelegate] performSelectorOnMainThread:@selector(ShowConsole) withObject:nil waitUntilDone:NO];
		}
#endif
}

- (void) touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self HandleTouches:touches ofType:TouchMoved];
}

- (void) touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	NumActiveTouches -= touches.count;
	[self HandleTouches:touches ofType:TouchEnded];
}

- (void) touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	NumActiveTouches -= touches.count;
	[self HandleTouches:touches ofType:TouchEnded];
}


@end

#endif // METAL
