// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "EAGLView.h"
#include "IOSAppDelegate.h"
#include "IOS/IOSInputInterface.h"

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import <UIKit/UIGeometry.h>

@implementation EAGLView

@synthesize SwapCount, OnScreenColorRenderBuffer, OnScreenColorRenderBufferMSAA;

// @todo zombie - redo input since Slate is iOS agnostic, completely, now.
void GetInputStack(/*TArray<FPointerEvent>& TouchEvents*/)
{
	/*TouchEvents = GInputStack;

	GInputStack.Empty();*/
}

/**
 * @return The Layer Class for the window
 */
+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

-(BOOL)becomeFirstResponder
{
	return YES;
}

- (id)initWithFrame:(CGRect)Frame
{
	if ((self = [super initWithFrame:Frame]))
	{
		// Get the layer
		CAEAGLLayer *EaglLayer = (CAEAGLLayer *)self.layer;
		EaglLayer.opaque = YES;
		NSMutableDictionary* Dict = [NSMutableDictionary dictionary];
		[Dict setValue:[NSNumber numberWithBool:NO] forKey:kEAGLDrawablePropertyRetainedBacking];
		[Dict setValue:kEAGLColorFormatRGBA8 forKey:kEAGLDrawablePropertyColorFormat];
		EaglLayer.drawableProperties = Dict;

		// Initialize a single, static OpenGL ES 2.0 context, shared by all EAGLView objects
		Context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

		// delete this on failure
		if (!Context || ![EAGLContext setCurrentContext:Context]) 
		{
			[self release];
			return nil;
		}

		// Initialize some variables
		SwapCount = 0;

		FMemory::Memzero(AllTouches, sizeof(AllTouches));

		bIsInitialized = false;
	}
	return self;
}



- (bool)CreateFramebuffer:(bool)bIsForOnDevice
{
	if (!bIsInitialized)
	{
		// make sure this is current
		[self MakeCurrent];

		// @todo-mobile
		const float NativeScale = [[UIScreen mainScreen] scale];

		// look up the CVar for the scale factor
 		static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MobileContentScaleFactor")); 
 		float RequestedContentScaleFactor = CVar->GetFloat();

		// for TV screens, always use scale factor of 1
		self.contentScaleFactor = bIsForOnDevice ? RequestedContentScaleFactor : 1.0f;
		UE_LOG(LogIOS, Log, TEXT("Setting contentScaleFactor to %0.4f (optimal = %0.4f)"), self.contentScaleFactor, NativeScale);

		if (self.contentScaleFactor == 1.0f || self.contentScaleFactor == 2.0f)
		{
			UE_LOG(LogIOS,Log,TEXT("Setting layer filter to NEAREST"));
			CAEAGLLayer *EaglLayer = (CAEAGLLayer *)self.layer;
			EaglLayer.magnificationFilter = kCAFilterNearest;
		}

		// Create our standard displayable surface
		glGenRenderbuffers(1, &OnScreenColorRenderBuffer);
		check(glGetError() == 0);
		glBindRenderbuffer(GL_RENDERBUFFER, OnScreenColorRenderBuffer);
		check(glGetError() == 0);
		[Context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)self.layer];

		// Get the size of the surface
		GLint OnScreenWidth, OnScreenHeight;
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &OnScreenWidth);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &OnScreenHeight);

		// NOTE: This resolve FBO is necessary even if we don't plan on using MSAA because otherwise
		// the shaders will not warm properly. Future investigation as to why; it seems unnecessary.

		// Create an FBO used to target the resolve surface
		glGenFramebuffers(1, &ResolveFrameBuffer);
		check(glGetError() == 0);
		glBindFramebuffer(GL_FRAMEBUFFER, ResolveFrameBuffer);
		check(glGetError() == 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, OnScreenColorRenderBuffer);
		check(glGetError() == 0);

#if USE_DETAILED_IPHONE_MEM_TRACKING
		//This value is used to allow the engine to track gl allocated memory see GetIPhoneOpenGLBackBufferSize
		UINT singleBufferSize = OnScreenWidth * OnScreenHeight *  4/*rgba8*/;
		IPhoneBackBufferMemSize = singleBufferSize *3/*iphone back buffer system is tripple buffered*/;
#endif

#if USE_DETAILED_IPHONE_MEM_TRACKING
		UE_LOG(LogIOS, Log, TEXT("IPhone Back Buffer Size: %i MB"), (IPhoneBackBufferMemSize/1024)/1024.f);
#endif

		bIsInitialized = true;
	}    
	return true;
}

