// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LinuxPlatformApplicationMisc.h"
#include "LinuxApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"

bool GInitializedSDL = false;

namespace
{
	uint32 GWindowStyleSDL = SDL_WINDOW_OPENGL;
}

uint32 FLinuxPlatformApplicationMisc::WindowStyle()
{
	return GWindowStyleSDL;
}

void FLinuxPlatformApplicationMisc::Init()
{
	// skip for servers and programs, unless they request later
	bool bIsNullRHI = !FApp::CanEverRender();
	if (!IS_PROGRAM && !bIsNullRHI)
	{
		InitSDL();
	}
}

bool FLinuxPlatformApplicationMisc::InitSDL()
{
	if (!GInitializedSDL)
	{
		UE_LOG(LogInit, Log, TEXT("Initializing SDL."));

		if (FParse::Param(FCommandLine::Get(), TEXT("vulkan")))
		{
			GWindowStyleSDL = SDL_WINDOW_VULKAN;
			UE_LOG(LogInit, Log, TEXT("Using SDL_WINDOW_VULKAN"));
		}
		else
		{
			GWindowStyleSDL = SDL_WINDOW_OPENGL;
			UE_LOG(LogInit, Log, TEXT("Using SDL_WINDOW_OPENGL"));
		}

		SDL_SetHint("SDL_VIDEO_X11_REQUIRE_XRANDR", "1");  // workaround for misbuilt SDL libraries on X11.
		// we don't use SDL for audio
		if (SDL_Init((SDL_INIT_EVERYTHING ^ SDL_INIT_AUDIO) | SDL_INIT_NOPARACHUTE) != 0)
		{
			const char * SDLError = SDL_GetError();

			// do not fail at this point, allow caller handle failure
			UE_LOG(LogInit, Warning, TEXT("Could not initialize SDL: %s"), UTF8_TO_TCHAR(SDLError));
			return false;
		}

		// print out version information
		SDL_version CompileTimeSDLVersion;
		SDL_version RunTimeSDLVersion;
		SDL_VERSION(&CompileTimeSDLVersion);
		SDL_GetVersion(&RunTimeSDLVersion);
		int SdlRevisionNum = SDL_GetRevisionNumber();
		FString SdlRevision = UTF8_TO_TCHAR(SDL_GetRevision());
		UE_LOG(LogInit, Log, TEXT("Initialized SDL %d.%d.%d revision: %d (%s) (compiled against %d.%d.%d)"),
			RunTimeSDLVersion.major, RunTimeSDLVersion.minor, RunTimeSDLVersion.patch,
			SdlRevisionNum, *SdlRevision,
			CompileTimeSDLVersion.major, CompileTimeSDLVersion.minor, CompileTimeSDLVersion.patch
			);

		// Used to make SDL push SDL_TEXTINPUT events.
		SDL_StartTextInput();

		GInitializedSDL = true;

		// needs to come after GInitializedSDL, otherwise it will recurse here
		if (!UE_BUILD_SHIPPING)
		{
			// dump information about screens for debug
			FDisplayMetrics DisplayMetrics;
			FDisplayMetrics::GetDisplayMetrics(DisplayMetrics);
			DisplayMetrics.PrintToLog();
		}
	}

	return true;
}

void FLinuxPlatformApplicationMisc::TearDown()
{
	if (GInitializedSDL)
	{
		UE_LOG(LogInit, Log, TEXT("Tearing down SDL."));
		SDL_Quit();
		GInitializedSDL = false;
	}
}

GenericApplication* FLinuxPlatformApplicationMisc::CreateApplication()
{
	return FLinuxApplication::CreateLinuxApplication();
}

void FLinuxPlatformApplicationMisc::PumpMessages( bool bFromMainLoop )
{
	if (GInitializedSDL && bFromMainLoop)
	{
		if( LinuxApplication )
		{
			LinuxApplication->SaveWindowLocationsForEventLoop();

			SDL_Event event;

			while (SDL_PollEvent(&event))
			{
				LinuxApplication->AddPendingEvent( event );
			}

			LinuxApplication->ClearWindowLocationsAfterEventLoop();
		}
		else
		{
			// No application to send events to. Just flush out the
			// queue.
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				// noop
			}
		}
	}
}

bool FLinuxPlatformApplicationMisc::ControlScreensaver(EScreenSaverAction Action)
{
	if (Action == FGenericPlatformApplicationMisc::EScreenSaverAction::Disable)
	{
		SDL_DisableScreenSaver();
	}
	else
	{
		SDL_EnableScreenSaver();
	}

	return true;
}

uint32 FLinuxPlatformApplicationMisc::GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
	return FGenericPlatformApplicationMisc::GetStandardPrintableKeyMap(KeyCodes, KeyNames, MaxMappings, false, true);
}

