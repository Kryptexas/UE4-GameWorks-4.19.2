// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "MacWindow.h"
#include "MacApplication.h"

/**
 * Custom window class used for input handling
 */
@implementation FSlateCocoaWindow

- (id)initWithContentRect:(NSRect)ContentRect styleMask:(NSUInteger)Style backing:(NSBackingStoreType)BufferingType defer:(BOOL)Flag
{
	id NewSelf = [super initWithContentRect:ContentRect styleMask:Style backing:BufferingType defer:Flag];
	if(NewSelf)
	{
		Parent = NULL;
		WindowMode = EWindowMode::Windowed;
		ChildWindows = nil;;
		bAcceptsInput = false;
		bRoundedCorners = false;
		bDisplayReconfiguring = false;
		bDeferOrderFront = false;
		bDeferOpacity = 0.0f;
		bRenderInitialised = false;
		bZoomed = [super isZoomed];
		[super setAlphaValue:bDeferOpacity];
	}
	return NewSelf;
}

- (NSRect)openGLFrame
{
	if([self styleMask] & (NSTexturedBackgroundWindowMask))
	{
		return [self frame];
	}
	else
	{
		return [[self contentView] frame];
	}
}

- (void)performDeferredOrderFront
{
	if(!bRenderInitialised)
	{
		bRenderInitialised = true;
		
		if(bDeferOrderFront)
		{
			bDeferOrderFront = false;
			
			[self setAlphaValue:bDeferOpacity];
		}
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
	MacApplication->OnWindowRedrawContents( self );
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

- (void)setParent:(NSWindow*)InParent
{
	Parent = InParent;
}

- (void)orderFrontEvenIfChildAndMakeMain:(bool)bMain andKey:(bool)bKey
{
	NSWindow* ParentWindow = [self getParent];
	if( ParentWindow && ![ParentWindow isMiniaturized] && NSContainsRect([[ParentWindow screen] frame], [self frame]) )
	{
		[ParentWindow removeChildWindow: self];
		[ParentWindow addChildWindow: self ordered: NSWindowAbove];
	}
	
	[self orderFront:nil];
	
	if(bMain && [self canBecomeMainWindow])
	{
		[self makeMainWindow];
	}
	if(bKey && [self canBecomeKeyWindow])
	{
		[self makeKeyWindow];
	}
}

- (void)disconnectChildWindows
{
	ChildWindows = [[NSMutableArray alloc] initWithArray: [self childWindows]];
	for( int32 Index = 0; Index < [ChildWindows count]; Index++ )
	{
		NSWindow* Window = (NSWindow*)[ChildWindows objectAtIndex: Index];
		[self removeChildWindow: Window];
		[Window setLevel: NSFloatingWindowLevel];
	}
	if(Parent && [self parentWindow])
	{
		[Parent removeChildWindow:self];
		[self setLevel: NSFloatingWindowLevel];
	}
}

- (void)reconnectChildWindows
{
	if( ChildWindows )
	{
		for( int32 Index = 0; Index < [ChildWindows count]; Index++ )
		{
			NSWindow* Window = (NSWindow*)[ChildWindows objectAtIndex: Index];
			if(NSContainsRect([[Window screen] frame], [self frame]) && ![self isMiniaturized])
			{
				[self addChildWindow: Window ordered: NSWindowAbove];
				[Window setLevel: ([Window styleMask] & NSClosableWindowMask) ? NSNormalWindowLevel : NSFloatingWindowLevel];
			}
		}
		[ChildWindows release];
		ChildWindows = NULL;
	}
	if(Parent && ![Parent isMiniaturized] && [self parentWindow] == nil && NSContainsRect([[Parent screen] frame], [self frame]))
	{
		[Parent addChildWindow: self ordered: NSWindowAbove];
		[self setLevel: ([self styleMask] & NSClosableWindowMask) ? NSNormalWindowLevel : NSFloatingWindowLevel];
	}
}

- (bool)shouldAddChildWindows
{
	return (ChildWindows == NULL) && ![self isMiniaturized];
}

- (NSWindow*)getParent
{
	return Parent;
}

- (void)removeCachedChild:(NSWindow*)InChild
{
	if(ChildWindows && InChild)
	{
		[ChildWindows removeObject:InChild];
	}
}

// Following few methods overload NSWindow's methods from Cocoa API, so have to use Cocoa's BOOL (signed char), not bool (unsigned int)
- (BOOL)canBecomeMainWindow
{
	return bAcceptsInput && ([self styleMask] != NSBorderlessWindowMask);
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
		bDeferOpacity = WindowAlpha;
		bDeferOrderFront = true;
	}
	else
	{
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

// keyDown and keyUp are empty, but having them lets Cocoa know we handle the keys ourselves
- (void)keyDown:(NSEvent *)Event
{
}

- (void)keyUp:(NSEvent *)Event
{
}

- (void)miniaturize:(id)sender
{
	// If parented detach or the whole app will hide, though the mouse handling will still be directed through our app.
	// There's a reason why Inspector windows in Cocoa apps don't have a minimise button...
	if(Parent)
	{
		[Parent removeChildWindow:self];
	}
	[super miniaturize:sender];
}

- (void)windowDidDeminiaturize:(NSNotification*)Notification
{
	// If parented ensure that the parent is visible and then reattach, to match Windows behaviour.
	if(Parent)
	{
		if([Parent isMiniaturized] == YES)
		{
			[Parent deminiaturize:self];
		}
		[Parent addChildWindow:self ordered: NSWindowAbove];
	}
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	WindowMode = EWindowMode::WindowedFullscreen;
	MacApplication->OnWindowDidResize( self );
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	WindowMode = EWindowMode::Windowed;
	MacApplication->OnWindowDidResize( self );
}

- (void)windowDidBecomeKey:(NSNotification *)Notification
{
	if([NSApp isHidden] == NO)
	{
		// First, make sure we're in front - on our child window level.
		// That's what happens on PC, and engine doesn't give us any BringToFront() calls.
		[self orderFrontEvenIfChildAndMakeMain:false andKey:false];
	}

	MacApplication->OnWindowDidBecomeKey( self );
}

- (void)windowDidResignKey:(NSNotification *)Notification
{
	[self setMovable: YES];
	[self setMovableByWindowBackground: NO];
	
	MacApplication->OnWindowDidResignKey( self );
}

- (void)windowWillMove:(NSNotification *)Notification
{
	MacApplication->OnWindowWillMove( self );
}

- (void)windowDidMove:(NSNotification *)Notification
{
	bZoomed = [self isZoomed];
	MacApplication->OnWindowDidMove( self );
}

- (void)windowDidChangeScreen:(NSNotification *)notification
{
	// You'd think this would be a good place to handle un/parenting...
	// However, do that here and your dragged window will disappear when you drag it onto another screen!
	// The windowdidChangeScreen notification only comes after you finish dragging.
	// It does however, work fine for handling display arrangement changes that cause a window to go offscreen.
	if(bDisplayReconfiguring)
	{
		if(Parent && ![Parent isMiniaturized] && [self parentWindow] == nil && NSContainsRect([[Parent screen] frame], [self frame]))
		{
			[Parent addChildWindow: self ordered: NSWindowAbove];
		}
		else if(Parent && [self parentWindow])
		{
			[Parent removeChildWindow:self];
		}
		
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
	MacApplication->OnWindowDidResize( self );
}

- (void)windowWillClose:(NSNotification *)notification
{
	MacApplication->OnWindowDidClose( self );
}

- (void)mouseDown:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)rightMouseDown:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
	
	// Really we shouldn't be doing this - on OS X only left-click changes focus,
	// but for the moment it is easier than changing Slate.
	if([self canBecomeKeyWindow])
	{
		[self makeKeyWindow];
	}
}

- (void)otherMouseDown:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)mouseUp:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)rightMouseUp:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)otherMouseUp:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (NSDragOperation)draggingEntered:(id < NSDraggingInfo >)Sender
{
	return NSDragOperationGeneric;
}

