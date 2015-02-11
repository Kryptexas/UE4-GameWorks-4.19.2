// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "StandaloneRendererPrivate.h"
#include "OpenGL/SlateOpenGLRenderer.h"
#include "MacWindow.h"
#include "MacTextInputMethodSystem.h"
#include "CocoaTextView.h"
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

@interface FSlateOpenGLLayer : NSOpenGLLayer
@property (assign) NSOpenGLContext* Context;
@property (assign) NSOpenGLPixelFormat* PixelFormat;
@end

@implementation FSlateOpenGLLayer

- (NSOpenGLPixelFormat *)openGLPixelFormatForDisplayMask:(uint32)Mask
{
	return self.PixelFormat;
}

- (NSOpenGLContext *)openGLContextForPixelFormat:(NSOpenGLPixelFormat *)PixelFormat
{
	return self.Context;
}

- (id)initWithContext:(NSOpenGLContext*)context andPixelFormat:(NSOpenGLPixelFormat*)pixelFormat
{
	self = [super init];
	if (self)
	{
		self.Context = context;
		self.PixelFormat = pixelFormat;
	}
	return self;
}

@end

@interface FSlateCocoaView : FCocoaTextView
@property (assign) NSOpenGLContext* Context;
@property (assign) NSOpenGLPixelFormat* PixelFormat;
@property (assign) FSlateOpenGLViewport* SlateViewport;
@end

@implementation FSlateCocoaView

- (CALayer*)makeBackingLayer
{
	return [[FSlateOpenGLLayer alloc] initWithContext:self.Context andPixelFormat:self.PixelFormat];
}

- (id)initWithFrame:(NSRect)frameRect context:(NSOpenGLContext*)context pixelFormat:(NSOpenGLPixelFormat*)pixelFormat andSlateViewport:(FSlateOpenGLViewport*)slateViewport
{
	self = [super initWithFrame:frameRect];
	if (self)
	{
		self.Context = context;
		self.PixelFormat = pixelFormat;
		self.SlateViewport = slateViewport;
	}
	return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
	self.SlateViewport->Draw();
}

- (BOOL)isOpaque
{
	return YES;
}

- (BOOL)mouseDownCanMoveWindow
{
	return YES;
}

@end

static void MacOpenGLContextReconfigurationCallBack(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void* UserInfo)
{
	FSlateOpenGLContext* Context = (FSlateOpenGLContext*)UserInfo;
	if (Context)
	{
		Context->bNeedsUpdate = true;
	}
}

FSlateOpenGLContext::FSlateOpenGLContext()
:	View(NULL)
,	PixelFormat(NULL)
,	Context(NULL)
,	ViewContext(NULL)
,	bNeedsUpdate(false)
{
}

FSlateOpenGLContext::~FSlateOpenGLContext()
{
	Destroy();
}

void FSlateOpenGLContext::Initialize(void* InWindow, const FSlateOpenGLContext* SharedContext)
{
	NSOpenGLPixelFormatAttribute Attributes[] =
	{
		kCGLPFAAccelerated,
		kCGLPFANoRecovery,
		kCGLPFASupportsAutomaticGraphicsSwitching,
		kCGLPFADoubleBuffer,
		kCGLPFAColorSize,
		32,
		0
	};

	PixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:Attributes];
	Context = [[NSOpenGLContext alloc] initWithFormat:PixelFormat shareContext:SharedContext ? SharedContext->Context : NULL];

	NSWindow* Window = (NSWindow*)InWindow;
	if (Window)
	{
		ViewContext = [[NSOpenGLContext alloc] initWithFormat:PixelFormat shareContext:Context];
		const NSRect ViewRect = NSMakeRect(0, 0, Window.frame.size.width, Window.frame.size.height);
		View = [[FSlateCocoaView alloc] initWithFrame:ViewRect context:ViewContext pixelFormat:PixelFormat andSlateViewport:Viewport];
		[View setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

		if (FPlatformMisc::IsRunningOnMavericks() && ([Window styleMask] & NSTexturedBackgroundWindowMask))
		{
			NSView* SuperView = [[Window contentView] superview];
			[SuperView addSubview:View];
			[SuperView setWantsLayer:YES];
			[SuperView addSubview:[Window standardWindowButton:NSWindowCloseButton]];
			[SuperView addSubview:[Window standardWindowButton:NSWindowMiniaturizeButton]];
			[SuperView addSubview:[Window standardWindowButton:NSWindowZoomButton]];
		}
		else
		{
			[View setWantsLayer:YES];
			[Window setContentView:View];
		}

		[[Window standardWindowButton:NSWindowCloseButton] setAction:@selector(performClose:)];

		View.layer.magnificationFilter = kCAFilterNearest;
		View.layer.minificationFilter = kCAFilterNearest;
	}

	[Context update];
	MakeCurrent();

	CGDisplayRegisterReconfigurationCallback(&MacOpenGLContextReconfigurationCallBack, this);
}

void FSlateOpenGLContext::Destroy()
{
	if (View)
	{
		NSWindow* Window = [View window];
		if (Window)
		{
			[Window setContentView:NULL];
		}
		[View release];
		View = NULL;

		// PixelFormat and Context are released by View
		CGDisplayRemoveReconfigurationCallback(&MacOpenGLContextReconfigurationCallBack, this);

		[PixelFormat release];
		[Context clearDrawable];
		[Context release];
		[ViewContext clearDrawable];
		[ViewContext release];
		PixelFormat = NULL;
		Context = NULL;
		ViewContext = NULL;
	}
}

void FSlateOpenGLContext::MakeCurrent()
{
	if (bNeedsUpdate)
	{
		[Context update];
		bNeedsUpdate = false;
	}
	[Context makeCurrentContext];
}
