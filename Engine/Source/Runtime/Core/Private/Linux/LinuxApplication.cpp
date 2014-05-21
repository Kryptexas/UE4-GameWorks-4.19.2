
#include "CorePrivate.h"
#include "LinuxApplication.h"
#include "LinuxWindow.h"
#include "LinuxCursor.h"
#include "GenericApplicationMessageHandler.h"
#if STEAM_CONTROLLER_SUPPORT
	#include "SteamControllerInterface.h"
#endif // STEAM_CONTROLLER_SUPPORT

#include "ds_extensions.h"

#if WITH_EDITOR
#include "ModuleManager.h"
#endif

//
// GameController thresholds
//
#define GAMECONTROLLER_LEFT_THUMB_DEADZONE  7849
#define GAMECONTROLLER_RIGHT_THUMB_DEADZONE 8689
#define GAMECONTROLLER_TRIGGER_THRESHOLD    30

float ShortToNormalFloat(short AxisVal)
{
	// normalize [-32768..32767] -> [-1..1]
	const float Norm = (AxisVal <= 0 ? 32768.f : 32767.f);
	return float(AxisVal) / Norm;
}

FLinuxApplication* LinuxApplication = NULL;

FLinuxApplication* FLinuxApplication::CreateLinuxApplication()
{
	LinuxApplication = new FLinuxApplication();
	
	//	init the sdl here
	if	( SDL_WasInit( 0 ) == 0 )
	{
		SDL_Init( SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER );
	}
	else
	{
		Uint32 subsystem_init = SDL_WasInit( SDL_INIT_EVERYTHING );

		if	( !(subsystem_init & SDL_INIT_EVENTS) )
		{
			SDL_InitSubSystem( SDL_INIT_EVENTS );
		}
		if	( !(subsystem_init & SDL_INIT_JOYSTICK) )
		{
			SDL_InitSubSystem( SDL_INIT_JOYSTICK );
		}
		if	( !(subsystem_init & SDL_INIT_GAMECONTROLLER) )
		{
			SDL_InitSubSystem( SDL_INIT_GAMECONTROLLER );
		}
	}

	SDLControllerState *controllerState = LinuxApplication->ControllerStates;
	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
	    if (SDL_IsGameController(i)) {
		    controllerState->controller = SDL_GameControllerOpen(i);
			if ( controllerState++->controller == NULL ) {
				UE_LOG(LogLoad, Warning, TEXT("Could not open gamecontroller %i: %s\n"), i, SDL_GetError() );
			}
		}
	}
	return LinuxApplication;
}


FLinuxApplication::FLinuxApplication() : GenericApplication( MakeShareable( new FLinuxCursor() ) )
#if STEAM_CONTROLLER_SUPPORT
	, SteamInput( SteamControllerInterface::Create(MessageHandler) )
#endif // STEAM_CONTROLLER_SUPPORT
{
	bUsingHighPrecisionMouseInput = false;
	bAllowedToDeferMessageProcessing = false;
	MouseCaptureWindow = NULL;
	ControllerStates = new SDLControllerState[SDL_NumJoysticks()];
	memset( ControllerStates, 0, sizeof(SDLControllerState) * SDL_NumJoysticks() );
}

FLinuxApplication::~FLinuxApplication()
{
	delete [] ControllerStates;
}

void FLinuxApplication::DestroyApplication()
{
	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		if(ControllerStates[i].controller != NULL)
		{
			SDL_GameControllerClose(ControllerStates[i].controller);
		}
	}
}


TSharedRef< FGenericWindow > FLinuxApplication::MakeWindow() 
{ 
	return FLinuxWindow::Make(); 
}

void FLinuxApplication::InitializeWindow(	const TSharedRef< FGenericWindow >& InWindow,
											const TSharedRef< FGenericWindowDefinition >& InDefinition,
											const TSharedPtr< FGenericWindow >& InParent,
											const bool bShowImmediately )
{
	const TSharedRef< FLinuxWindow > Window = StaticCastSharedRef< FLinuxWindow >( InWindow );
	const TSharedPtr< FLinuxWindow > ParentWindow = StaticCastSharedPtr< FLinuxWindow >( InParent );

	Windows.Add( Window );
	Window->Initialize( this, InDefinition, ParentWindow, bShowImmediately );
}

