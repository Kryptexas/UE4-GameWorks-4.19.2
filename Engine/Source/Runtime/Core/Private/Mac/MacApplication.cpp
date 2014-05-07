// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#include "MacApplication.h"
#include "MacWindow.h"
#include "MacCursor.h"
#include "GenericApplicationMessageHandler.h"
#include "HIDInputInterface.h"
#include "AnalyticsEventAttribute.h"
#include "IAnalyticsProvider.h"

#include "ModuleManager.h"

FMacApplication* MacApplication = NULL;

FMacApplication* FMacApplication::CreateMacApplication()
{
	MacApplication = new FMacApplication();
	return MacApplication;
}

void FMacApplication::OnDisplayReconfiguration(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void* UserInfo)
{
	FMacApplication* App = (FMacApplication*)UserInfo;
	for (int32 WindowIndex=0; WindowIndex < App->Windows.Num(); ++WindowIndex)
	{
		TSharedRef< FMacWindow > WindowRef = App->Windows[ WindowIndex ];
		WindowRef->OnDisplayReconfiguration(Display, Flags);
	}
}

FMacApplication::FMacApplication()
	: GenericApplication( MakeShareable( new FMacCursor() ) )
	, bUsingHighPrecisionMouseInput( false )
	, bUsingTrackpad( false )
	, HIDInput( HIDInputInterface::Create( MessageHandler ) )
	, DraggedWindow( NULL )
	, MouseCaptureWindow( NULL )
	, bIsMouseCaptureEnabled( false )
	, bIsMouseCursorLocked( false )
	, ModifierKeysFlags( 0 )
	, CurrentModifierFlags( 0 )
{
	CGDisplayRegisterReconfigurationCallback(FMacApplication::OnDisplayReconfiguration, this);
	
	TextInputMethodSystem = MakeShareable( new FMacTextInputMethodSystem );
	if(!TextInputMethodSystem->Initialize())
	{
		TextInputMethodSystem.Reset();
	}

#if WITH_EDITOR
	FMemory::MemZero(GestureUsage);
	LastGestureUsed = EGestureEvent::None;
#endif
}

FMacApplication::~FMacApplication()
{
	CGDisplayRemoveReconfigurationCallback(FMacApplication::OnDisplayReconfiguration, this);
	if (MouseCaptureWindow)
	{
		[MouseCaptureWindow close];
		MouseCaptureWindow = NULL;
	}
	
	TextInputMethodSystem->Terminate();

	MacApplication = NULL;
}

TSharedRef< FGenericWindow > FMacApplication::MakeWindow()
{
	return FMacWindow::Make();
}

void FMacApplication::InitializeWindow( const TSharedRef< FGenericWindow >& InWindow, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FGenericWindow >& InParent, const bool bShowImmediately )
{
	const TSharedRef< FMacWindow > Window = StaticCastSharedRef< FMacWindow >( InWindow );
	const TSharedPtr< FMacWindow > ParentWindow = StaticCastSharedPtr< FMacWindow >( InParent );

	Windows.Add( Window );
	Window->Initialize( this, InDefinition, ParentWindow, bShowImmediately );
}

void FMacApplication::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	GenericApplication::SetMessageHandler( InMessageHandler );
	HIDInput->SetMessageHandler( InMessageHandler );
}

static TSharedPtr< FMacWindow > FindWindowByNSWindow( const TArray< TSharedRef< FMacWindow > >& WindowsToSearch, FSlateCocoaWindow* const WindowHandle )
{
	for (int32 WindowIndex=0; WindowIndex < WindowsToSearch.Num(); ++WindowIndex)
	{
		TSharedRef< FMacWindow > Window = WindowsToSearch[ WindowIndex ];
		if ( Window->GetWindowHandle() == WindowHandle )
		{
			return Window;
		}
	}

	return TSharedPtr< FMacWindow >( NULL );
}

void FMacApplication::PumpMessages( const float TimeDelta )
{
	FPlatformMisc::PumpMessages( true );
}

void FMacApplication::AddPendingEvent( NSEvent* Event )
{
	SCOPED_AUTORELEASE_POOL;
	PendingEvents.Add( [Event retain] );
}

void FMacApplication::OnWindowDraggingFinished()
{
	if( DraggedWindow )
	{
		SCOPED_AUTORELEASE_POOL;
		[DraggedWindow reconnectChildWindows];
		DraggedWindow = NULL;
	}
}