- (void)draggingExited:(id < NSDraggingInfo >)Sender
{
	MacApplication->OnDragOut( self );
}

- (NSDragOperation)draggingUpdated:(id < NSDraggingInfo >)Sender
{
	MacApplication->OnDragOver( self );
	return NSDragOperationGeneric;
}

- (BOOL)prepareForDragOperation:(id < NSDraggingInfo >)Sender
{
	MacApplication->OnDragEnter( self, [Sender draggingPasteboard] );
	return YES;
}

- (BOOL)performDragOperation:(id < NSDraggingInfo >)Sender
{
	MacApplication->OnDragDrop( self );
	return YES;
}

- (BOOL)isMovable
{
	BOOL Movable = (BOOL)(MacApplication->IsWindowMovable(self, NULL) && [super isMovable]);
	return Movable;
}

@end

/**
 * Custom window class used for mouse capture
 */
@implementation FMouseCaptureWindow

- (id)initWithTargetWindow: (FSlateCocoaWindow*)Window
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

- (FSlateCocoaWindow*)targetWindow
{
	return TargetWindow;
}

- (void)setTargetWindow: (FSlateCocoaWindow*)Window
{
	TargetWindow = Window;
}

- (void)mouseDown:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)rightMouseDown:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)otherMouseDown:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)mouseUp:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)rightMouseUp:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

