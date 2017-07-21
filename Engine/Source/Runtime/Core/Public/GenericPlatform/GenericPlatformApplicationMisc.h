// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "GenericApplication.h"

struct CORE_API FGenericPlatformApplicationMisc
{
	FORCEINLINE static void Init() 
	{ 
	}

	FORCEINLINE static void TearDown() 
	{ 
	}

	static class GenericApplication* CreateApplication();

	/**
	 *	Pumps Windows messages.
	 *	@param bFromMainLoop if true, this is from the main loop, otherwise we are spinning waiting for the render thread
	 */
	FORCEINLINE static void PumpMessages(bool bFromMainLoop)
	{
	}

	/**
	 * Prevents screen-saver from kicking in by moving the mouse by 0 pixels. This works even on
	 * Vista in the presence of a group policy for password protected screen saver.
	 */
	static void PreventScreenSaver()
	{
	}

	enum EScreenSaverAction
	{
		Disable,
		Enable
	};

	/**
	 * Disables screensaver (if platform supports such an API)
	 *
	 * @param Action enable or disable
	 * @return true if succeeded, false if platform does not have this API and PreventScreenSaver() hack is needed
	 */
	static bool ControlScreensaver(EScreenSaverAction Action)
	{
		return false;
	}

	FORCEINLINE static uint32 GetKeyMap( uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings )
	{
		return 0;
	}

	FORCEINLINE static uint32 GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings)
	{
		return 0;
	}

	/**
	 * Sample the displayed pixel color from anywhere on the screen using the OS
	 *
	 * @param	InScreenPos		The screen coords to sample for current pixel color
	 * @param	InGamma			Optional gamma correction to apply to the screen color
	 * @return					The color of the pixel displayed at the chosen location
	 */
	static struct FLinearColor GetScreenPixelColor(const struct FVector2D& InScreenPos, float InGamma = 1.0f);

	/**
	 * Searches for a window that matches the window name or the title starts with a particular text. When
	 * found, it returns the title text for that window
	 *
	 * @param TitleStartsWith an alternative search method that knows part of the title text
	 * @param OutTitle the string the data is copied into
	 *
	 * @return whether the window was found and the text copied or not
	 */
	static bool GetWindowTitleMatchingText(const TCHAR* TitleStartsWith, FString& OutTitle)
	{
		return false;
	}

	/**
	* Returns monitor's DPI scale factor at given screen coordinates (expressed in pixels)
	* @return Monitor's DPI scale factor at given point
	*/
	static float GetDPIScaleFactorAtPoint(float X, float Y)
	{
		return 1.0f;
	}

	/*
	 * Resets the gamepad to player controller id assignments
	 */
	static void ResetGamepadAssignments()
	{}

	/*
	* Resets the gamepad assignment to player controller id
	*/
	static void ResetGamepadAssignmentToController(int32 ControllerId)
	{}

	/*
	 * Returns true if controller id assigned to a gamepad
	 */
	static bool IsControllerAssignedToGamepad(int32 ControllerId)
	{
		return (ControllerId == 0);
	}

	/** Copies text to the operating system clipboard. */
	static void ClipboardCopy(const TCHAR* Str);

	/** Pastes in text from the operating system clipboard. */
	static void ClipboardPaste(class FString& Dest);

protected:

	/**
	* Retrieves some standard key code mappings (usually called by a subclass's GetCharKeyMap)
	*
	* @param OutKeyMap Key map to add to.
	* @param bMapUppercaseKeys If true, will map A, B, C, etc to EKeys::A, EKeys::B, EKeys::C
	* @param bMapLowercaseKeys If true, will map a, b, c, etc to EKeys::A, EKeys::B, EKeys::C
	*/
	static uint32 GetStandardPrintableKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings, bool bMapUppercaseKeys, bool bMapLowercaseKeys);
};