bool FMacApplication::IsWindowMovable(FSlateCocoaWindow* Win, bool* OutMovableByBackground)
{
	if(OutMovableByBackground)
	{
		*OutMovableByBackground = false;
	}

	FSlateCocoaWindow* NativeWindow = (FSlateCocoaWindow*)Win;
	TSharedPtr< FMacWindow > CurrentEventWindow = FindWindowByNSWindow( Windows, NativeWindow );
	if( CurrentEventWindow.IsValid() )
	{
		const FVector2D CursorPos = static_cast<FMacCursor*>( Cursor.Get() )->GetPosition();
		const int32 LocalMouseX = CursorPos.X - CurrentEventWindow->PositionX;
		const int32 LocalMouseY = CursorPos.Y - CurrentEventWindow->PositionY;
		
		const EWindowZone::Type Zone = MessageHandler->GetWindowZoneForPoint( CurrentEventWindow.ToSharedRef(), LocalMouseX, LocalMouseY );
		bool IsMouseOverTitleBar = Zone == EWindowZone::TitleBar;
		switch(Zone)
		{
			case EWindowZone::NotInWindow:
			case EWindowZone::TopLeftBorder:
			case EWindowZone::TopBorder:
			case EWindowZone::TopRightBorder:
			case EWindowZone::LeftBorder:
			case EWindowZone::RightBorder:
			case EWindowZone::BottomLeftBorder:
			case EWindowZone::BottomBorder:
			case EWindowZone::BottomRightBorder:
				return true;
			case EWindowZone::TitleBar:
				if(OutMovableByBackground)
				{
					*OutMovableByBackground = true;
				}
				return true;
			case EWindowZone::ClientArea:
			case EWindowZone::MinimizeButton:
			case EWindowZone::MaximizeButton:
			case EWindowZone::CloseButton:
			case EWindowZone::SysMenu:
			default:
				return false;
		}
	}
	return true;
}

void FMacApplication::ProcessDeferredEvents( const float TimeDelta )
{
	SCOPED_AUTORELEASE_POOL;

	// Processing some events may result in adding more events and calling this function recursively. To avoid problems,
	// we make a local copy of pending events and empty the global one.
	TArray< NSEvent* > Events( PendingEvents );
	PendingEvents.Empty();
	for( int32 Index = 0; Index < Events.Num(); Index++ )
	{
		ProcessEvent( Events[Index] );
		[Events[Index] release];
	}
}

void FMacApplication::HandleModifierChange(TSharedPtr< FMacWindow > CurrentEventWindow, NSUInteger NewModifierFlags, NSUInteger FlagsShift, NSUInteger UE4Shift, EMacModifierKeys TranslatedCode)
{
	bool CurrentPressed = (CurrentModifierFlags & FlagsShift) != 0;
	bool NewPressed = (NewModifierFlags & FlagsShift) != 0;
	if(CurrentPressed != NewPressed)
	{
		if( NewPressed )
		{
			ModifierKeysFlags |= 1 << UE4Shift;
			if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnKeyDown( TranslatedCode, 0, false );
			}
		}
		else
		{
			ModifierKeysFlags &= ~(1 << UE4Shift);
			if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnKeyUp( TranslatedCode, 0, false );
			}
		}
	}
}