void FLinuxApplication::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	GenericApplication::SetMessageHandler(InMessageHandler);
#if STEAM_CONTROLLER_SUPPORT
	SteamInput->SetMessageHandler(InMessageHandler);
#endif // STEAM_CONTROLLER_SUPPORT
}

static TSharedPtr< FLinuxWindow > FindWindowBySDLWindow( const TArray< TSharedRef< FLinuxWindow > >& WindowsToSearch, SDL_HWindow const WindowHandle )
{
	for (int32 WindowIndex=0; WindowIndex < WindowsToSearch.Num(); ++WindowIndex)
	{
		TSharedRef< FLinuxWindow > Window = WindowsToSearch[ WindowIndex ];
		if ( Window->GetHWnd() == WindowHandle )
		{
			return Window;
		}
	}

	return TSharedPtr< FLinuxWindow >( NULL );
}

void FLinuxApplication::PumpMessages( const float TimeDelta )
{
	FPlatformMisc::PumpMessages( true );
}


void FLinuxApplication::AddPendingEvent( SDL_Event SDLEvent )
{
	if( GPumpingMessagesOutsideOfMainLoop && bAllowedToDeferMessageProcessing )
	{
		PendingEvents.Add( SDLEvent );
	}
	else
	{
		// When not deferring messages, process them immediately
		ProcessDeferredMessage( SDLEvent );
	}
}

bool FLinuxApplication::GeneratesKeyCharMessage(const SDL_KeyboardEvent & KeyDownEvent)
{
	bool bCmdKeyPressed = (KeyDownEvent.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) != 0;
	const SDL_Keycode Sym = KeyDownEvent.keysym.sym;

	// filter out command keys, non-ASCI and arrow keycodes that don't generate WM_CHAR under Windows (TODO: find a table?)
	return !bCmdKeyPressed && Sym < 128 &&
		(Sym != SDLK_DOWN && Sym != SDLK_LEFT && Sym != SDLK_RIGHT && Sym != SDLK_UP);
}

