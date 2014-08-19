// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "CocoaWindow.h"
#include "CocoaTextView.h"
#include "MacApplication.h"
#include "MacWindow.h"

/**
 * Custom window class used for input handling
 */
@implementation FCocoaWindow

@synthesize bForwardEvents;
@synthesize TargetWindowMode;
@synthesize PreFullScreenRect;

- (id)initWithContentRect:(NSRect)ContentRect styleMask:(NSUInteger)Style backing:(NSBackingStoreType)BufferingType defer:(BOOL)Flag
{
	WindowMode = EWindowMode::Windowed;
	bAcceptsInput = false;
	bRoundedCorners = false;
	bDisplayReconfiguring = false;
	bDeferOrderFront = false;
	DeferOpacity = 0.0f;
	bRenderInitialised = false;
	bDeferSetFrame = false;
	bDeferSetOrigin = false;

	id NewSelf = [super initWithContentRect:ContentRect styleMask:Style backing:BufferingType defer:Flag];
	if(NewSelf)
	{
		bZoomed = [super isZoomed];
		self.bForwardEvents = true;
		self.TargetWindowMode = EWindowMode::Windowed;
		[super setAlphaValue:DeferOpacity];
		DeferFrame = [super frame];
		self.PreFullScreenRect = DeferFrame;
	}
	return NewSelf;
}

- (NSRect)openGLFrame
{
	if(self.TargetWindowMode == EWindowMode::Fullscreen || WindowMode == EWindowMode::Fullscreen)
	{
		return self.PreFullScreenRect;
	}
	else if([self styleMask] & (NSTexturedBackgroundWindowMask))
	{
		return (!bDeferSetFrame ? [self frame] : DeferFrame);
	}
	else
	{
		return (!bDeferSetFrame ? [[self contentView] frame] : [self contentRectForFrameRect:DeferFrame]);
	}
}


- (NSView*)openGLView
{
	if([self styleMask] & (NSTexturedBackgroundWindowMask))
	{
		NSView* SuperView = [[self contentView] superview];
		for(NSView* View in [SuperView subviews])
		{
			if([View isKindOfClass:[FCocoaTextView class]])
			{
				return View;
			}
		}
		return nil;
	}
	else
	{
		return [self contentView];
	}
}

- (void)performDeferredOrderFront
{
	if(!bRenderInitialised)
	{
		bRenderInitialised = true;
	}
	
	if(bDeferOrderFront)
	{
		if(!(bDeferSetFrame || bDeferSetOrigin))
		{
			bDeferOrderFront = false;
			[super setAlphaValue:DeferOpacity];
		}
		else
		{
			[self performDeferredSetFrame];
		}
	}
}

- (void)performDeferredSetFrame
{
	if(bRenderInitialised && (bDeferSetFrame || bDeferSetOrigin))
	{
		dispatch_block_t Block = ^{
			if(!bDeferSetFrame && bDeferSetOrigin)
			{
				DeferFrame.size = [self frame].size;
			}
			
			[super setFrame:DeferFrame display:YES];
		};
		
		if([NSThread isMainThread])
		{
			Block();
		}
		else
		{
			dispatch_async(dispatch_get_main_queue(), Block);
		}
		
		bDeferSetFrame = false;
		bDeferSetOrigin = false;
	}
}

- (void)orderWindow:(NSWindowOrderingMode)OrderingMode relativeTo:(NSInteger)OtherWindowNumber
{
	bool bModal = FMacWindow::CurrentModalWindow() == nil || FMacWindow::CurrentModalWindow() == self || [self styleMask] == NSBorderlessWindowMask;
	if(OrderingMode == NSWindowOut || bModal)
	{
		if([self alphaValue] > 0.0f)
		{
			[self performDeferredSetFrame];
		}
		[super orderWindow:OrderingMode relativeTo:OtherWindowNumber];
	}
}

- (bool)roundedCorners
{
    return bRoundedCorners;
}

- (void)setRoundedCorners:(bool)bUseRoundedCorners
{
	bRoundedCorners = bUseRoundedCorners;
}

- (void)setAcceptsInput:(bool)InAcceptsInput
{
	bAcceptsInput = InAcceptsInput;
}

- (void)redrawContents
{
	if(bNeedsRedraw && bForwardEvents && ([self isVisible] && [super alphaValue] > 0.0f))
	{
		MacApplication->OnWindowRedrawContents( self );
	}
	bNeedsRedraw = false;
}