void FMacApplication::ProcessEvent( NSEvent* Event )
{
	SCOPED_AUTORELEASE_POOL;

	const NSEventType EventType = [Event type];

	FSlateCocoaWindow* NativeWindow = FindEventWindow( Event );
	TSharedPtr< FMacWindow > CurrentEventWindow = FindWindowByNSWindow( Windows, NativeWindow );
	if( !CurrentEventWindow.IsValid() && LastEventWindow.IsValid() )
	{
		CurrentEventWindow = LastEventWindow;
		NativeWindow = CurrentEventWindow->GetWindowHandle();
	}

	if( CurrentEventWindow.IsValid() )
	{
		LastEventWindow = CurrentEventWindow;
	}

	if( !NativeWindow )
	{
		return;
	}

	if( CurrentModifierFlags != [Event modifierFlags] )
	{
		NSUInteger ModifierFlags = [Event modifierFlags];
		
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<4), 7, MMK_RightCommand);
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<3), 6, MMK_LeftCommand);
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<1), 0, MMK_LeftShift);
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<16), 8, MMK_CapsLock);
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<5), 4, MMK_LeftAlt);
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<0), 2, MMK_LeftControl);
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<2), 1, MMK_RightShift);
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<6), 5, MMK_RightAlt);
		HandleModifierChange(CurrentEventWindow, ModifierFlags, (1<<13), 3, MMK_RightControl);
		
		CurrentModifierFlags = ModifierFlags;
	}

	switch( EventType )
	{
		case NSMouseMoved:
		case NSLeftMouseDragged:
		case NSRightMouseDragged:
		case NSOtherMouseDragged:
		{
			if( CurrentEventWindow.IsValid() && CurrentEventWindow->IsRegularWindow() )
			{
				bool IsMouseOverTitleBar = false;
				bool IsMovable = IsWindowMovable(NativeWindow, &IsMouseOverTitleBar);
				[NativeWindow setMovable: IsMovable];
				[NativeWindow setMovableByWindowBackground: IsMouseOverTitleBar];
			}
			
			// Cocoa does not update NSWindow's frame until user stops dragging the window, so while window is being dragged, we calculate
			// its position based on mouse move delta
			if( DraggedWindow )
			{
				const int32 X = FMath::TruncToInt(CurrentEventWindow->PositionX + [Event deltaX]);
				const int32 Y = FMath::TruncToInt(CurrentEventWindow->PositionY + [Event deltaY]);
				if( CurrentEventWindow.IsValid() )
				{
					MessageHandler->OnMovedWindow( CurrentEventWindow.ToSharedRef(), X, Y );
					CurrentEventWindow->PositionX = X;
					CurrentEventWindow->PositionY = Y;
				}
			}

			if( CurrentEventWindow.IsValid() )
			{
				FMacCursor* MacCursor = (FMacCursor*)Cursor.Get();
				
				// Under OS X we disassociate the cursor and mouse position during hi-precision mouse input.
				// The game snaps the mouse cursor back to the starting point when this is disabled, which
				// accumulates mouse delta that we want to ignore.
				const FVector2D AccumDelta = static_cast<FMacCursor*>( Cursor.Get() )->GetMouseWarpDelta(true);

				if( bUsingHighPrecisionMouseInput )
				{
					// Find the screen the cursor is currently on.
					NSEnumerator *screenEnumerator = [[NSScreen screens] objectEnumerator];
					NSScreen *screen;
					while ((screen = [screenEnumerator nextObject]) && !NSMouseInRect(NSMakePoint(HighPrecisionMousePos.X, HighPrecisionMousePos.Y), screen.frame, NO))
						;
					
					// Clamp to no more than the reported delta - a single event of no mouse movement won't be noticed
					// but going in the wrong direction will.
					const FVector2D FullDelta([Event deltaX], [Event deltaY]);
					const FVector2D WarpDelta(FMath::Abs(AccumDelta.X)<FMath::Abs(FullDelta.X) ? AccumDelta.X : FullDelta.X, FMath::Abs(AccumDelta.Y)<FMath::Abs(FullDelta.Y) ? AccumDelta.Y : FullDelta.Y);
					
					FVector2D Delta = (FullDelta - WarpDelta) / 2.f;
					
					HighPrecisionMousePos = static_cast<FMacCursor*>( Cursor.Get() )->GetPosition() + Delta;
					MacCursor->UpdateCursorClipping( HighPrecisionMousePos );
					
					// Clamp to the current screen and avoid the menu bar and dock to prevent popups and other
					// assorted potential for mouse abuse.
					NSRect VisibleFrame = [screen visibleFrame];
					NSRect FullFrame = [screen frame];
					VisibleFrame.origin.y = (FullFrame.origin.y+FullFrame.size.height) - (VisibleFrame.origin.y + VisibleFrame.size.height);
					
					HighPrecisionMousePos.X = FMath::Clamp(HighPrecisionMousePos.X, (float)VisibleFrame.origin.x, (float)(VisibleFrame.origin.x + VisibleFrame.size.width)-1.f);
					HighPrecisionMousePos.Y = FMath::Clamp(HighPrecisionMousePos.Y, (float)VisibleFrame.origin.y, (float)(VisibleFrame.origin.y + VisibleFrame.size.height)-1.f);
					
					MacCursor->WarpCursor( HighPrecisionMousePos.X, HighPrecisionMousePos.Y );
					MessageHandler->OnRawMouseMove( Delta.X, Delta.Y );
				}
				else
				{
					// Call this here or else the cursor gets stuck in place after returning from high precision mode
					CGAssociateMouseAndMouseCursorPosition( true );
					FVector2D CurrentPosition = MacCursor->GetPosition();
					if( MacCursor->UpdateCursorClipping( CurrentPosition ) )
					{
						MacCursor->SetPosition( CurrentPosition.X, CurrentPosition.Y );
					}
					MessageHandler->OnMouseMove();
				}

				if( !DraggedWindow && !GetCapture() )
				{
					MessageHandler->OnCursorSet();
				}
			}
			break;
		}

		case NSLeftMouseDown:
		case NSRightMouseDown:
		case NSOtherMouseDown:
		{
			EMouseButtons::Type Button = [Event type] == NSLeftMouseDown ? EMouseButtons::Left : EMouseButtons::Right;
			if ([Event type] == NSOtherMouseDown)
			{
				switch ([Event buttonNumber])
				{
					case 2:
						Button = EMouseButtons::Middle;
						break;

					case 3:
						Button = EMouseButtons::Thumb01;
						break;

					case 4:
						Button = EMouseButtons::Thumb02;
						break;
				}
			}

			if( CurrentEventWindow.IsValid() )
			{
				if (Button == LastPressedMouseButton && ([Event clickCount] % 2) == 0)
				{
					MessageHandler->OnMouseDoubleClick( CurrentEventWindow, Button );
				}
				else
				{
					MessageHandler->OnMouseDown( CurrentEventWindow, Button );
				}

				if( !DraggedWindow && !GetCapture() )
				{
					MessageHandler->OnCursorSet();
				}
			}

			LastPressedMouseButton = Button;

			break;
		}

		case NSLeftMouseUp:
		case NSRightMouseUp:
		case NSOtherMouseUp:
		{
			EMouseButtons::Type Button = [Event type] == NSLeftMouseUp ? EMouseButtons::Left : EMouseButtons::Right;
			if ([Event type] == NSOtherMouseUp)
			{
				switch ([Event buttonNumber])
				{
					case 2:
						Button = EMouseButtons::Middle;
						break;

					case 3:
						Button = EMouseButtons::Thumb01;
						break;

					case 4:
						Button = EMouseButtons::Thumb02;
						break;
				}
			}

			if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnMouseUp( Button );

				if( !DraggedWindow && !GetCapture() )
				{
					MessageHandler->OnCursorSet();
				}
			}
			break;
		}

		case NSScrollWheel:
		{
			if( CurrentEventWindow.IsValid() )
			{
				const float DeltaX = ([Event modifierFlags] & NSShiftKeyMask) ? [Event deltaY] : [Event deltaX];
				const float DeltaY = ([Event modifierFlags] & NSShiftKeyMask) ? [Event deltaX] : [Event deltaY];
		
				NSEventPhase Phase = [Event phase];
				
				if( Phase == NSEventPhaseNone || Phase == NSEventPhaseEnded || Phase == NSEventPhaseCancelled )
				{
					bUsingTrackpad = false;
				}
				else
				{
					bUsingTrackpad = true;
				}
				
				if ([Event momentumPhase] != NSEventPhaseNone || [Event phase] != NSEventPhaseNone)
				{
					bool bInverted = [Event isDirectionInvertedFromDevice];
					
					FVector2D ScrollDelta( [Event scrollingDeltaX], [Event scrollingDeltaY] );
					
					
					// This is actually a scroll gesture from trackpad
					MessageHandler->OnTouchGesture( EGestureEvent::Scroll, bInverted ? -ScrollDelta : ScrollDelta, DeltaY );
					RecordUsage( EGestureEvent::Scroll );
				}
				else
				{
					MessageHandler->OnMouseWheel( DeltaY );
				}

				if( !DraggedWindow && !GetCapture() )
				{
					MessageHandler->OnCursorSet();
				}
			}
			break;
		}

		case NSEventTypeMagnify:
		{
			if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnTouchGesture( EGestureEvent::Magnify, FVector2D( [Event magnification], [Event magnification] ), 0 );
				RecordUsage( EGestureEvent::Magnify );
			}
			break;
		}

		case NSEventTypeSwipe:
		{
			if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnTouchGesture( EGestureEvent::Swipe, FVector2D( [Event deltaX], [Event deltaY] ), 0 );
				RecordUsage( EGestureEvent::Swipe );
			}
			break;
		}

		case NSEventTypeRotate:
		{
			if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnTouchGesture( EGestureEvent::Rotate, FVector2D( [Event rotation], [Event rotation] ), 0 );
				RecordUsage( EGestureEvent::Rotate );
			}
			break;
		}
	
		case NSEventTypeBeginGesture:
		{
			bUsingTrackpad = true;
            if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnBeginGesture();
            }
			break;
		}
        
		case NSEventTypeEndGesture:
		{
            if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnEndGesture();
            }
			bUsingTrackpad = false;
