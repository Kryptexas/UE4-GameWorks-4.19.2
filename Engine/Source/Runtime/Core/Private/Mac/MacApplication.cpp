// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#include "MacApplication.h"
#include "MacWindow.h"
#include "MacCursor.h"
#include "GenericApplicationMessageHandler.h"
#include "HIDInputInterface.h"

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
}

FMacApplication::~FMacApplication()
{
	CGDisplayRemoveReconfigurationCallback(FMacApplication::OnDisplayReconfiguration, this);
	if (MouseCaptureWindow)
	{
		[MouseCaptureWindow close];
		MouseCaptureWindow = NULL;
	}

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

void FMacApplication::HandleExternallyChangedModifier(TSharedPtr< FMacWindow > CurrentEventWindow, NSUInteger NewModifierFlags, NSUInteger FlagsShift, NSUInteger UE4Shift, EMacModifierKeys TranslatedCode)
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
	
	// Mission Control (aka Expose, Spaces) can silently steal focus and modify the modifier flags,
	// without posting any events, leaving our input caching very confused as the key states won't match.
	// We should only need to capture the modifiers, because they must go down first, we shouldn't get messages for the
	// other keys.
	if( ( EventType != NSFlagsChanged || [Event keyCode] == 0 ) && CurrentModifierFlags != [Event modifierFlags] )
	{
		NSUInteger ModifierFlags = [Event modifierFlags];
		
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<4), 7, MMK_RightCommand);
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<3), 6, MMK_LeftCommand);
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<1), 0, MMK_LeftShift);
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<16), 8, MMK_CapsLock);
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<5), 4, MMK_LeftAlt);
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<0), 2, MMK_LeftControl);
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<2), 1, MMK_RightShift);
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<6), 5, MMK_RightAlt);
		HandleExternallyChangedModifier(CurrentEventWindow, ModifierFlags, (1<<13), 3, MMK_RightControl);
		
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
				const int32 X = FMath::Trunc(CurrentEventWindow->PositionX + [Event deltaX]);
				const int32 Y = FMath::Trunc(CurrentEventWindow->PositionY + [Event deltaY]);
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
			}
			break;
		}

		case NSEventTypeSwipe:
		{
			if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnTouchGesture( EGestureEvent::Swipe, FVector2D( [Event deltaX], [Event deltaY] ), 0 );
			}
			break;
		}

		case NSEventTypeRotate:
		{
			if( CurrentEventWindow.IsValid() )
			{
				MessageHandler->OnTouchGesture( EGestureEvent::Rotate, FVector2D( [Event rotation], [Event rotation] ), 0 );
			}
			break;
		}
    
        case NSEventTypeBeginGesture:
        {
            bUsingTrackpad = true;
            break;
        }
        case NSEventTypeEndGesture:
        {
            bUsingTrackpad = false;
            break;
        }
			
		case NSKeyDown:
		{
			NSString *Characters = [Event characters];
			if( [Characters length] && CurrentEventWindow.IsValid() )
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

		case NSFlagsChanged:
		{
			const uint32 KeyCode = [Event keyCode];
			const uint32 Flags = [Event modifierFlags];
			CurrentModifierFlags = Flags;
			uint32 TranslatedCode = 0;

			if( KeyCode >= 54 && KeyCode <= 62 )
			{
				bool Pressed = false;
				uint32 Shift = 0;

				switch( KeyCode )
				{
					case 54:	Pressed = ( ( Flags & (1<<4) ) != 0 );	Shift = 7;	TranslatedCode = MMK_RightCommand;	break; // right cmd
					case 55:	Pressed = ( ( Flags & (1<<3) ) != 0 );	Shift = 6;	TranslatedCode = MMK_LeftCommand;	break; // left cmd
					case 56:	Pressed = ( ( Flags & (1<<1) ) != 0 );	Shift = 0;	TranslatedCode = MMK_LeftShift;		break; // left shift
					case 57:	Pressed = ( ( Flags & (1<<16)) != 0 );	Shift = 8;	TranslatedCode = MMK_CapsLock;		break; // caps lock
					case 58:	Pressed = ( ( Flags & (1<<5) ) != 0 );	Shift = 4;	TranslatedCode = MMK_LeftAlt;		break; // left alt
					case 59:	Pressed = ( ( Flags & (1<<0) ) != 0 );	Shift = 2;	TranslatedCode = MMK_LeftControl;	break; // left ctrl
					case 60:	Pressed = ( ( Flags & (1<<2) ) != 0 );	Shift = 1;	TranslatedCode = MMK_RightShift;	break; // right shift
					case 61:	Pressed = ( ( Flags & (1<<6) ) != 0 );	Shift = 5;	TranslatedCode = MMK_RightAlt;		break; // right alt
					case 62:	Pressed = ( ( Flags & (1<<13) ) != 0 );	Shift = 3;	TranslatedCode = MMK_RightControl;	break; // right ctrl
					default:
						check(0);
						break;
				}

				if( Pressed )
				{
					ModifierKeysFlags |= 1 << Shift;
					if( CurrentEventWindow.IsValid() )
					{
						MessageHandler->OnKeyDown( TranslatedCode, 0, false );
					}
				}
				else
				{
					ModifierKeysFlags &= ~(1 << Shift);
					if( CurrentEventWindow.IsValid() )
					{
						MessageHandler->OnKeyUp( TranslatedCode, 0, false );
					}
				}
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
				if( [Window isMiniaturized] || ![Window isVisible] || ![Window isKindOfClass: [FSlateCocoaWindow class]] )
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
	HIDInput->Tick( TimeDelta );
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

	const int32 ScreenHeight = FMath::Trunc([Screen frame].size.height);
	const NSRect VisibleFrame = [Screen visibleFrame];

	FPlatformRect WorkArea;
	WorkArea.Left = VisibleFrame.origin.x;
	WorkArea.Top = ScreenHeight - VisibleFrame.size.height - VisibleFrame.origin.y;
	WorkArea.Right = WorkArea.Left + VisibleFrame.size.width;
	WorkArea.Bottom = WorkArea.Top + VisibleFrame.size.height;

	return WorkArea;
}

NSScreen* FMacApplication::FindScreenByPoint( int32 X, int32 Y ) const
{
	NSArray* AllScreens = [NSScreen screens];
	NSScreen* PrimaryScreen = (NSScreen*)[AllScreens objectAtIndex: 0];

	const float PrimaryScreenHeight = [PrimaryScreen frame].size.height;

	NSScreen* TargetScreen = PrimaryScreen;

	for( int32 Index = 1; Index < [AllScreens count]; Index++ )
	{
		NSScreen* Screen = (NSScreen*)[AllScreens objectAtIndex: Index];

		NSRect ScreenFrame = [Screen frame];
		ScreenFrame.origin.y = PrimaryScreenHeight - ScreenFrame.origin.y - ScreenFrame.size.height;

		if( FMath::Trunc( ScreenFrame.origin.x ) < X && FMath::Trunc( ScreenFrame.origin.x + ScreenFrame.size.width ) >= X
		   && FMath::Trunc( ScreenFrame.origin.y ) < Y && FMath::Trunc( ScreenFrame.origin.y + ScreenFrame.size.height ) >= Y )
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

	// Get the screen rect of the primary monitor, excluding taskbar etc.
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Left = VisibleFrame.origin.x;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Top = ScreenFrame.size.height - (VisibleFrame.origin.y + VisibleFrame.size.height);
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right = VisibleFrame.origin.x + VisibleFrame.size.width;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom = ScreenFrame.size.height - VisibleFrame.origin.y;

	// Virtual desktop area
	OutDisplayMetrics.VirtualDisplayRect.Left = 0;
	OutDisplayMetrics.VirtualDisplayRect.Top = 0;
	OutDisplayMetrics.VirtualDisplayRect.Right = 0;
	OutDisplayMetrics.VirtualDisplayRect.Bottom = 0;

	for( int32 Index = 0; Index < [AllScreens count]; Index++ )
	{
		NSScreen* Screen = (NSScreen*)[AllScreens objectAtIndex: Index];
		NSRect DisplayFrame = [Screen frame];
		if(DisplayFrame.origin.x != ScreenFrame.origin.x)
		{
			OutDisplayMetrics.VirtualDisplayRect.Right += DisplayFrame.size.width;
		}
		else
		{
			OutDisplayMetrics.VirtualDisplayRect.Right = FMath::Max(OutDisplayMetrics.VirtualDisplayRect.Right, (int)DisplayFrame.size.width);
		}
		if(DisplayFrame.origin.y != ScreenFrame.origin.y)
		{
			OutDisplayMetrics.VirtualDisplayRect.Bottom += DisplayFrame.size.height;
		}
		else
		{
			OutDisplayMetrics.VirtualDisplayRect.Bottom = FMath::Max(OutDisplayMetrics.VirtualDisplayRect.Bottom, (int)DisplayFrame.size.height);
		}
	}
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
			Width = FMath::Trunc([[Window screen] frame].size.width);
			Height = FMath::Trunc([[Window screen] frame].size.height);
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
	// Keys like F1-F12, enter or backspace do not need translation
	bool bNeedsTranslation = (CharCode < 0xF700 || CharCode > 0xF8FF) && CharCode != 0x7F;
	if( bNeedsTranslation )
	{
		// For non-numpad keys, the key code depends on the keyboard layout, so find out what was pressed by converting the key code to a Latin character
		TISInputSourceRef CurrentKeyboard = TISCopyCurrentKeyboardInputSource();
		if( CurrentKeyboard )
		{
			CFDataRef CurrentLayoutData = ( CFDataRef )TISGetInputSourceProperty( CurrentKeyboard, kTISPropertyUnicodeKeyLayoutData );
			CFRelease( CurrentKeyboard );

			if( CurrentLayoutData ) // @todo: figure out why it's NULL for Hiragana etc. and how to make Japanese characters input work.
			{
				const UCKeyboardLayout *KeyboardLayout = ( UCKeyboardLayout *)CFDataGetBytePtr( CurrentLayoutData );
				if( KeyboardLayout )
				{
					UniChar Buffer[256] = { 0 };
					UniCharCount BufferLength = 256;
					uint32 DeadKeyState = 0;

					// To ensure we get a latin characted, we pretend that command modifier key is pressed
					OSStatus Status = UCKeyTranslate( KeyboardLayout, KeyCode, kUCKeyActionDown, cmdKey >> 8, LMGetKbdType(), kUCKeyTranslateNoDeadKeysMask, &DeadKeyState, BufferLength, &BufferLength, Buffer );
					if( Status == noErr )
					{
						CharCode = Buffer[0];
					}
				}
			}
		}
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
bool FMacApplication::SupportsSourceAccess() const 
{
	return true;
}

void FMacApplication::GotoLineInSource(const FString& FileAndLineNumber)
{
	FString FullPath, LineNumberWithColumnString;
	if (FileAndLineNumber.Split(TEXT("|"), &FullPath, &LineNumberWithColumnString))
	{
		FString LineNumberString;
		FString ColumnNumberString;
		if ( !LineNumberWithColumnString.Split(TEXT(":"), &LineNumberString, &ColumnNumberString, ESearchCase::CaseSensitive, ESearchDir::FromEnd) )
		{
			// The column was not in the string
			LineNumberString = LineNumberWithColumnString;
			ColumnNumberString = TEXT("");
		}
		
		int32 LineNumber = FCString::Strtoi(*LineNumberString, NULL, 10);
		int32 ColumnNumber = FCString::Strtoi(*ColumnNumberString, NULL, 10);

		if ( FModuleManager::Get().IsSolutionFilePresent() )
		{
			FString ProjPath = FPaths::ConvertRelativePathToFull(FModuleManager::Get().GetSolutionFilepath());
			CFStringRef ProjPathString = FPlatformString::TCHARToCFString(*ProjPath);
			NSString* ProjectPath = [(NSString*)ProjPathString stringByDeletingLastPathComponent];
			[[NSWorkspace sharedWorkspace] openFile:ProjectPath withApplication:@"Xcode" andDeactivate:YES];
		}
		
		bool ExecutionSucceeded = false;
		
		NSAppleScript* AppleScript = nil;
		NSURL* PathURL = [[NSBundle mainBundle] URLForResource:@"OpenXcodeAtFileAndLine" withExtension:@"applescript"];
		
		NSDictionary* AppleScriptCreationError = nil;
		AppleScript = [[NSAppleScript alloc] initWithContentsOfURL:PathURL error:&AppleScriptCreationError];
		
		if (!AppleScriptCreationError)
		{
			int PID = [[NSProcessInfo processInfo] processIdentifier];
			NSAppleEventDescriptor* ThisApplication = [NSAppleEventDescriptor descriptorWithDescriptorType:typeKernelProcessID bytes:&PID length:sizeof(PID)];
			
			NSAppleEventDescriptor* ContainerEvent = [NSAppleEventDescriptor appleEventWithEventClass:'ascr' eventID:'psbr' targetDescriptor:ThisApplication returnID:kAutoGenerateReturnID transactionID:kAnyTransactionID];
			
			[ContainerEvent setParamDescriptor:[NSAppleEventDescriptor descriptorWithString:@"OpenXcodeAtFileAndLine"] forKeyword:'snam'];
			
			{
				NSAppleEventDescriptor* Arguments = [[NSAppleEventDescriptor alloc] initListDescriptor];

				CFStringRef FileString = FPlatformString::TCHARToCFString(*FullPath);
				NSString* Path = (NSString*)FileString;
				
				if([Path isAbsolutePath] == NO)
				{
					NSString* CurDir = [[NSFileManager defaultManager] currentDirectoryPath];
					NSString* ResolvedPath = [[NSString stringWithFormat:@"%@/%@", CurDir, Path] stringByResolvingSymlinksInPath];
					if([[NSFileManager defaultManager] fileExistsAtPath:ResolvedPath])
					{
						Path = ResolvedPath;
					}
					else // If it doesn't exist, supply only the filename, we'll use Open Quickly to try and find it from Xcode
					{
						Path = [Path lastPathComponent];
					}
				}
				
				[Arguments insertDescriptor:[NSAppleEventDescriptor descriptorWithString:Path] atIndex:([Arguments numberOfItems] + 1)];
				CFRelease(FileString);
				
				CFStringRef LineString = FPlatformString::TCHARToCFString(*LineNumberString);
				if(LineString)
				{
					[Arguments insertDescriptor:[NSAppleEventDescriptor descriptorWithString:(NSString*)LineString] atIndex:([Arguments numberOfItems] + 1)];
					CFRelease(LineString);
				}
				else
				{
					[Arguments insertDescriptor:[NSAppleEventDescriptor descriptorWithString:@"1"] atIndex:([Arguments numberOfItems] + 1)];
				}
				
				[ContainerEvent setParamDescriptor:Arguments forKeyword:keyDirectObject];
				[Arguments release];
			}
			
			NSDictionary* ExecutionError = nil;
			[AppleScript executeAppleEvent:ContainerEvent error:&ExecutionError];
			if(ExecutionError == nil)
			{
				ExecutionSucceeded = true;
			}
		}
		
		[AppleScript release];
		
		// Fallback to trivial implementation when something goes wrong (like not having permission for UI scripting)
		if(ExecutionSucceeded == false)
		{
			FPlatformProcess::LaunchFileInDefaultExternalApplication(*FullPath);
		}
	}
}
#endif