- (void)setWindowMode:(EWindowMode::Type)NewWindowMode
{
	WindowMode = NewWindowMode;
}

- (EWindowMode::Type)windowMode
{
	return WindowMode;
}

- (void)setDisplayReconfiguring:(bool)bIsDisplayReconfiguring
{
	bDisplayReconfiguring = bIsDisplayReconfiguring;
}

- (void)orderFrontAndMakeMain:(bool)bMain andKey:(bool)bKey
{
	if ([NSApp isHidden] == NO)
	{
		bool bBringToFront = FMacWindow::CurrentModalWindow() == nil || FMacWindow::CurrentModalWindow() == self || [self styleMask] == NSBorderlessWindowMask;
		if (bBringToFront)
		{
			[self orderFront:nil];
		}
		
		if (bMain && [self canBecomeMainWindow] && self != [NSApp mainWindow])
		{
			[self makeMainWindow];
		}
		if (bKey && [self canBecomeKeyWindow] && self != [NSApp keyWindow])
		{
			[self makeKeyWindow];
		}
	}
}

// Following few methods overload NSWindow's methods from Cocoa API, so have to use Cocoa's BOOL (signed char), not bool (unsigned int)
- (BOOL)canBecomeMainWindow
{
	bool bNoModalOrCurrent = FMacWindow::CurrentModalWindow() == nil || FMacWindow::CurrentModalWindow() == self;
	return bAcceptsInput && ([self styleMask] != NSBorderlessWindowMask) && bNoModalOrCurrent;
}

- (BOOL)canBecomeKeyWindow
{
	return bAcceptsInput && ![self ignoresMouseEvents];
}

- (BOOL)validateMenuItem:(NSMenuItem *)MenuItem
{
	// Borderless windows we use do not automatically handle first responder's actions, so we force it here
	return ([MenuItem action] == @selector(performClose:) || [MenuItem action] == @selector(performMiniaturize:) || [MenuItem action] == @selector(performZoom:)) ? YES : [super validateMenuItem:MenuItem];
}

- (void)setAlphaValue:(CGFloat)WindowAlpha
{
	if(!bRenderInitialised)
	{
		DeferOpacity = WindowAlpha;
		bDeferOrderFront = true;
	}
	else
	{
		if([self isVisible] && WindowAlpha > 0.0f)
		{
			[self performDeferredSetFrame];
		}
		[super setAlphaValue:WindowAlpha];
	}
}

- (void)orderOut:(id)Sender
{
	bDeferOrderFront = false;
	
	[super orderOut:Sender];
}

- (void)performClose:(id)Sender
{
	bDeferOrderFront = false;
	
	[self close];
}

- (void)performMiniaturize:(id)Sender
{
	[self miniaturize: self];
}

- (void)performZoom:(id)Sender
{
	bZoomed = !bZoomed;
	[self zoom: self];
}

- (void)setFrame:(NSRect)FrameRect display:(BOOL)Flag
{
	NSSize Size = [self frame].size;
	NSSize NewSize = FrameRect.size;
	if(!bRenderInitialised || ([self isVisible] && [super alphaValue] > 0.0f && (Size.width > 1 || Size.height > 1 || NewSize.width > 1 || NewSize.height > 1)))
	{
		[super setFrame:FrameRect display:Flag];
		bDeferSetFrame = false;
	}
	else
	{
		bDeferSetFrame = true;
		DeferFrame = FrameRect;
		if(self.bForwardEvents)
		{
			MacApplication->OnWindowDidResize( self );
		}
	}
}

- (void)setFrameOrigin:(NSPoint)Point
{
	NSSize Size = [self frame].size;
	if(!bRenderInitialised || ([self isVisible] && [super alphaValue] > 0.0f && (Size.width > 1 || Size.height > 1)))
	{
		[super setFrameOrigin:Point];
		bDeferSetOrigin = false;
	}
	else
	{
		bDeferSetOrigin = true;
		DeferFrame.origin = Point;
	}
}

- (void)keyDown:(NSEvent *)Event
{
	if(self.bForwardEvents)
	{
		MacApplication->ProcessEvent( Event );
	}
}

