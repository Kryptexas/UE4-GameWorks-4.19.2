// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>

@interface EAGLView : UIView
{
@public
	// are we initialized?
	bool bIsInitialized;

//@private
	// keeps track of the number of active touches
	// used to bring up the three finger touch debug console after 3 active touches are registered
	int NumActiveTouches;

	// the GL context
	EAGLContext* Context;

	// the internal MSAA FBO used to resolve the color buffer at present-time
	GLuint ResolveFrameBuffer;

	// track the touches by pointer (which will stay the same for a given finger down) - note we don't deref the pointers in this array
	UITouch* AllTouches[10];
}

@property (nonatomic) GLuint SwapCount;
@property (nonatomic) GLuint OnScreenColorRenderBuffer;
@property (nonatomic) GLuint OnScreenColorRenderBufferMSAA;

- (bool)CreateFramebuffer:(bool)bIsForOnDevice;
- (void)DestroyFramebuffer;

- (void)MakeCurrent;
- (void)UnmakeCurrent;
- (void)SwapBuffers;


// @todo zombie - redo input since Slate is iOS agnostic, completely, now.
void GetInputStack(/*TArray<FPointerEvent>& TouchEvents*/);

@end

/**
 * A view controller subclass that handles loading our GL view as well as autorotation
 */
@interface IOSViewController : UIViewController
{

}
@end