#if WITH_EDITOR
			LastGestureUsed = EGestureEvent::None;
#endif
			break;
		}
			
		case NSKeyDown:
		{
			NSString *Characters = [Event characters];
			if( [Characters length] && CurrentEventWindow.IsValid() )
			{
				if(!CurrentEventWindow->OnIMKKeyDown(Event))
				{
					const bool IsRepeat = [Event isARepeat];
					const TCHAR Character = ConvertChar( [Characters characterAtIndex:0] );
					const TCHAR CharCode = [[Event charactersIgnoringModifiers] characterAtIndex:0];
					const uint32 KeyCode = [Event keyCode];
					const bool IsPrintable = IsPrintableKey( Character );
					
					MessageHandler->OnKeyDown( KeyCode, TranslateCharCode( CharCode, KeyCode ), IsRepeat );
					
					// First KeyDown, then KeyChar. This is important, as in-game console ignores first character otherwise
					
					bool bCmdKeyPressed = [Event modifierFlags] & 0x18;
					if ( !bCmdKeyPressed && IsPrintable )
					{
						MessageHandler->OnKeyChar( Character, IsRepeat );
					}
				}
			}
			break;
		}

		case NSKeyUp:
		{
			NSString *Characters = [Event characters];
			if( [Characters length] && CurrentEventWindow.IsValid() )
			{
				const bool IsRepeat = [Event isARepeat];
				const TCHAR Character = ConvertChar( [Characters characterAtIndex:0] );
				const TCHAR CharCode = [[Event charactersIgnoringModifiers] characterAtIndex:0];
				const uint32 KeyCode = [Event keyCode];
				const bool IsPrintable = IsPrintableKey( Character );

				MessageHandler->OnKeyUp( KeyCode, TranslateCharCode( CharCode, KeyCode ), IsRepeat );
			}
			break;
		}
	}
}