uint32 FLinuxPlatformApplicationMisc::GetKeyMap( uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings )
{
#define ADDKEYMAP(KeyCode, KeyName)		if (NumMappings<MaxMappings) { KeyCodes[NumMappings]=KeyCode; KeyNames[NumMappings]=KeyName; ++NumMappings; };

	uint32 NumMappings = 0;

	if (KeyCodes && KeyNames && (MaxMappings > 0))
	{
		ADDKEYMAP(SDLK_BACKSPACE, TEXT("BackSpace"));
		ADDKEYMAP(SDLK_TAB, TEXT("Tab"));
		ADDKEYMAP(SDLK_RETURN, TEXT("Enter"));
		ADDKEYMAP(SDLK_RETURN2, TEXT("Enter"));
		ADDKEYMAP(SDLK_KP_ENTER, TEXT("Enter"));
		ADDKEYMAP(SDLK_PAUSE, TEXT("Pause"));

		ADDKEYMAP(SDLK_ESCAPE, TEXT("Escape"));
		ADDKEYMAP(SDLK_SPACE, TEXT("SpaceBar"));
		ADDKEYMAP(SDLK_PAGEUP, TEXT("PageUp"));
		ADDKEYMAP(SDLK_PAGEDOWN, TEXT("PageDown"));
		ADDKEYMAP(SDLK_END, TEXT("End"));
		ADDKEYMAP(SDLK_HOME, TEXT("Home"));

		ADDKEYMAP(SDLK_LEFT, TEXT("Left"));
		ADDKEYMAP(SDLK_UP, TEXT("Up"));
		ADDKEYMAP(SDLK_RIGHT, TEXT("Right"));
		ADDKEYMAP(SDLK_DOWN, TEXT("Down"));

		ADDKEYMAP(SDLK_INSERT, TEXT("Insert"));
		ADDKEYMAP(SDLK_DELETE, TEXT("Delete"));

		ADDKEYMAP(SDLK_F1, TEXT("F1"));
		ADDKEYMAP(SDLK_F2, TEXT("F2"));
		ADDKEYMAP(SDLK_F3, TEXT("F3"));
		ADDKEYMAP(SDLK_F4, TEXT("F4"));
		ADDKEYMAP(SDLK_F5, TEXT("F5"));
		ADDKEYMAP(SDLK_F6, TEXT("F6"));
		ADDKEYMAP(SDLK_F7, TEXT("F7"));
		ADDKEYMAP(SDLK_F8, TEXT("F8"));
		ADDKEYMAP(SDLK_F9, TEXT("F9"));
		ADDKEYMAP(SDLK_F10, TEXT("F10"));
		ADDKEYMAP(SDLK_F11, TEXT("F11"));
		ADDKEYMAP(SDLK_F12, TEXT("F12"));

		ADDKEYMAP(SDLK_LCTRL, TEXT("LeftControl"));
		ADDKEYMAP(SDLK_LSHIFT, TEXT("LeftShift"));
		ADDKEYMAP(SDLK_LALT, TEXT("LeftAlt"));
		ADDKEYMAP(SDLK_LGUI, TEXT("LeftCommand"));
		ADDKEYMAP(SDLK_RCTRL, TEXT("RightControl"));
		ADDKEYMAP(SDLK_RSHIFT, TEXT("RightShift"));
		ADDKEYMAP(SDLK_RALT, TEXT("RightAlt"));
		ADDKEYMAP(SDLK_RGUI, TEXT("RightCommand"));

		ADDKEYMAP(SDLK_KP_0, TEXT("NumPadZero"));
		ADDKEYMAP(SDLK_KP_1, TEXT("NumPadOne"));
		ADDKEYMAP(SDLK_KP_2, TEXT("NumPadTwo"));
		ADDKEYMAP(SDLK_KP_3, TEXT("NumPadThree"));
		ADDKEYMAP(SDLK_KP_4, TEXT("NumPadFour"));
		ADDKEYMAP(SDLK_KP_5, TEXT("NumPadFive"));
		ADDKEYMAP(SDLK_KP_6, TEXT("NumPadSix"));
		ADDKEYMAP(SDLK_KP_7, TEXT("NumPadSeven"));
		ADDKEYMAP(SDLK_KP_8, TEXT("NumPadEight"));
		ADDKEYMAP(SDLK_KP_9, TEXT("NumPadNine"));
		ADDKEYMAP(SDLK_KP_MULTIPLY, TEXT("Multiply"));
		ADDKEYMAP(SDLK_KP_PLUS, TEXT("Add"));
		ADDKEYMAP(SDLK_KP_MINUS, TEXT("Subtract"));
		ADDKEYMAP(SDLK_KP_DECIMAL, TEXT("Decimal"));
		ADDKEYMAP(SDLK_KP_DIVIDE, TEXT("Divide"));

		ADDKEYMAP(SDLK_CAPSLOCK, TEXT("CapsLock"));
		ADDKEYMAP(SDLK_NUMLOCKCLEAR, TEXT("NumLock"));
		ADDKEYMAP(SDLK_SCROLLLOCK, TEXT("ScrollLock"));
	}

	check(NumMappings < MaxMappings);
	return NumMappings;
}

void FLinuxPlatformApplicationMisc::ClipboardCopy(const TCHAR* Str)
{
	if (SDL_SetClipboardText(TCHAR_TO_UTF8(Str)))
	{
		UE_LOG(LogInit, Fatal, TEXT("Error copying clipboard contents: %s\n"), UTF8_TO_TCHAR(SDL_GetError()));
	}
}

void FLinuxPlatformApplicationMisc::ClipboardPaste(class FString& Result)
{
	char* ClipContent;
	ClipContent = SDL_GetClipboardText();

	if (!ClipContent)
	{
		UE_LOG(LogInit, Fatal, TEXT("Error pasting clipboard contents: %s\n"), UTF8_TO_TCHAR(SDL_GetError()));
		// unreachable
		Result = TEXT("");
	}
	else
	{
		Result = FString(UTF8_TO_TCHAR(ClipContent));
	}
	SDL_free(ClipContent);
}