- (void)keyUp:(NSEvent *)Event
{
	if(self.bForwardEvents)
	{
		MacApplication->ProcessEvent( Event );
	}
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
	// Handle clicking on the titlebar fullscreen item
	if(self.TargetWindowMode == EWindowMode::Windowed)
	{
		// @todo: Fix fullscreen mode mouse coordinate handling - for now default to windowed fullscreen
		self.TargetWindowMode = EWindowMode::WindowedFullscreen;
	}
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	WindowMode = self.TargetWindowMode;
	if(self.bForwardEvents)
	{
		MacApplication->OnWindowDidResize( self );
	}
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	WindowMode = EWindowMode::Windowed;
	self.TargetWindowMode = EWindowMode::Windowed;
	if(self.bForwardEvents)
	{
		MacApplication->OnWindowDidResize( self );
	}
}

- (void)windowDidBecomeKey:(NSNotification *)Notification
{
	if([NSApp isHidden] == NO)
	{
		if(FMacWindow::CurrentModalWindow() == nil || FMacWindow::CurrentModalWindow() == self || [self styleMask] == NSBorderlessWindowMask)
		{
			[self orderFrontAndMakeMain:false andKey:false];
		}
		else
		{
			[FMacWindow::CurrentModalWindow() orderFrontAndMakeMain:true andKey:true];
		}
	}
	
	if(self.bForwardEvents)
	{
		MacApplication->OnWindowDidBecomeKey( self );
	}
}

- (void)windowDidResignKey:(NSNotification *)Notification
{
	[self setMovable: YES];
	[self setMovableByWindowBackground: NO];
	
	if(self.bForwardEvents)
	{
		MacApplication->OnWindowDidResignKey( self );
	}
}

- (void)windowWillMove:(NSNotification *)Notification
{
	if(self.bForwardEvents)
	{
		MacApplication->OnWindowWillMove( self );
	}
}

- (void)windowDidMove:(NSNotification *)Notification
{
	bZoomed = [self isZoomed];
	
	NSView* OpenGLView = [self openGLView];
	[[NSNotificationCenter defaultCenter] postNotificationName:NSViewGlobalFrameDidChangeNotification object:OpenGLView];
	
	if(self.bForwardEvents)
	{
		MacApplication->OnWindowDidMove( self );
	}
}

- (void)windowDidChangeScreen:(NSNotification *)notification
{
	// You'd think this would be a good place to handle un/parenting...
	// However, do that here and your dragged window will disappear when you drag it onto another screen!
	// The windowdidChangeScreen notification only comes after you finish dragging.
	// It does however, work fine for handling display arrangement changes that cause a window to go offscreen.
	if(bDisplayReconfiguring)
	{
		NSScreen* Screen = [self screen];
		NSRect Frame = [self frame];
		NSRect VisibleFrame = [Screen visibleFrame];
		if(NSContainsRect(VisibleFrame, Frame) == NO)
		{
			// May need to scale the window to fit if it is larger than the new display.
			if (Frame.size.width > VisibleFrame.size.width || Frame.size.height > VisibleFrame.size.height)
			{
				NSRect NewFrame;
				NewFrame.size.width = Frame.size.width > VisibleFrame.size.width ? VisibleFrame.size.width : Frame.size.width;
				NewFrame.size.height = Frame.size.height > VisibleFrame.size.height ? VisibleFrame.size.height : Frame.size.height;
				NewFrame.origin = VisibleFrame.origin;
				
				[self setFrame:NewFrame display:NO];
			}
			else
			{
				NSRect Intersection = NSIntersectionRect(VisibleFrame, Frame);
				NSPoint Origin = Frame.origin;
				
				// If there's at least something on screen, try shifting it entirely on screen.
				if(Intersection.size.width > 0 && Intersection.size.height > 0)
				{
					CGFloat X = Frame.size.width - Intersection.size.width;
					CGFloat Y = Frame.size.height - Intersection.size.height;
					
					if(Intersection.size.width+Intersection.origin.x >= VisibleFrame.size.width+VisibleFrame.origin.x)
					{
						Origin.x -= X;
					}
					else if(Origin.x < VisibleFrame.origin.x)
					{
						Origin.x += X;
					}
					
					if(Intersection.size.height+Intersection.origin.y >= VisibleFrame.size.height+VisibleFrame.origin.y)
					{
						Origin.y -= Y;
					}
					else if(Origin.y < VisibleFrame.origin.y)
					{
						Origin.y += Y;
					}
				}
				else
				{
					Origin = VisibleFrame.origin;
				}
				
				[self setFrameOrigin:Origin];
			}
		}
	}
}