FSlateCocoaWindow* FMacApplication::FindEventWindow( NSEvent* Event )
{
	SCOPED_AUTORELEASE_POOL;

	bool IsMouseEvent = false;
	switch ([Event type])
	{
		case NSMouseMoved:
		case NSLeftMouseDragged:
		case NSRightMouseDragged:
		case NSOtherMouseDragged:
		case NSLeftMouseDown:
		case NSRightMouseDown:
		case NSOtherMouseDown:
		case NSLeftMouseUp:
		case NSRightMouseUp:
		case NSOtherMouseUp:
		case NSScrollWheel:
			IsMouseEvent = true;
			break;
	}

	FSlateCocoaWindow* EventWindow = (FSlateCocoaWindow*)[Event window];

	if( IsMouseEvent )
	{
		if( DraggedWindow )
		{
			EventWindow = DraggedWindow;
		}
		else if( MouseCaptureWindow && MouseCaptureWindow == [Event window] )
		{
			EventWindow = [MouseCaptureWindow targetWindow];
		}
		else
		{
			const NSPoint CursorPos = [NSEvent mouseLocation];

			// Only the editor needs to handle the space-per-display logic introduced in Mavericks.
#if WITH_EDITOR
			NSScreen* MouseScreen = nil;
			// Only fetch the spans-displays once - it requires a log-out to change.
			static bool bScreensHaveSeparateSpaces = false;
			static bool bSettingFetched = false;
			if(!bSettingFetched)
			{
				bSettingFetched = true;
				bScreensHaveSeparateSpaces = [NSScreen screensHaveSeparateSpaces];
			}
			if(bScreensHaveSeparateSpaces)
			{
				// New default mode which uses a separate Space per display
				// Find the screen the cursor is currently on so we can ignore invisible window regions.
				NSEnumerator* ScreenEnumerator = [[NSScreen screens] objectEnumerator];
				while ((MouseScreen = [ScreenEnumerator nextObject]) && !NSMouseInRect(CursorPos, MouseScreen.frame, NO))
					;
			}
#endif
			
			NSArray* AllWindows = [NSApp orderedWindows];
			for( int32 Index = 0; Index < [AllWindows count]; Index++ )
			{
				NSWindow* Window = (NSWindow*)[AllWindows objectAtIndex: Index];
				if( [Window isMiniaturized] || ![Window isVisible] || ![Window isOnActiveSpace] || ![Window isKindOfClass: [FSlateCocoaWindow class]] )
				{
					continue;
				}

				if( [Window canBecomeKeyWindow] == false )
				{
					NSWindow* ParentWindow = [Window parentWindow];
					while( ParentWindow )
					{
						if( [ParentWindow canBecomeKeyWindow] )
						{
							Window = ParentWindow;
							break;
						}
						ParentWindow = [Window parentWindow];
					}
				}
				
				NSRect VisibleFrame = [Window frame];
#if WITH_EDITOR
				if(MouseScreen != nil)
				{
					VisibleFrame = NSIntersectionRect([MouseScreen frame], VisibleFrame);
				}
#endif

				if( NSPointInRect( CursorPos, VisibleFrame ) )
				{
					EventWindow = (FSlateCocoaWindow*)Window;
					break;
				}
			}
		}
	}

	return EventWindow;
}

void FMacApplication::PollGameDeviceState( const float TimeDelta )
{
	// Poll game device state and send new events
	HIDInput->SendControllerEvents();
}

void FMacApplication::SetCapture( const TSharedPtr< FGenericWindow >& InWindow )
{
	bIsMouseCaptureEnabled = InWindow.IsValid();
	UpdateMouseCaptureWindow( bIsMouseCaptureEnabled ? ((FMacWindow*)InWindow.Get())->GetWindowHandle() : NULL );
}

void* FMacApplication::GetCapture( void ) const
{
	return ( bIsMouseCaptureEnabled && MouseCaptureWindow ) ? [MouseCaptureWindow targetWindow] : NULL;
}

void FMacApplication::UseMouseCaptureWindow( bool bUseMouseCaptureWindow )
{
	bIsMouseCaptureEnabled = bUseMouseCaptureWindow;
	if(bUseMouseCaptureWindow)
	{
		// Bring the mouse capture window to front
		if(MouseCaptureWindow)
		{
			[MouseCaptureWindow orderFront: nil];
		}
	}
	else
	{
		if(MouseCaptureWindow)
		{
			[MouseCaptureWindow orderOut: nil];
		}
	}
}

void FMacApplication::UpdateMouseCaptureWindow( FSlateCocoaWindow* TargetWindow )
{
	SCOPED_AUTORELEASE_POOL;

	// To prevent mouse events to get passed to the Dock or top menu bar, we create a transparent, full screen window above everything.
	// This assures that only the editor (and with that, its active window) gets the mouse events while mouse capture is active.

	const bool bEnable = bIsMouseCaptureEnabled || bIsMouseCursorLocked;

	if( bEnable )
	{
		if( !TargetWindow )
		{
			if( [MouseCaptureWindow targetWindow] )
			{
				TargetWindow = [MouseCaptureWindow targetWindow];
			}
			else if( [[NSApp keyWindow] isKindOfClass: [FSlateCocoaWindow class]] )
			{
				TargetWindow = (FSlateCocoaWindow*)[NSApp keyWindow];
			}
		}

		if( !MouseCaptureWindow )
		{
			MouseCaptureWindow = [[FMouseCaptureWindow alloc] initWithTargetWindow: TargetWindow];
			check(MouseCaptureWindow);
		}
		else
		{
			[MouseCaptureWindow setTargetWindow: TargetWindow];
		}

		// Bring the mouse capture window to front
		[MouseCaptureWindow orderFront: nil];
	}
	else
	{
		// Hide the mouse capture window
		if( MouseCaptureWindow )
		{
			[MouseCaptureWindow orderOut: nil];
			[MouseCaptureWindow setTargetWindow: NULL];
		}
	}
}

void FMacApplication::SetHighPrecisionMouseMode( const bool Enable, const TSharedPtr< FGenericWindow >& InWindow )
{
	bUsingHighPrecisionMouseInput = Enable;
	HighPrecisionMousePos = static_cast<FMacCursor*>( Cursor.Get() )->GetPosition();
	
	CGAssociateMouseAndMouseCursorPosition( !Enable );
}

