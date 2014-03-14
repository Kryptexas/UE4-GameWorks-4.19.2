// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HTML5Misc.cpp: HTML5 implementations of misc functions
=============================================================================*/

#include "CorePrivate.h"
#include "HTML5Application.h"
#include <ctime>

void FHTML5Misc::PlatformInit()
{
	// Randomize.
	if (GIsBenchmarking)
	{
		srand(0);
	}
	else
	{
		srand((unsigned)time(NULL));
	}

	// Identity.
	UE_LOG(LogInit, Log, TEXT("Computer: %s"), FPlatformProcess::ComputerName());
	UE_LOG(LogInit, Log, TEXT("User: %s"), FPlatformProcess::UserName());

	// Timer resolution.
	UE_LOG(LogInit, Log, TEXT("High frequency timer resolution =%f MHz"), 0.000001 / FPlatformTime::GetSecondsPerCycle());
}

GenericApplication* FHTML5Misc::CreateApplication()
{
	return FHTML5Application::CreateHTML5Application();
}

#if PLATFORM_HTML5_WIN32 && SDL_MAJOR_VERSION < 2
#define SDLK_KP_0 SDLK_KP0
#define SDLK_KP_1 SDLK_KP1
#define SDLK_KP_2 SDLK_KP2
#define SDLK_KP_3 SDLK_KP3
#define SDLK_KP_4 SDLK_KP4
#define SDLK_KP_5 SDLK_KP5
#define SDLK_KP_6 SDLK_KP6
#define SDLK_KP_7 SDLK_KP7
#define SDLK_KP_8 SDLK_KP8
#define SDLK_KP_9 SDLK_KP9
#define SDLK_SCROLLLOCK SDLK_SCROLLOCK
#endif

uint32 FHTML5Misc::GetCharKeyMap(uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
	return FGenericPlatformMisc::GetStandardPrintableKeyMap(KeyCodes, KeyNames, MaxMappings, false, true);
}