- (void)DestroyFramebuffer
{
	if (bIsInitialized)
	{
		// toss framebuffers
		if(ResolveFrameBuffer)
		{
			glDeleteFramebuffers(1, &ResolveFrameBuffer);
		}
		if(OnScreenColorRenderBuffer)
		{
			glDeleteRenderbuffers(1, &OnScreenColorRenderBuffer);
			OnScreenColorRenderBuffer = 0;
		}
// 		if( GMSAAAllowed )
// 		{
// 			if(OnScreenColorRenderBufferMSAA)
// 			{
// 				glDeleteRenderbuffers(1, &OnScreenColorRenderBufferMSAA);
// 				OnScreenColorRenderBufferMSAA = 0;
// 			}
// 		}

		// we are ready to be re-initialized
		bIsInitialized = false;
	}
}

- (void)MakeCurrent
{
	[EAGLContext setCurrentContext:Context];
}

- (void)UnmakeCurrent
{
	[EAGLContext setCurrentContext:nil];
}

- (void)SwapBuffers
{
	// We may need this in the MSAA case
//	GLint CurrentFramebuffer = 0;
	// @todo-mobile: Fix this when we have MSAA support
// 	if( GMSAAAllowed && GMSAAEnabled )
// 	{
// 		// Get the currently bound FBO
// 		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &CurrentFramebuffer);
// 
// 		// Set up and perform the resolve (the READ is already set)
// 		glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, ResolveFrameBuffer);
// 		glResolveMultisampleFramebufferAPPLE();
// 
// 		// After the resolve, we can discard the old attachments
// 		GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
// 		glDiscardFramebufferEXT( GL_READ_FRAMEBUFFER_APPLE, 3, attachments );
// 	}
// 	else
	//{
	//	// Discard the now-unncessary depth buffer
	//	GLenum attachments[] = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
	//	glDiscardFramebufferEXT( GL_READ_FRAMEBUFFER_APPLE, 2, attachments );
	//}

	// Perform the actual present with the on-screen renderbuffer
	//glBindRenderbuffer(GL_RENDERBUFFER, OnScreenColorRenderBuffer);
	[Context presentRenderbuffer:GL_RENDERBUFFER];

// 	if( GMSAAAllowed && GMSAAEnabled )
// 	{
// 		// Restore the DRAW framebuffer object
// 		glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, CurrentFramebuffer);
// 	}

	// increment our swap counter
	SwapCount++;
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
		TouchMessage.Position = FVector2D(MIN(self.frame.size.width - 1, Loc.x), MIN(self.frame.size.height - 1, Loc.y)) * Scale;
		TouchMessage.LastPosition = FVector2D(MIN(self.frame.size.width - 1, PrevLoc.x), MIN(self.frame.size.height - 1, PrevLoc.y)) * Scale;
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

@implementation IOSViewController

/**
 * The ViewController was created, so now we need to create our view to be controlled (an EAGLView)
 */
- (void) loadView
{
	// get the landcape size of the screen
	CGRect Frame = [[UIScreen mainScreen] bounds];
	if (![IOSAppDelegate GetDelegate].bDeviceInPortraitMode)
	{
		Swap<float>(Frame.size.width, Frame.size.height);
	}

	self.view = [[UIView alloc] initWithFrame:Frame];

	// settings copied from InterfaceBuilder
#if defined(__IPHONE_7_0)
	if ([IOSAppDelegate GetDelegate].OSVersion >= 7.0)
	{
		self.edgesForExtendedLayout = UIRectEdgeNone;
	}
#endif

	self.view.clearsContextBeforeDrawing = NO;
	self.view.multipleTouchEnabled = NO;
}

/**
 * View was unloaded from us
 */ 
- (void) viewDidUnload
{
	UE_LOG(LogIOS, Log, TEXT("IOSViewController unloaded the view. This is unexpected, tell Josh Adams"));
}

/**
 * Tell the OS that our view controller can auto-rotate between the two landscape modes
 */
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	//BOOL bIsValidOrientation;

	//// will the app let us rotate?
	//BOOL bCanRotate = [IPhoneAppDelegate GetDelegate].bCanAutoRotate;
	//
	//// is it one of the valid orientations to rotate to?
	//bIsValidOrientation = bCanRotate && 
	//	(((interfaceOrientation == UIInterfaceOrientationLandscapeLeft) && ![IOSAppDelegate GetDelegate].bDeviceInPortraitMode) || 
	//	((interfaceOrientation == UIInterfaceOrientationLandscapeRight) && ![IOSAppDelegate GetDelegate].bDeviceInPortraitMode) || 
	//	((interfaceOrientation == UIInterfaceOrientationPortrait) && [IOSAppDelegate GetDelegate].bDeviceInPortraitMode) ||
	//	((interfaceOrientation == UIInterfaceOrientationPortraitUpsideDown) && [IOSAppDelegate GetDelegate].bDeviceInPortraitMode));
	//	
	//return bIsValidOrientation;
	return YES;
}

/**
 * Tell the OS to hide the status bar (iOS 7 method for hiding)
 */
- (BOOL)prefersStatusBarHidden
{
	return YES;
}

@end