FModifierKeysState FMacApplication::GetModifierKeys() const
{
	uint32 CurrentFlags = ModifierKeysFlags;

	const bool bIsLeftShiftDown		= ( CurrentFlags & ( 1 << 0 ) ) != 0;
	const bool bIsRightShiftDown	= ( CurrentFlags & ( 1 << 1 ) ) != 0;
	const bool bIsLeftControlDown	= ( CurrentFlags & ( 1 << 6 ) ) != 0 || ( CurrentFlags & ( 1 << 2 ) ) != 0; // Mac pretends the Command key is Ctrl
	const bool bIsRightControlDown	= ( CurrentFlags & ( 1 << 7 ) ) != 0 || ( CurrentFlags & ( 1 << 3 ) ) != 0; // Mac pretends the Command key is Ctrl
	const bool bIsLeftAltDown		= ( CurrentFlags & ( 1 << 4 ) ) != 0;
	const bool bIsRightAltDown		= ( CurrentFlags & ( 1 << 5 ) ) != 0;

	return FModifierKeysState( bIsLeftShiftDown, bIsRightShiftDown, bIsLeftControlDown, bIsRightControlDown, bIsLeftAltDown, bIsRightAltDown );
}

FPlatformRect FMacApplication::GetWorkArea( const FPlatformRect& CurrentWindow ) const
{
	SCOPED_AUTORELEASE_POOL;

	NSScreen* Screen = FindScreenByPoint( CurrentWindow.Left, CurrentWindow.Top );

	const int32 ScreenHeight = FMath::TruncToInt([Screen frame].size.height);
	const NSRect VisibleFrame = [Screen visibleFrame];
	
	NSArray* AllScreens = [NSScreen screens];
	NSScreen* PrimaryScreen = (NSScreen*)[AllScreens objectAtIndex: 0];
	NSRect PrimaryFrame = [PrimaryScreen frame];

	FPlatformRect WorkArea;
	WorkArea.Left = VisibleFrame.origin.x;
	WorkArea.Top = (PrimaryFrame.origin.y + PrimaryFrame.size.height) - (VisibleFrame.origin.y + VisibleFrame.size.height);
	WorkArea.Right = WorkArea.Left + VisibleFrame.size.width;
	WorkArea.Bottom = WorkArea.Top + VisibleFrame.size.height;

	return WorkArea;
}

NSScreen* FMacApplication::FindScreenByPoint( int32 X, int32 Y ) const
{
	NSPoint Point = {0};
	Point.x = X;
	Point.y = FPlatformMisc::ConvertSlateYPositionToCocoa(Y);
	
	NSArray* AllScreens = [NSScreen screens];
	NSScreen* TargetScreen = [AllScreens objectAtIndex: 0];
	for(NSScreen* Screen in AllScreens)
	{
		if(Screen && NSPointInRect(Point, [Screen frame]))
		{
			TargetScreen = Screen;
			break;
		}
	}

	return TargetScreen;
}

void FMacApplication::GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const
{
	SCOPED_AUTORELEASE_POOL;

	NSArray* AllScreens = [NSScreen screens];
	NSScreen* PrimaryScreen = (NSScreen*)[AllScreens objectAtIndex: 0];

	NSRect ScreenFrame = [PrimaryScreen frame];
	NSRect VisibleFrame = [PrimaryScreen visibleFrame];

	// Total screen size of the primary monitor
	OutDisplayMetrics.PrimaryDisplayWidth = ScreenFrame.size.width;
	OutDisplayMetrics.PrimaryDisplayHeight = ScreenFrame.size.height;

	// Virtual desktop area
	NSRect WholeWorkspace = {0};
	for (NSScreen* Screen in AllScreens)
	{
		if (Screen)
		{
			WholeWorkspace = NSUnionRect(WholeWorkspace, [Screen frame]);
		}
	}
	OutDisplayMetrics.VirtualDisplayRect.Left = WholeWorkspace.origin.x;
	OutDisplayMetrics.VirtualDisplayRect.Top = FMath::Min((ScreenFrame.size.height - (WholeWorkspace.origin.y + WholeWorkspace.size.height)), 0.0);
	OutDisplayMetrics.VirtualDisplayRect.Right = WholeWorkspace.origin.x + WholeWorkspace.size.width;
	OutDisplayMetrics.VirtualDisplayRect.Bottom = WholeWorkspace.size.height + OutDisplayMetrics.VirtualDisplayRect.Top;
	
	// Get the screen rect of the primary monitor, excluding taskbar etc.
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Left = VisibleFrame.origin.x;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Top = ScreenFrame.size.height - (VisibleFrame.origin.y + VisibleFrame.size.height);
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right = VisibleFrame.origin.x + VisibleFrame.size.width;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom = OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Top + VisibleFrame.size.height;
}