void FLinuxApplication::ProcessDeferredMessage( SDL_Event Event )
{
	// This function can be reentered when entering a modal tick loop.
	// We need to make a copy of the events that need to be processed or we may end up processing the same messages twice 
	SDL_HWindow NativeWindow = NULL;

	TSharedPtr< FLinuxWindow > CurrentEventWindow = FindEventWindow( &Event );
	if( !CurrentEventWindow.IsValid() && LastEventWindow.IsValid() )
	{
		CurrentEventWindow = LastEventWindow;
	}
	if( CurrentEventWindow.IsValid() )
	{
		LastEventWindow = CurrentEventWindow;
		NativeWindow = CurrentEventWindow->GetHWnd();
	}
	if( !NativeWindow )
	{
		return;
	}
	switch( Event.type )
	{
	case SDL_KEYDOWN:
		{
			SDL_KeyboardEvent KeyEvent = Event.key;
			const SDL_Keycode KeyCode = KeyEvent.keysym.scancode;
			const bool bIsRepeated = KeyEvent.repeat != 0;
				
			// First KeyDown, then KeyChar. This is important, as in-game console ignores first character otherwise
			MessageHandler->OnKeyDown(KeyCode, KeyEvent.keysym.sym, bIsRepeated);

			if (GeneratesKeyCharMessage(KeyEvent))
			{
				const TCHAR Character = ConvertChar(KeyEvent.keysym);
				MessageHandler->OnKeyChar(Character, bIsRepeated);
			}
		}
		break;
	case SDL_KEYUP:
		{
			SDL_KeyboardEvent keyEvent = Event.key;
			const SDL_Keycode KeyCode = keyEvent.keysym.scancode;
			const TCHAR Character = ConvertChar( keyEvent.keysym );
			const bool IsRepeat = keyEvent.repeat != 0;

			MessageHandler->OnKeyUp( KeyCode, keyEvent.keysym.sym, IsRepeat );
		}
		break;
	case SDL_MOUSEMOTION:
		{
			SDL_MouseMotionEvent motionEvent = Event.motion;
			FLinuxCursor *LinuxCursor = (FLinuxCursor*)Cursor.Get();

			if(LinuxCursor->IsHidden())
			{
				int width, height;
				SDL_GetWindowSize( NativeWindow, &width, &height );
				if( motionEvent.x != (width / 2) || motionEvent.y != (height / 2) )
				{
					int xOffset, yOffset;
					SDL_GetWindowPosition( NativeWindow, &xOffset, &yOffset );
					LinuxCursor->SetPosition( width / 2 + xOffset, height / 2 + yOffset );
				}
				else
				{
					break;
				}
			}
			else
			{
				FVector2D CurrentPosition = LinuxCursor->GetPosition();
				if( LinuxCursor->UpdateCursorClipping( CurrentPosition ) )
				{
					LinuxCursor->SetPosition( CurrentPosition.X, CurrentPosition.Y );
				}
				if( !CurrentEventWindow->GetDefinition().HasOSWindowBorder )
				{
					if ( CurrentEventWindow->IsRegularWindow() )
					{
						int xOffset, yOffset;
						SDL_GetWindowPosition( NativeWindow, &xOffset, &yOffset );
						MessageHandler->GetWindowZoneForPoint( CurrentEventWindow.ToSharedRef(), CurrentPosition.X - xOffset, CurrentPosition.Y - yOffset );
						MessageHandler->OnCursorSet();
					}
				}
			}

			if(bUsingHighPrecisionMouseInput)
			{
				MessageHandler->OnRawMouseMove( motionEvent.xrel, motionEvent.yrel );
			}
			else
			{
				MessageHandler->OnMouseMove();
			}

		}
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		{
			SDL_MouseButtonEvent buttonEvent = Event.button;
				
			EMouseButtons::Type button;
			switch(buttonEvent.button)
			{
			case SDL_BUTTON_LEFT:
				button = EMouseButtons::Left;
				break;
			case SDL_BUTTON_MIDDLE:
				button = EMouseButtons::Middle;
				break;
			case SDL_BUTTON_RIGHT:
				button = EMouseButtons::Right;
				break;
			case SDL_BUTTON_X1:
				button = EMouseButtons::Thumb01;
				break;
			case SDL_BUTTON_X2:
				button = EMouseButtons::Thumb02;
				break;
			default:
				button = EMouseButtons::Invalid;
				break;
			}
			if(buttonEvent.type == SDL_MOUSEBUTTONUP)
			{
				MessageHandler->OnMouseUp( button );
			}
			// SDL 2.0.2+
			//else if(buttonEvent.clicks > 1)
			//{
			//	MessageHandler->OnMouseDoubleClick( NativeWindow, button );
			//}
			else
			{
				MessageHandler->OnMouseDown( CurrentEventWindow, button );
			}
		}
		break;
	case SDL_MOUSEWHEEL:
		{
			SDL_MouseWheelEvent wheelEvent = Event.wheel;
			const float SpinFactor = 1 / 120.0f;
			const short WheelDelta = wheelEvent.y;

			MessageHandler->OnMouseWheel( static_cast<float>( WheelDelta ) * SpinFactor );
		}
		break;
	case SDL_CONTROLLERAXISMOTION:
		{
			SDL_ControllerAxisEvent caxisEvent = Event.caxis;
			EControllerButtons::Type analog;
			float value = ShortToNormalFloat(caxisEvent.value);

			switch (caxisEvent.axis)
			{
			case SDL_CONTROLLER_AXIS_LEFTX:
				analog = EControllerButtons::LeftAnalogX;
				if(caxisEvent.value > GAMECONTROLLER_LEFT_THUMB_DEADZONE)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[0])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::LeftStickRight, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[0] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[0])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::LeftStickRight, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[0] = false;
				}
				if(caxisEvent.value < -GAMECONTROLLER_LEFT_THUMB_DEADZONE)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[1])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::LeftStickLeft, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[1] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[1])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::LeftStickLeft, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[1] = false;
				}
				break;
			case SDL_CONTROLLER_AXIS_LEFTY:
				analog = EControllerButtons::LeftAnalogY;
				value *= -1;
				if(caxisEvent.value > GAMECONTROLLER_LEFT_THUMB_DEADZONE)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[2])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::LeftStickDown, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[2] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[2])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::LeftStickDown, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[2] = false;
				}
				if(caxisEvent.value < -GAMECONTROLLER_LEFT_THUMB_DEADZONE)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[3])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::LeftStickUp, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[3] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[3])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::LeftStickUp, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[3] = false;
				}
				break;
			case SDL_CONTROLLER_AXIS_RIGHTX:
				analog = EControllerButtons::RightAnalogX;
				if(caxisEvent.value > GAMECONTROLLER_RIGHT_THUMB_DEADZONE)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[4])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::RightStickRight, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[4] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[4])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::RightStickRight, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[4] = false;
				}
				if(caxisEvent.value < -GAMECONTROLLER_RIGHT_THUMB_DEADZONE)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[5])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::RightStickLeft, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[5] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[5])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::RightStickLeft, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[5] = false;
				}
				break;
			case SDL_CONTROLLER_AXIS_RIGHTY:
				analog = EControllerButtons::RightAnalogY;
				value *= -1;
				if(caxisEvent.value > GAMECONTROLLER_RIGHT_THUMB_DEADZONE)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[6])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::RightStickDown, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[6] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[6])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::RightStickDown, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[6] = false;
				}
				if(caxisEvent.value < -GAMECONTROLLER_RIGHT_THUMB_DEADZONE)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[7])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::RightStickUp, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[7] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[7])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::RightStickUp, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[7] = false;
				}
				break;
			case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
				analog = EControllerButtons::LeftTriggerAnalog;
				if(caxisEvent.value > GAMECONTROLLER_TRIGGER_THRESHOLD)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[8])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::LeftTriggerThreshold, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[8] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[8])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::LeftTriggerThreshold, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[8] = false;
				}
				break;
			case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
				analog = EControllerButtons::LeftTriggerAnalog;
				if(caxisEvent.value > GAMECONTROLLER_TRIGGER_THRESHOLD)
				{
					if(!ControllerStates[caxisEvent.which].analogOverThreshold[9])
					{
						MessageHandler->OnControllerButtonPressed(EControllerButtons::RightTriggerThreshold, caxisEvent.which, false);
						ControllerStates[caxisEvent.which].analogOverThreshold[9] = true;
					}
				}
				else if(ControllerStates[caxisEvent.which].analogOverThreshold[9])
				{
					MessageHandler->OnControllerButtonReleased(EControllerButtons::RightTriggerThreshold, caxisEvent.which, false);
					ControllerStates[caxisEvent.which].analogOverThreshold[9] = false;
				}
				break;
			default:
				analog = EControllerButtons::Invalid;
				break;
			}

			MessageHandler->OnControllerAnalog(analog, caxisEvent.which, value);
		}
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		{
			SDL_ControllerButtonEvent cbuttonEvent = Event.cbutton;
			EControllerButtons::Type button;

			switch (cbuttonEvent.button)
			{
			case SDL_CONTROLLER_BUTTON_A:
				button = EControllerButtons::FaceButtonBottom;
				break;
			case SDL_CONTROLLER_BUTTON_B:
				button = EControllerButtons::FaceButtonRight;
				break;
			case SDL_CONTROLLER_BUTTON_X:
				button = EControllerButtons::FaceButtonLeft;
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				button = EControllerButtons::FaceButtonTop;
				break;
			case SDL_CONTROLLER_BUTTON_BACK:
				button = EControllerButtons::SpecialLeft;
				break;
			case SDL_CONTROLLER_BUTTON_START:
				button = EControllerButtons::SpecialRight;
				break;
			case SDL_CONTROLLER_BUTTON_LEFTSTICK:
				button = EControllerButtons::LeftStickDown;
				break;
			case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
				button = EControllerButtons::RightStickDown;
				break;
			case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
				button = EControllerButtons::LeftShoulder;
				break;
			case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				button = EControllerButtons::RightShoulder;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				button = EControllerButtons::DPadUp;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				button = EControllerButtons::DPadDown;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				button = EControllerButtons::DPadLeft;
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				button = EControllerButtons::DPadRight;
				break;
			default:
				button = EControllerButtons::Invalid;
				break;
			}

			if(cbuttonEvent.type == SDL_CONTROLLERBUTTONDOWN)
			{
				MessageHandler->OnControllerButtonPressed(button, cbuttonEvent.which, false);
			}
			else
			{
				MessageHandler->OnControllerButtonReleased(button, cbuttonEvent.which, false);
			}
		}
		break;
		
	case SDL_WINDOWEVENT:
		{
			SDL_WindowEvent windowEvent = Event.window;

			switch (windowEvent.event)
			{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_SIZE_CHANGED has been send.\n" );
					
						int NewWidth  = windowEvent.data1;
						int NewHeight = windowEvent.data2;

						MessageHandler->OnSizeChanged( CurrentEventWindow.ToSharedRef(), NewWidth, NewHeight, 
																		//	bWasMinimized
																			false
																			);
					}
					break;

				case SDL_WINDOWEVENT_RESIZED:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_RESIZED has been send.\n" );

						int NewWidth  = windowEvent.data1;
						int NewHeight = windowEvent.data2;

						MessageHandler->OnSizeChanged( CurrentEventWindow.ToSharedRef(), NewWidth, NewHeight, 
																		//	bWasMinimized
																			false
																			);

					//	MessageHandler->OnResizingWindow( CurrentEventWindow.ToSharedRef() );
					}

				case SDL_WINDOWEVENT_CLOSE:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_CLOSE has been send: %d %d.\n", windowEvent.data1, windowEvent.data2 );
						if(windowEvent.data1 == 0 && windowEvent.data2 == 0)
						{
							MessageHandler->OnWindowClose( CurrentEventWindow.ToSharedRef() );
						}
					}
					break;

				case SDL_WINDOWEVENT_SHOWN:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_SHOWN has been send.\n" );
					}
					break;

				case SDL_WINDOWEVENT_HIDDEN:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_HIDDEN has been send.\n" );
					}
					break;

				case SDL_WINDOWEVENT_EXPOSED:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_EXPOSED has been send.\n" );
					}
					break;

				case SDL_WINDOWEVENT_MOVED:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_MOVED has been send.\n" );
						MessageHandler->OnMovedWindow( CurrentEventWindow.ToSharedRef(), windowEvent.data1, windowEvent.data2 );
					}
					break;

				case SDL_WINDOWEVENT_MINIMIZED:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_MINIMIZED has been send.\n" );
					}
					break;

				case SDL_WINDOWEVENT_MAXIMIZED:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_MAXIMIZED has been send.\n" );
						MessageHandler->OnWindowAction( CurrentEventWindow.ToSharedRef(), EWindowAction::Maximize );
					}
					break;

				case SDL_WINDOWEVENT_RESTORED:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_RESTORED has been send.\n" );
						MessageHandler->OnWindowAction( CurrentEventWindow.ToSharedRef(), EWindowAction::Restore );
					}
					break;

				case SDL_WINDOWEVENT_ENTER:
					{
						if ( CurrentEventWindow.IsValid() )
						{
							MessageHandler->OnCursorSet();
							//MessageHandler->OnWindowActivationChanged( CurrentEventWindow.ToSharedRef(), EWindowActivation::ActivateByMouse );
						}
					}
					break;

				case SDL_WINDOWEVENT_LEAVE:
					{
						if( CurrentEventWindow.IsValid() && GetCapture() != NULL)
						{
							UpdateMouseCaptureWindow((SDL_HWindow)GetCapture());
						}
					}
					break;

				case SDL_WINDOWEVENT_FOCUS_GAINED:
					{
						if ( CurrentEventWindow.IsValid() )
						{
							MessageHandler->OnWindowActivationChanged( CurrentEventWindow.ToSharedRef(), EWindowActivation::Activate );
						}
					}
					break;

				case SDL_WINDOWEVENT_FOCUS_LOST:
					{
					//	printf( "Ariel - SDL_WINDOWEVENT_FOCUS_LOST has been send.\n" );
						if ( CurrentEventWindow.IsValid() )
						{
							MessageHandler->OnWindowActivationChanged( CurrentEventWindow.ToSharedRef(), EWindowActivation::Deactivate );
						}
					}
				break;
			}
		}
		break;
	}

}