- (void)windowDidResize:(NSNotification *)Notification
{
	bZoomed = [self isZoomed];
	if(self.bForwardEvents)
	{
		MacApplication->OnWindowDidResize( self );
	}
	bNeedsRedraw = true;
}

- (void)windowWillClose:(NSNotification *)notification
{
	if(self.bForwardEvents && MacApplication)
	{
		MacApplication->OnWindowDidClose( self );
	}
}

- (void)mouseDown:(NSEvent*)Event
{
	if(self.bForwardEvents)
	{
		MacApplication->ProcessEvent( Event );
	}
}

- (void)rightMouseDown:(NSEvent*)Event
{
	// Really we shouldn't be doing this - on OS X only left-click changes focus,
	// but for the moment it is easier than changing Slate.
	if([self canBecomeKeyWindow])
	{
		[self makeKeyWindow];
	}
	
	if(self.bForwardEvents)
	{
		MacApplication->ProcessEvent( Event );
	}
}

- (void)otherMouseDown:(NSEvent*)Event
{
	if(self.bForwardEvents)
	{
		MacApplication->ProcessEvent( Event );
	}
}

- (void)mouseUp:(NSEvent*)Event
{
	if(self.bForwardEvents)
	{
		MacApplication->ProcessEvent( Event );
	}
}

- (void)rightMouseUp:(NSEvent*)Event
{
	if(self.bForwardEvents)
	{
		MacApplication->ProcessEvent( Event );
	}
}

- (void)otherMouseUp:(NSEvent*)Event
{
	if(self.bForwardEvents)
	{
		MacApplication->ProcessEvent( Event );
	}
}

- (NSDragOperation)draggingEntered:(id < NSDraggingInfo >)Sender
{
	return NSDragOperationGeneric;
}

- (void)draggingExited:(id < NSDraggingInfo >)Sender
{
	if(self.bForwardEvents)
	{
		MacApplication->OnDragOut( self );
	}
}

- (NSDragOperation)draggingUpdated:(id < NSDraggingInfo >)Sender
{
	if(self.bForwardEvents)
	{
		MacApplication->OnDragOver( self );
	}
	return NSDragOperationGeneric;
}

- (BOOL)prepareForDragOperation:(id < NSDraggingInfo >)Sender
{
	if(self.bForwardEvents)
	{
		MacApplication->OnDragEnter( self, [Sender draggingPasteboard] );
	}
	return YES;
}

- (BOOL)performDragOperation:(id < NSDraggingInfo >)Sender
{
	if(self.bForwardEvents)
	{
		MacApplication->OnDragDrop( self );
	}
	return YES;
}

- (BOOL)isMovable
{
	BOOL Movable = [super isMovable];
	if(Movable && bRenderInitialised && MacApplication)
	{
		Movable &= MacApplication->IsWindowMovable(self, NULL);
	}
	return Movable;
}

@end

/**
 * Custom window class used for mouse capture
 */
@implementation FMouseCaptureWindow

- (id)initWithTargetWindow: (FCocoaWindow*)Window
{
	self = [super initWithContentRect: [[Window screen] frame] styleMask: NSBorderlessWindowMask backing: NSBackingStoreBuffered defer: NO];
	[self setBackgroundColor: [NSColor clearColor]];
	[self setOpaque: NO];
	[self setLevel: NSMainMenuWindowLevel + 1];
	[self setIgnoresMouseEvents: NO];
	[self setAcceptsMouseMovedEvents: YES];
	[self setHidesOnDeactivate: YES];
	
	TargetWindow = Window;
	
	return self;
}

- (FCocoaWindow*)targetWindow
{
	return TargetWindow;
}

- (void)setTargetWindow: (FCocoaWindow*)Window
{
	TargetWindow = Window;
}

- (void)mouseDown:(NSEvent*)Event
{
	MacApplication->ProcessEvent( Event );
}

- (void)rightMouseDown:(NSEvent*)Event
{
	MacApplication->ProcessEvent( Event );
}

- (void)otherMouseDown:(NSEvent*)Event
{
	MacApplication->ProcessEvent( Event );
}

- (void)mouseUp:(NSEvent*)Event
{
	MacApplication->ProcessEvent( Event );
}

- (void)rightMouseUp:(NSEvent*)Event
{
	MacApplication->ProcessEvent( Event );
}

- (void)otherMouseUp:(NSEvent*)Event
{
	MacApplication->ProcessEvent( Event );
}

@end