void FMacApplication::OnDragEnter( FSlateCocoaWindow* Window, void *InPasteboard )
{
	SCOPED_AUTORELEASE_POOL;

	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( !EventWindow.IsValid() )
	{
		return;
	}

	// Decipher the pasteboard data

	NSPasteboard *Pasteboard = (NSPasteboard*)InPasteboard;

	const bool bHaveText = [[Pasteboard types] containsObject:NSPasteboardTypeString];
	const bool bHaveFiles = [[Pasteboard types] containsObject:NSFilenamesPboardType];

	if (bHaveFiles)
	{
		TArray<FString> FileList;

		NSArray *Files = [Pasteboard propertyListForType:NSFilenamesPboardType];
		for (int32 Index = 0; Index < [Files count]; Index++)
		{
			NSString* FilePath = [Files objectAtIndex: Index];
			const FString ListElement = FString([FilePath fileSystemRepresentation]);
			FileList.Add(ListElement);
		}

		MessageHandler->OnDragEnterFiles( EventWindow.ToSharedRef(), FileList );
	}
	else if (bHaveText)
	{
		NSString *Text = [Pasteboard propertyListForType:NSPasteboardTypeString];
		TCHAR* TextData = (TCHAR*)FMemory::Malloc(([Text length] + 1) * sizeof(TCHAR));
		FPlatformString::CFStringToTCHAR((CFStringRef)Text, TextData);

		MessageHandler->OnDragEnterText( EventWindow.ToSharedRef(), FString(TextData) );

		FMemory::Free(TextData);
	}
}

void FMacApplication::OnDragOver( FSlateCocoaWindow* Window )
{
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		MessageHandler->OnDragOver( EventWindow.ToSharedRef() );
	}
}

void FMacApplication::OnDragOut( FSlateCocoaWindow* Window )
{
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		MessageHandler->OnDragLeave( EventWindow.ToSharedRef() );
	}
}

void FMacApplication::OnDragDrop( FSlateCocoaWindow* Window )
{
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		MessageHandler->OnDragDrop( EventWindow.ToSharedRef() );
	}
}

void FMacApplication::OnWindowDidBecomeKey( FSlateCocoaWindow* Window )
{
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		MessageHandler->OnWindowActivationChanged( EventWindow.ToSharedRef(), EWindowActivation::Activate );
		KeyWindows.Add( EventWindow.ToSharedRef() );
	}
}

void FMacApplication::OnWindowDidResignKey( FSlateCocoaWindow* Window )
{
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		MessageHandler->OnWindowActivationChanged( EventWindow.ToSharedRef(), EWindowActivation::Deactivate );
	}
}

void FMacApplication::OnWindowWillMove( FSlateCocoaWindow* Window )
{
	SCOPED_AUTORELEASE_POOL;

	DraggedWindow = Window;
	[DraggedWindow disconnectChildWindows];
}

void FMacApplication::OnWindowDidMove( FSlateCocoaWindow* Window )
{
	SCOPED_AUTORELEASE_POOL;

	NSRect WindowFrame = [Window frame];
	NSRect OpenGLFrame = [Window openGLFrame];
	
	const int32 X = (int32)WindowFrame.origin.x;
	const int32 Y = FPlatformMisc::ConvertSlateYPositionToCocoa( (int32)WindowFrame.origin.y ) - OpenGLFrame.size.height + 1;

	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		MessageHandler->OnMovedWindow( EventWindow.ToSharedRef(), X, Y );
		EventWindow->PositionX = X;
		EventWindow->PositionY = Y;
	}
}

void FMacApplication::OnWindowDidResize( FSlateCocoaWindow* Window )
{
	SCOPED_AUTORELEASE_POOL;

	OnWindowDidMove( Window );
	
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		// default is no override
		uint32 Width = [Window openGLFrame].size.width;
		uint32 Height = [Window openGLFrame].size.height;
		
		if([Window windowMode] != EWindowMode::Windowed)
		{
			// Grab current monitor data for sizing
			Width = FMath::TruncToInt([[Window screen] frame].size.width);
			Height = FMath::TruncToInt([[Window screen] frame].size.height);
		}
		
		MessageHandler->OnSizeChanged( EventWindow.ToSharedRef(), Width, Height );
	}
}

void FMacApplication::OnWindowRedrawContents( FSlateCocoaWindow* Window )
{
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		SCOPED_AUTORELEASE_POOL;
		MessageHandler->OnSizeChanged( EventWindow.ToSharedRef(), [Window openGLFrame].size.width, [Window openGLFrame].size.height );
	}
}

void FMacApplication::OnWindowDidClose( FSlateCocoaWindow* Window )
{
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		SCOPED_AUTORELEASE_POOL;
		MessageHandler->OnWindowActivationChanged( EventWindow.ToSharedRef(), EWindowActivation::Deactivate );
		Windows.Remove( EventWindow.ToSharedRef() );
		KeyWindows.Remove( EventWindow.ToSharedRef() );
		MessageHandler->OnWindowClose( EventWindow.ToSharedRef() );
	}
}

bool FMacApplication::OnWindowDestroyed( FSlateCocoaWindow* Window )
{
	TSharedPtr< FMacWindow > EventWindow = FindWindowByNSWindow( Windows, Window );
	if( EventWindow.IsValid() )
	{
		MessageHandler->OnWindowActivationChanged( EventWindow.ToSharedRef(), EWindowActivation::Deactivate );
		Windows.Remove( EventWindow.ToSharedRef() );
		KeyWindows.Remove( EventWindow.ToSharedRef() );
		return true;
	}
	return false;
}

void FMacApplication::OnMouseCursorLock( bool bLockEnabled )
{
	bIsMouseCursorLocked = bLockEnabled;
	UpdateMouseCaptureWindow( NULL );
}