uint32 FHTML5Misc::GetKeyMap(uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
#define ADDKEYMAP(KeyCode, KeyName)		if (NumMappings<MaxMappings) { KeyCodes[NumMappings]=KeyCode; KeyNames[NumMappings]=KeyName; ++NumMappings; };

	uint32 NumMappings = 0;

	if (KeyCodes && KeyNames && (MaxMappings > 0))
	{
		ADDKEYMAP( SDLK_BACKSPACE, TEXT("BackSpace") );
		ADDKEYMAP( SDLK_TAB, TEXT("Tab") );
		ADDKEYMAP( SDLK_RETURN, TEXT("Enter") );
		ADDKEYMAP( SDLK_PAUSE, TEXT("Pause") );

		ADDKEYMAP( SDLK_CAPSLOCK, TEXT("CapsLock") );
		ADDKEYMAP( SDLK_ESCAPE, TEXT("Escape") );
		ADDKEYMAP( SDLK_SPACE, TEXT("SpaceBar") );
		ADDKEYMAP( SDLK_PAGEUP, TEXT("PageUp") );
		ADDKEYMAP( SDLK_PAGEDOWN, TEXT("PageDown") );
		ADDKEYMAP( SDLK_END, TEXT("End") );
		ADDKEYMAP( SDLK_HOME, TEXT("Home") );

		ADDKEYMAP( SDLK_LEFT, TEXT("Left") );
		ADDKEYMAP( SDLK_UP, TEXT("Up") );
		ADDKEYMAP( SDLK_RIGHT, TEXT("Right") );
		ADDKEYMAP( SDLK_DOWN, TEXT("Down") );

		ADDKEYMAP( SDLK_INSERT, TEXT("Insert") );
		ADDKEYMAP( SDLK_DELETE, TEXT("Delete") );

		ADDKEYMAP( SDLK_KP_0, TEXT("NumPadZero") );
		ADDKEYMAP( SDLK_KP_1, TEXT("NumPadOne") );
		ADDKEYMAP( SDLK_KP_2, TEXT("NumPadTwo") );
		ADDKEYMAP( SDLK_KP_3, TEXT("NumPadThree") );
		ADDKEYMAP( SDLK_KP_4, TEXT("NumPadFour") );
		ADDKEYMAP( SDLK_KP_5, TEXT("NumPadFive") );
		ADDKEYMAP( SDLK_KP_6, TEXT("NumPadSix") );
		ADDKEYMAP( SDLK_KP_7, TEXT("NumPadSeven") );
		ADDKEYMAP( SDLK_KP_8, TEXT("NumPadEight") );
		ADDKEYMAP( SDLK_KP_9, TEXT("NumPadNine") );

		ADDKEYMAP( SDLK_KP_MULTIPLY, TEXT("Multiply") );
		ADDKEYMAP( SDLK_KP_PLUS, TEXT("Add") );
		ADDKEYMAP( SDLK_KP_MINUS, TEXT("Subtract") );
		ADDKEYMAP( SDLK_KP_PERIOD, TEXT("Decimal") );
		ADDKEYMAP( SDLK_KP_DIVIDE, TEXT("Divide") );

		ADDKEYMAP( SDLK_F1, TEXT("F1") );
		ADDKEYMAP( SDLK_F2, TEXT("F2") );
		ADDKEYMAP( SDLK_F3, TEXT("F3") );
		ADDKEYMAP( SDLK_F4, TEXT("F4") );
		ADDKEYMAP( SDLK_F5, TEXT("F5") );
		ADDKEYMAP( SDLK_F6, TEXT("F6") );
		ADDKEYMAP( SDLK_F7, TEXT("F7") );
		ADDKEYMAP( SDLK_F8, TEXT("F8") );
		ADDKEYMAP( SDLK_F9, TEXT("F9") );
		ADDKEYMAP( SDLK_F10, TEXT("F10") );
		ADDKEYMAP( SDLK_F11, TEXT("F11") );
		ADDKEYMAP( SDLK_F12, TEXT("F12") );

//		ADDKEYMAP( VK_NUMLOCK, TEXT("NumLock") );

		ADDKEYMAP( SDLK_SCROLLLOCK, TEXT("ScrollLock") );

		ADDKEYMAP( SDLK_LSHIFT, TEXT("LeftShift") );
		ADDKEYMAP( SDLK_RSHIFT, TEXT("RightShift") );
		ADDKEYMAP( SDLK_LCTRL, TEXT("LeftControl") );
		ADDKEYMAP( SDLK_RCTRL, TEXT("RightControl") );
		ADDKEYMAP( SDLK_LALT, TEXT("LeftAlt") );
		ADDKEYMAP( SDLK_RALT, TEXT("RightAlt") );

		ADDKEYMAP( SDLK_SEMICOLON, TEXT("Semicolon") );
		ADDKEYMAP( SDLK_EQUALS, TEXT("Equals") );
		ADDKEYMAP( SDLK_COMMA, TEXT("Comma") );
		ADDKEYMAP( SDLK_UNDERSCORE, TEXT("Underscore") );
		ADDKEYMAP( SDLK_PERIOD, TEXT("Period") );
		ADDKEYMAP( SDLK_SLASH, TEXT("Slash") );
		ADDKEYMAP( SDLK_BACKQUOTE, TEXT("Tilde") );
		ADDKEYMAP( SDLK_LEFTBRACKET, TEXT("LeftBracket") );
		ADDKEYMAP( SDLK_BACKSLASH, TEXT("Backslash") );
		ADDKEYMAP( SDLK_RIGHTBRACKET, TEXT("RightBracket") );
		ADDKEYMAP( SDLK_QUOTE, TEXT("Quote") );
//		ADDKEYMAP( VK_LWIN, TEXT("LeftCommand") );
//		ADDKEYMAP( VK_RWIN, TEXT("RightCommand") );
	}

	return NumMappings;
}