void FLinuxApplication::ProcessDeferredEvents( const float TimeDelta )
{
	// This function can be reentered when entering a modal tick loop.
	// We need to make a copy of the events that need to be processed or we may end up processing the same messages twice 
	SDL_HWindow NativeWindow = NULL;

	TArray< SDL_Event > Events( PendingEvents );
	PendingEvents.Empty();

	for( int32 Index = 0; Index < Events.Num(); ++Index )
	{
		ProcessDeferredMessage( Events[Index] );
	}
}

void FLinuxApplication::PollGameDeviceState( const float TimeDelta )
{
#if STEAM_CONTROLLER_SUPPORT
	// Poll game device states and send new events
	SteamInput->SendControllerEvents();
#endif // STEAM_CONTROLLER_SUPPORT
}

TCHAR FLinuxApplication::ConvertChar( SDL_Keysym Keysym )
{
	if( Keysym.sym >= 128 )
	{
		return 0;
	}

    TCHAR Char = Keysym.sym;

    if (Keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
    {
        // Convert to uppercase (FIXME: what about CAPS?)
        if( Keysym.sym >= 97 && Keysym.sym <= 122)
        {
            return Keysym.sym - 32;
        }
        else if( Keysym.sym >= 91 && Keysym.sym <= 93)
        {
            return Keysym.sym + 32; // [ \ ] -> { | }
        }
        else
        {
            switch(Keysym.sym)
            {
                case '`': // ` -> ~
                    Char = TEXT('`');
                    break;

                case '-': // - -> _
                    Char = TEXT('_');
                    break;

                case '=': // - -> _
                    Char = TEXT('+');
                    break;

                case ',':
                    Char = TEXT('<');
                    break;

                case '.':
                    Char = TEXT('>');
                    break;

                case ';':
                    Char = TEXT(':');
                    break;

                case '\'':
                    Char = TEXT('\"');
                    break;

                case '/':
                    Char = TEXT('?');
                    break;

                case '0':
                    Char = TEXT(')');
                    break;

                case '9':
                    Char = TEXT('(');
                    break;

                case '8':
                    Char = TEXT('*');
                    break;

                case '7':
                    Char = TEXT('&');
                    break;

                case '6':
                    Char = TEXT('^');
                    break;

                case '5':
                    Char = TEXT('%');
                    break;

                case '4':
                    Char = TEXT('$');
                    break;

                case '3':
                    Char = TEXT('#');
                    break;

                case '2':
                    Char = TEXT('@');
                    break;

                case '1':
                    Char = TEXT('!');
                    break;

                default:
                    break;
            }
        }
    }

    return Char;
}

TSharedPtr< FLinuxWindow > FLinuxApplication::FindEventWindow( SDL_Event* Event )
{
	uint16 windowID;
	switch (Event->type)
	{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			windowID = Event->key.windowID;
			break;
		case SDL_MOUSEMOTION:
			windowID = Event->motion.windowID;
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			windowID = Event->button.windowID;
			break;
		case SDL_MOUSEWHEEL:
			windowID = Event->wheel.windowID;
			break;

		case SDL_WINDOWEVENT:
			windowID = Event->window.windowID;
			break;
		default:
			return TSharedPtr< FLinuxWindow >( NULL );
	}

	for (int32 WindowIndex=0; WindowIndex < Windows.Num(); ++WindowIndex)
	{
		TSharedRef< FLinuxWindow > Window = Windows[ WindowIndex ];
		if ( SDL_GetWindowID(Window->GetHWnd()) == windowID )
		{
			return Window;
		}
	}

	if(Windows.Num() > 0)
	{
		return TSharedPtr< FLinuxWindow >( Windows[0] );
	}

	return TSharedPtr< FLinuxWindow >( NULL );
}

FModifierKeysState FLinuxApplication::GetModifierKeys() const
{
	SDL_Keymod modifiers = SDL_GetModState();

	const bool bIsLeftShiftDown		= (modifiers & KMOD_LSHIFT) != 0;
	const bool bIsRightShiftDown	= (modifiers & KMOD_RSHIFT) != 0;
	const bool bIsLeftControlDown	= (modifiers & KMOD_LCTRL) != 0;
	const bool bIsRightControlDown	= (modifiers & KMOD_RCTRL) != 0;
	const bool bIsLeftAltDown		= (modifiers & KMOD_LALT) != 0;
	const bool bIsRightAltDown		= (modifiers & KMOD_RALT) != 0;

	return FModifierKeysState( bIsLeftShiftDown, bIsRightShiftDown, bIsLeftControlDown, bIsRightControlDown, bIsLeftAltDown, bIsRightAltDown );
}


void FLinuxApplication::SetCapture( const TSharedPtr< FGenericWindow >& InWindow )
{
	bIsMouseCaptureEnabled = InWindow.IsValid();
	UpdateMouseCaptureWindow( bIsMouseCaptureEnabled ? ((FLinuxWindow*)InWindow.Get())->GetHWnd() : NULL );
}


void* FLinuxApplication::GetCapture( void ) const
{
	return ( bIsMouseCaptureEnabled && MouseCaptureWindow ) ? MouseCaptureWindow : NULL;
}

void FLinuxApplication::UpdateMouseCaptureWindow( SDL_HWindow TargetWindow )
{
	const bool bEnable = bIsMouseCaptureEnabled || bIsMouseCursorLocked;
	FLinuxCursor *LinuxCursor = static_cast< FLinuxCursor* >(Cursor.Get());

	if( bEnable )
	{
		if( TargetWindow )
		{
			MouseCaptureWindow = TargetWindow;
		}
		if (MouseCaptureWindow && !LinuxCursor->IsHidden())
		{
			if (EDSExtSuccess != DSEXT_SetMouseGrab(TargetWindow, SDL_TRUE))
			{
				UE_LOG(LogHAL, Log, TEXT("Could not grab cursor for SDL window %p"), TargetWindow);
			}
		}
	}
	else
	{
		if( MouseCaptureWindow )
		{
			if (!LinuxCursor->IsHidden())
			{
				if (EDSExtSuccess != DSEXT_SetMouseGrab(TargetWindow, SDL_FALSE))
				{
					UE_LOG(LogHAL, Log, TEXT("Could not ungrab cursor for SDL window %p"), TargetWindow);
				}
			}
			MouseCaptureWindow = NULL;
		}
	}
}

void FLinuxApplication::SetHighPrecisionMouseMode( const bool Enable, const TSharedPtr< FGenericWindow >& InWindow )
{
	MessageHandler->OnCursorSet();
	bUsingHighPrecisionMouseInput = Enable;
}


FPlatformRect FLinuxApplication::GetWorkArea( const FPlatformRect& CurrentWindow ) const
{
	RECT WindowsWindowDim;
	WindowsWindowDim.left	= CurrentWindow.Left;
	WindowsWindowDim.top	= CurrentWindow.Top;
	WindowsWindowDim.right	= CurrentWindow.Right;
	WindowsWindowDim.bottom	= CurrentWindow.Bottom;

	//	asdd
#if 0
	// ... figure out the best monitor for that window.
	HMONITOR hBestMonitor = MonitorFromRect( &WindowsWindowDim, MONITOR_DEFAULTTONEAREST );

	// Get information about that monitor...
	MONITORINFO MonitorInfo;
	MonitorInfo.cbSize = sizeof(MonitorInfo);
	GetMonitorInfo( hBestMonitor, &MonitorInfo);

	// ... so that we can figure out the work area (are not covered by taskbar)
	MonitorInfo.rcWork;

	FPlatformRect WorkArea;
	WorkArea.Left = MonitorInfo.rcWork.left;
	WorkArea.Top = MonitorInfo.rcWork.top;
	WorkArea.Right = MonitorInfo.rcWork.right;
	WorkArea.Bottom = MonitorInfo.rcWork.bottom;
#endif

	FPlatformRect WorkArea;
	WorkArea.Left	= WindowsWindowDim.left;
	WorkArea.Top	= WindowsWindowDim.top;
	WorkArea.Right	= WindowsWindowDim.right;
	WorkArea.Bottom	= WindowsWindowDim.bottom;

	return WorkArea;
}

void FLinuxApplication::OnMouseCursorLock( bool bLockEnabled )
{
	bIsMouseCursorLocked = bLockEnabled;
	UpdateMouseCaptureWindow( NULL );
}


bool FLinuxApplication::TryCalculatePopupWindowPosition( const FPlatformRect& InAnchor, const FVector2D& InSize, const EPopUpOrientation::Type Orientation, /*OUT*/ FVector2D* const CalculatedPopUpPosition ) const
{
	return false;
}


void FLinuxApplication::GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const
{
	SDL_Rect bounds;
	SDL_GetDisplayBounds( 0, &bounds );

	// Get screen rect
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Left = bounds.x;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Top = bounds.y;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right = bounds.w;
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom = bounds.h;
	OutDisplayMetrics.VirtualDisplayRect = OutDisplayMetrics.PrimaryDisplayWorkAreaRect;

	// Total screen size of the primary monitor
	OutDisplayMetrics.PrimaryDisplayWidth = OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right - OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Left;
	OutDisplayMetrics.PrimaryDisplayHeight = OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Top;
}