bool FMacApplication::IsPrintableKey(uint32 Character)
{
	switch (Character)
	{
		case NSPauseFunctionKey:		// EKeys::Pause
		case 0x1b:						// EKeys::Escape
		case NSPageUpFunctionKey:		// EKeys::PageUp
		case NSPageDownFunctionKey:		// EKeys::PageDown
		case NSEndFunctionKey:			// EKeys::End
		case NSHomeFunctionKey:			// EKeys::Home
		case NSLeftArrowFunctionKey:	// EKeys::Left
		case NSUpArrowFunctionKey:		// EKeys::Up
		case NSRightArrowFunctionKey:	// EKeys::Right
		case NSDownArrowFunctionKey:	// EKeys::Down
		case NSInsertFunctionKey:		// EKeys::Insert
		case NSDeleteFunctionKey:		// EKeys::Delete
		case NSF1FunctionKey:			// EKeys::F1
		case NSF2FunctionKey:			// EKeys::F2
		case NSF3FunctionKey:			// EKeys::F3
		case NSF4FunctionKey:			// EKeys::F4
		case NSF5FunctionKey:			// EKeys::F5
		case NSF6FunctionKey:			// EKeys::F6
		case NSF7FunctionKey:			// EKeys::F7
		case NSF8FunctionKey:			// EKeys::F8
		case NSF9FunctionKey:			// EKeys::F9
		case NSF10FunctionKey:			// EKeys::F10
		case NSF11FunctionKey:			// EKeys::F11
		case NSF12FunctionKey:			// EKeys::F12
			return false;

		default:
			return true;
	}
}

TCHAR FMacApplication::ConvertChar(TCHAR Character)
{
	switch (Character)
	{
		case NSDeleteCharacter:
			return '\b';
		default:
			return Character;
	}
}

TCHAR FMacApplication::TranslateCharCode(TCHAR CharCode, uint32 KeyCode)
{
	// Keys like F1-F12 or Enter do not need translation
	bool bNeedsTranslation = CharCode < NSOpenStepUnicodeReservedBase || CharCode > 0xF8FF;
	if( bNeedsTranslation )
	{
		// For non-numpad keys, the key code depends on the keyboard layout, so find out what was pressed by converting the key code to a Latin character
		TISInputSourceRef CurrentKeyboard = TISCopyCurrentKeyboardLayoutInputSource();
		if( CurrentKeyboard )
		{
			CFDataRef CurrentLayoutData = ( CFDataRef )TISGetInputSourceProperty( CurrentKeyboard, kTISPropertyUnicodeKeyLayoutData );
			CFRelease( CurrentKeyboard );

			if( CurrentLayoutData )
			{
				const UCKeyboardLayout *KeyboardLayout = ( UCKeyboardLayout *)CFDataGetBytePtr( CurrentLayoutData );
				if( KeyboardLayout )
				{
					UniChar Buffer[256] = { 0 };
					UniCharCount BufferLength = 256;
					uint32 DeadKeyState = 0;

					// To ensure we get a latin character, we pretend that command modifier key is pressed
					OSStatus Status = UCKeyTranslate( KeyboardLayout, KeyCode, kUCKeyActionDown, cmdKey >> 8, LMGetKbdType(), kUCKeyTranslateNoDeadKeysMask, &DeadKeyState, BufferLength, &BufferLength, Buffer );
					if( Status == noErr )
					{
						CharCode = Buffer[0];
					}
				}
			}
		}
	}
	// Private use range should not be returned
	else
	{
		CharCode = 0;
	}

	return CharCode;
}

TSharedPtr<FMacWindow> FMacApplication::GetKeyWindow()
{
	TSharedPtr<FMacWindow> KeyWindow;
	if(KeyWindows.Num() > 0)
	{
		KeyWindow = KeyWindows.Top();
	}
	return KeyWindow;
}

#if WITH_EDITOR

void FMacApplication::RecordUsage(EGestureEvent::Type Gesture)
{
	if ( LastGestureUsed != Gesture )
	{
		LastGestureUsed = Gesture;
		GestureUsage[Gesture] += 1;
	}
}

void FMacApplication::SendAnalytics(IAnalyticsProvider* Provider)
{
	checkAtCompileTime(EGestureEvent::Count == 5, "If the number of gestures changes you need to add more entries below!");

	TArray<FAnalyticsEventAttribute> GestureAttributes;
	GestureAttributes.Add(FAnalyticsEventAttribute(FString("Scroll"),	GestureUsage[EGestureEvent::Scroll]));
	GestureAttributes.Add(FAnalyticsEventAttribute(FString("Magnify"),	GestureUsage[EGestureEvent::Magnify]));
	GestureAttributes.Add(FAnalyticsEventAttribute(FString("Swipe"),	GestureUsage[EGestureEvent::Swipe]));
	GestureAttributes.Add(FAnalyticsEventAttribute(FString("Rotate"),	GestureUsage[EGestureEvent::Rotate]));

    Provider->RecordEvent(FString("Mac.Gesture.Usage"), GestureAttributes);

	FMemory::MemZero(GestureUsage);
	LastGestureUsed = EGestureEvent::None;
}

#endif