- (void)otherMouseUp:(NSEvent*)Event
{
	MacApplication->AddPendingEvent( Event );
}

@end


FMacWindow::~FMacWindow()
{
	// While on Windows invalid HWNDs fail silently, accessing an invalid NSWindow is fatal.
	// So instead we release the window here.
	if(WindowHandle)
	{
		[WindowHandle release];
		WindowHandle = nil;
	}
}

TSharedRef<FMacWindow> FMacWindow::Make()
{
	// First, allocate the new native window object.  This doesn't actually create a native window or anything,
	// we're simply instantiating the object so that we can keep shared references to it.
	return MakeShareable( new FMacWindow() );
}

void FMacWindow::Initialize( FMacApplication* const Application, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FMacWindow >& InParent, const bool bShowImmediately )
{
	SCOPED_AUTORELEASE_POOL;

	OwningApplication = Application;
	Definition = InDefinition;

	// Finally, let's initialize the new native window object.  Calling this function will often cause OS
	// window messages to be sent! (such as activation messages)

	const int32 X = FMath::Trunc( Definition->XDesiredPositionOnScreen );

	NSScreen* TargetScreen = Application->FindScreenByPoint( X, Definition->YDesiredPositionOnScreen );

	int32 Y = FMath::Trunc( Definition->YDesiredPositionOnScreen );

	// Make sure it's not under the menu bar on whatever display being targeted
	const int32 ScreenHeight = FMath::Trunc( [TargetScreen frame].size.height );
	const int32 VisibleHeight = FMath::Trunc( [TargetScreen visibleFrame].origin.y + [TargetScreen visibleFrame].size.height );
	Y = FMath::Max( Y, ScreenHeight - VisibleHeight );

	const int32 SizeX = FMath::Max(FMath::Trunc( Definition->WidthDesiredOnScreen ), 1);
	const int32 SizeY = FMath::Max(FMath::Trunc( Definition->HeightDesiredOnScreen ), 1);

	PositionX = X;
	PositionY = Y;

	const NSRect ViewRect = NSMakeRect(X, Y, SizeX, SizeY);

	uint32 WindowStyle = 0;
	if( Definition->IsRegularWindow )
	{
		WindowStyle = NSClosableWindowMask;
		
		// In order to support rounded, shadowed windows set the window to be
		// titled - we'll set the OpenGL view to cover the whole window
		WindowStyle |= NSTitledWindowMask | NSTexturedBackgroundWindowMask;
		
		if( Definition->SupportsMinimize )
		{
			WindowStyle |= NSMiniaturizableWindowMask;
		}
		if( Definition->SupportsMaximize )
		{
			WindowStyle |= NSResizableWindowMask;
		}
	}
	else
	{
		WindowStyle = NSBorderlessWindowMask;
	}

	if( Definition->HasOSWindowBorder )
	{
		WindowStyle |= NSTitledWindowMask;
		WindowStyle &= ~(NSTexturedBackgroundWindowMask);
	}

	WindowHandle = [[FSlateCocoaWindow alloc] initWithContentRect: ViewRect styleMask: WindowStyle backing: NSBackingStoreBuffered defer: NO];

	if( WindowHandle == NULL )
	{
		// @todo Error message should be localized!
		NSRunInformationalAlertPanel( @"Error", @"Window creation failed!", @"Yes", NULL, NULL );
		check(0);
		return;
	}
	
	// Disable automatic release on close - explicit retain/release makes the Destroy() logic much easier to follow
	[WindowHandle setReleasedWhenClosed: NO];
	
	[WindowHandle setWindowMode: WindowMode];
	[WindowHandle setAcceptsInput: Definition->AcceptsInput];
	[WindowHandle setDisplayReconfiguring: false];
	[WindowHandle setAcceptsMouseMovedEvents: YES];
	[WindowHandle setDelegate: WindowHandle];
	[WindowHandle setLevel: Definition->IsRegularWindow ? NSNormalWindowLevel : NSFloatingWindowLevel];
	
	// Use of rounded corners will always render with system values for rounding
	[WindowHandle setRoundedCorners: (Definition->CornerRadius != 0)];
	
	if( !Definition->HasOSWindowBorder )
	{
		[WindowHandle setBackgroundColor: [NSColor darkGrayColor]];
		[WindowHandle setHasShadow: YES];
	}

	[WindowHandle setOpaque: NO];

	ReshapeWindow( X, Y, SizeX, SizeY );

	if( Definition->IsRegularWindow )
	{
		CFStringRef CFName = FPlatformString::TCHARToCFString( *Definition->Title );
		[WindowHandle setTitle: ( NSString *)CFName];
		CFRelease( CFName );

		[NSApp addWindowsItem: WindowHandle title: [WindowHandle title] filename: NO];

		// Tell Cocoa that we are opting into drag and drop.
		// Only makes sense for regular windows (windows that last a while.)
		[WindowHandle registerForDraggedTypes: [NSArray arrayWithObject: NSFilenamesPboardType]];

		[WindowHandle setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
	}

	if( InParent.IsValid() )
	{
		FSlateCocoaWindow* ParentWindowHandle = static_cast<FSlateCocoaWindow*>(InParent->WindowHandle);
		[WindowHandle setParent:ParentWindowHandle];
		if(![ParentWindowHandle shouldAddChildWindows] || !NSContainsRect([[ParentWindowHandle screen] frame], [WindowHandle frame]))
		{
			[WindowHandle setLevel: ([WindowHandle styleMask] & NSClosableWindowMask) ? NSNormalWindowLevel : NSFloatingWindowLevel];
		}
	}
	else
	{
		[WindowHandle setParent:nil];
	}

	if( Definition->SupportsTransparency )
	{
		SetOpacity( Definition->Opacity );
	}
	else
	{
		SetOpacity( 1.0f );
	}
}

FMacWindow::FMacWindow()
	: WindowHandle(NULL)
	, bIsVisible(false)
	, WindowMode( EWindowMode::Windowed )
{
	PreFullscreenWindowRect.left = PreFullscreenWindowRect.top = PreFullscreenWindowRect.right = PreFullscreenWindowRect.bottom = 0;
}

FSlateCocoaWindow* FMacWindow::GetWindowHandle() const
{
	return WindowHandle;
}

void FMacWindow::ReshapeWindow( int32 X, int32 Y, int32 Width, int32 Height )
{
	SCOPED_AUTORELEASE_POOL;
	check(WindowHandle);
	
	const TSharedRef<FGenericApplicationMessageHandler> MessageHandler = OwningApplication->MessageHandler;
	MessageHandler->BeginReshapingWindow( SharedThis( this ) );

	const int32 InvertedY = FPlatformMisc::ConvertSlateYPositionToCocoa(Y) - Height + 1;
	if (Definition->HasOSWindowBorder)
	{
		[WindowHandle setFrame: [WindowHandle frameRectForContentRect: NSMakeRect(X, InvertedY, FMath::Max(Width, 1), FMath::Max(Height, 1))] display:NO];
	}
	else
	{
		[WindowHandle setFrame: NSMakeRect(X, InvertedY, FMath::Max(Width, 1), FMath::Max(Height, 1)) display:NO];
	}
	
	// Force resize back to screen size in fullscreen - not ideally pretty but means we don't
	// have to subvert the OS X or UE fullscreen handling events elsewhere.
	if(WindowMode != EWindowMode::Windowed)
	{
		[WindowHandle setFrame: [WindowHandle screen].frame display:NO];
	}
	
	WindowHandle->bZoomed = [WindowHandle isZoomed];
	
	MessageHandler->FinishedReshapingWindow( SharedThis( this ) );
}

bool FMacWindow::GetFullScreenInfo( int32& X, int32& Y, int32& Width, int32& Height ) const
{
	SCOPED_AUTORELEASE_POOL;

	const NSRect Frame = [WindowHandle screen].frame;
	X = Frame.origin.x;
	Y = Frame.origin.y;
	Width = Frame.size.width;
	Height = Frame.size.height;
	return true;
}

void FMacWindow::MoveWindowTo( int32 X, int32 Y )
{
	SCOPED_AUTORELEASE_POOL;

	const int32 InvertedY = FPlatformMisc::ConvertSlateYPositionToCocoa(Y) - [WindowHandle frame].size.height + 1;
	[WindowHandle setFrameOrigin: NSMakePoint(X, InvertedY)];
}

void FMacWindow::BringToFront( bool bForce )
{
	if (bIsVisible)
	{
		SCOPED_AUTORELEASE_POOL;
		[WindowHandle orderFrontEvenIfChildAndMakeMain:false andKey:false];
	}
}

void FMacWindow::Destroy()
{
	if( WindowHandle )
	{
		// Retain the window as this FMacWindow may be destructed within OnWindowDestroyed.
		// The destructor will release the window once, so we need to still have a reference.
		// This makes it possible to correctly handle any NSEvent's sent to the window during destruction
		// as the WindowHandle is still a valid NSWindow. Unlike HWNDs accessing a destructed NSWindow* is fatal.
		FSlateCocoaWindow* Window = [WindowHandle retain];
		if( MacApplication->OnWindowDestroyed( Window ) )
		{
			// This FMacWindow may have been destructed by now & so the WindowHandle will probably be invalid memory.
			
			// Remove from parent
			FSlateCocoaWindow* ParentWindow = (FSlateCocoaWindow*)[Window getParent];
			if( ParentWindow )
			{
				[ParentWindow removeCachedChild: Window];
				[ParentWindow removeChildWindow: Window];
			}
			
			// Then change the focus to something useful, either the previous in the stack
			TSharedPtr<FMacWindow> KeyWindow = MacApplication->GetKeyWindow();
			if( KeyWindow.IsValid() && [Window isKeyWindow] && KeyWindow->GetOSWindowHandle() && ![(FSlateCocoaWindow*)KeyWindow->GetOSWindowHandle() isMiniaturized] )
			{
				// Activate specified previous window if still present, provided it isn't minimized
				KeyWindow->SetWindowFocus();
			}
			// Or, activate parent to make sure that some other window won't steal focus, provided it isn't minimized. Important for menu stack.
			else if( ParentWindow && [Window isKeyWindow] && ![ParentWindow isMiniaturized] )
			{
				// If miniaturized dispatch the message to prevent PrivateDrawWindows from exploding because the
				// window we are currently closing hasn't been removed from the window list.
				[ParentWindow orderFrontEvenIfChildAndMakeMain:true andKey:true];
			}
			
			// Close the window, but don't destruct it...
			[Window performClose:nil];
			
			// Since event handling of the change in focus may try to communicate with this window,
			// dispatch a call to release it which should be handled afterward.
			// This will ensure proper event handling & destruction order.
			dispatch_async(dispatch_get_main_queue(), ^{
				[Window release];
			});
		}
	}
}

void FMacWindow::Minimize()
{
	SCOPED_AUTORELEASE_POOL;
	[WindowHandle miniaturize:nil];
}

void FMacWindow::Maximize()
{
	SCOPED_AUTORELEASE_POOL;
	if( ![WindowHandle isZoomed] )
	{
		WindowHandle->bZoomed = true;
		[WindowHandle zoom:nil];
	}
}

void FMacWindow::Restore()
{
	SCOPED_AUTORELEASE_POOL;
	if( [WindowHandle isZoomed] )
	{
		WindowHandle->bZoomed = !WindowHandle->bZoomed;
		[WindowHandle zoom:nil];
	}
	else
	{
		WindowHandle->bZoomed = false;
		[WindowHandle deminiaturize:nil];
	}
}

void FMacWindow::Show()
{
	SCOPED_AUTORELEASE_POOL;
	if (!bIsVisible)
	{
		bool bMakeMainAndKey = ([WindowHandle canBecomeKeyWindow] && Definition->ActivateWhenFirstShown);
		[WindowHandle orderFrontEvenIfChildAndMakeMain:bMakeMainAndKey andKey:bMakeMainAndKey];
		
		bIsVisible = [WindowHandle isVisible];
		static bool bCannotRecurse = false;
		if(bCannotRecurse)
		{
			bCannotRecurse = true;
			// For the movie code we don't pump the event loop - so this function must do so before it returns
			// or the window will never become main/key!
			bool isMainAndKey = false;
			do
			{
				OwningApplication->PumpMessages(0.f);
				OwningApplication->ProcessDeferredEvents(0.f);
				bIsVisible = [WindowHandle isVisible];
				isMainAndKey = [WindowHandle isKeyWindow];
			} while(!bIsVisible && isMainAndKey != bMakeMainAndKey);
			bCannotRecurse = false;
		}
	}
}

void FMacWindow::Hide()
{
	if (bIsVisible)
	{
		SCOPED_AUTORELEASE_POOL;
		bIsVisible = false;
		[WindowHandle orderOut:nil];
	}
}

void FMacWindow::SetWindowMode( EWindowMode::Type NewWindowMode )
{
	SCOPED_AUTORELEASE_POOL;

	// In OS X fullscreen and windowed fullscreen are the same
	bool bMakeFullscreen = NewWindowMode != EWindowMode::Windowed;
	bool bIsFullscreen = WindowMode != EWindowMode::Windowed;

	if( bIsFullscreen != bMakeFullscreen )
	{
		bool WindowIsFullScreen = !bMakeFullscreen;
		[WindowHandle toggleFullScreen:nil];
		// Ensure that the window has transitioned BEFORE leaving this function
		// this prevents problems with failure to correctly update mouse locks
		// and OpenGL contexts due to bad event ordering.
		do
		{
			OwningApplication->PumpMessages(0.f);
			OwningApplication->ProcessDeferredEvents(0.f);
			WindowIsFullScreen = [WindowHandle windowMode] != EWindowMode::Windowed;
		} while(WindowIsFullScreen != bMakeFullscreen);
		WindowMode = NewWindowMode;
	}
}

bool FMacWindow::IsMaximized() const
{
	return WindowHandle->bZoomed;
}

bool FMacWindow::IsVisible() const
{
	return bIsVisible;
}

bool FMacWindow::GetRestoredDimensions(int32& X, int32& Y, int32& Width, int32& Height)
{
	SCOPED_AUTORELEASE_POOL;

	NSRect Frame = [WindowHandle frame];

	X = Frame.origin.x;
	Y = FPlatformMisc::ConvertSlateYPositionToCocoa(Frame.origin.y) - Frame.size.height + 1;

	Width = Frame.size.width;
	Height = Frame.size.height;

	return true;
}

void FMacWindow::SetWindowFocus()
{
	SCOPED_AUTORELEASE_POOL;
	[WindowHandle orderFrontEvenIfChildAndMakeMain:true andKey:true];
}

void FMacWindow::SetOpacity( const float InOpacity )
{
	SCOPED_AUTORELEASE_POOL;
	[WindowHandle setAlphaValue:InOpacity];
}

void FMacWindow::Enable( bool bEnable )
{
	SCOPED_AUTORELEASE_POOL;
	[WindowHandle setIgnoresMouseEvents: !bEnable];
}

bool FMacWindow::IsPointInWindow( int32 X, int32 Y ) const
{
	SCOPED_AUTORELEASE_POOL;
	bool PointInWindow = false;
	if(![WindowHandle isMiniaturized])
	{
		NSRect WindowFrame = [WindowHandle frame];
		NSRect VisibleFrame = WindowFrame;
		VisibleFrame.origin = NSMakePoint(0, 0);
		// Only the editor needs to handle the space-per-display logic introduced in Mavericks.
	#if WITH_EDITOR
		// Only fetch the spans-displays once - it requires a log-out to change.
		// Note that we have no way to tell if the current setting is actually in effect,
		// so this won't work if the user schedules a change but doesn't logout to confirm it.
		static bool bSpansDisplays = false;
		static bool bSettingFetched = false;
		if(!bSettingFetched)
		{
			bSettingFetched = true;
			NSUserDefaults* SpacesDefaults = [[[NSUserDefaults alloc] init] autorelease];
			[SpacesDefaults addSuiteNamed:@"com.apple.spaces"];
			bSpansDisplays = [SpacesDefaults boolForKey:@"spans-displays"] == YES;
		}
		if(!bSpansDisplays)
		{
			NSRect ScreenFrame = [[WindowHandle screen] frame];
			NSRect Intersection = NSIntersectionRect(ScreenFrame, WindowFrame);
			VisibleFrame.size = Intersection.size;
			VisibleFrame.origin.x = WindowFrame.origin.x < ScreenFrame.origin.x ? (ScreenFrame.origin.x - WindowFrame.origin.x) : 0;
			VisibleFrame.origin.y = (WindowFrame.origin.y + WindowFrame.size.height) > (ScreenFrame.origin.y + ScreenFrame.size.height) ? (WindowFrame.size.height - Intersection.size.height) : 0;
		}
	#endif
		PointInWindow = (NSPointInRect(NSMakePoint(X, Y), VisibleFrame) == YES);
	}
	return PointInWindow;
}

int32 FMacWindow::GetWindowBorderSize() const
{
	return 0;
}

bool FMacWindow::IsForegroundWindow() const
{
	SCOPED_AUTORELEASE_POOL;
	return [WindowHandle isMainWindow];
}

void FMacWindow::SetText(const TCHAR* const Text)
{
	SCOPED_AUTORELEASE_POOL;
	CFStringRef CFName = FPlatformString::TCHARToCFString( Text );
	[WindowHandle setTitle: (NSString*)CFName];
	if(IsRegularWindow())
	{
		[NSApp changeWindowsItem: WindowHandle title: (NSString*)CFName filename: NO];
	}
	CFRelease( CFName );
}

bool FMacWindow::IsRegularWindow() const
{
	return Definition->IsRegularWindow;
}

void FMacWindow::AdjustCachedSize( FVector2D& Size ) const
{
	// No adjustmnet needed
}

void FMacWindow::OnDisplayReconfiguration(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags)
{
	if(WindowHandle)
	{
		if(Flags & kCGDisplayBeginConfigurationFlag)
		{
			[WindowHandle setMovable: YES];
			[WindowHandle setMovableByWindowBackground: NO];
			
			[WindowHandle setDisplayReconfiguring: true];
		}
		else if(Flags & kCGDisplayDesktopShapeChangedFlag)
		{
			[WindowHandle setDisplayReconfiguring: false];
		}
	}
}
