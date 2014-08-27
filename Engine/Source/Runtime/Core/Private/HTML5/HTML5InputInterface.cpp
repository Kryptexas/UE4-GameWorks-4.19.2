// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "HTML5Cursor.h"
#include "HTML5InputInterface.h"
#include <SDL/SDL_events.h>

DECLARE_LOG_CATEGORY_EXTERN(LogHTML5, Log, All);
DEFINE_LOG_CATEGORY(LogHTML5);

TSharedRef< FHTML5InputInterface > FHTML5InputInterface::Create(  const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler, const TSharedPtr< ICursor >& InCursor )
{
	return MakeShareable( new FHTML5InputInterface( InMessageHandler, InCursor ) );
}

FHTML5InputInterface::FHTML5InputInterface( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler, const TSharedPtr< ICursor >& InCursor )
	: MessageHandler( InMessageHandler )
	, Cursor( InCursor )
{
#if !PLATFORM_HTML5_WIN32 || SDL_MAJOR_VERSION > 1
	// start listening for keys
	SDL_StartTextInput();
#else
	SDL_EnableKeyRepeat(100, 50);
#endif
}

void FHTML5InputInterface::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	MessageHandler = InMessageHandler;
}

void FHTML5InputInterface::Tick(float DeltaTime, const SDL_Event& Event,TSharedRef < FGenericWindow>& ApplicationWindow )
{
#if PLATFORM_HTML5_WIN32 && SDL_MAJOR_VERSION < 2
	// we have to emulate repeat
	static SDLKey lastKey = SDLK_UNKNOWN;
	bool isRepeat = false;
#define EVENT_KEY_REPEAT isRepeat
#define KMOD_GUI KMOD_META
#else
#define EVENT_KEY_REPEAT Event.key.repeat
#endif

	{
		if (Event.type == SDL_KEYDOWN)
		{
			MessageHandler->OnKeyDown(Event.key.keysym.sym, Event.key.keysym.sym, EVENT_KEY_REPEAT);
			if (!(Event.key.keysym.mod & KMOD_CTRL) && !(Event.key.keysym.mod & KMOD_ALT) && !(Event.key.keysym.mod & KMOD_GUI))
			{
				MessageHandler->OnKeyChar(Event.key.keysym.sym, EVENT_KEY_REPEAT);
			}
		}
		else if (Event.type == SDL_KEYUP)
		{
			MessageHandler->OnKeyUp(Event.key.keysym.sym, Event.key.keysym.sym, EVENT_KEY_REPEAT);
#if PLATFORM_HTML5_WIN32 && SDL_MAJOR_VERSION < 2
			lastKey = SDLK_UNKNOWN;
#endif
		}
		else if (Event.type == SDL_MOUSEBUTTONDOWN)
		{
			EMouseButtons::Type MouseButton = Event.button.button == 1 ? EMouseButtons::Left : Event.button.button == 2 ? EMouseButtons::Middle : EMouseButtons::Right;
			MessageHandler->OnMouseDown(ApplicationWindow, MouseButton );
		}
		else if (Event.type == SDL_MOUSEBUTTONUP)
		{
			EMouseButtons::Type MouseButton = Event.button.button == 1 ? EMouseButtons::Left : Event.button.button == 2 ? EMouseButtons::Middle : EMouseButtons::Right;
			MessageHandler->OnMouseUp(MouseButton );
		}
		else if (Event.type == SDL_MOUSEMOTION)
		{
			Cursor->SetPosition(Event.motion.x, Event.motion.y);
			MessageHandler->OnRawMouseMove(Event.motion.xrel, Event.motion.yrel);
			MessageHandler->OnMouseMove(); 
		} 
#if PLATFORM_HTML5_BROWSER 
		else if ( Event.type == SDL_MOUSEWHEEL )
		{
			SDL_MouseWheelEvent* w = (SDL_MouseWheelEvent*)&Event;
			const float SpinFactor = 1 / 120.0f;
			MessageHandler->OnMouseWheel(w->y * SpinFactor);
		}
#endif 

	}
}

void FHTML5InputInterface::SendControllerEvents()
{
}
