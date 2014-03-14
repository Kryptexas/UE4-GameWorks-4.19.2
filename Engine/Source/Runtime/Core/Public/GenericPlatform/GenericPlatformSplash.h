// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	GenericPlatformSplash.h: Generic platform splash screen...does nothing
==============================================================================================*/

#pragma once

/**
 * SplashTextType defines the types of text on the splash screen
 */
namespace SplashTextType
{
	enum Type
	{
		/** Startup progress text */
		StartupProgress	= 0,

		/** Version information text line 1 */
		VersionInfo1,

		/** Copyright information text */
		CopyrightInfo,

		// ...

		/** Number of text types (must be final enum value) */
		NumTextTypes
	};
}

/**
* Generic implementation for most platforms
**/
struct CORE_API FGenericPlatformSplash
{
	/**
	* Show the splash screen
	*/
	FORCEINLINE static void Show()
	{
	}
	/**
	* Hide the splash screen
	*/
	FORCEINLINE static void Hide()
	{
	}

	/**
	 * Sets the text displayed on the splash screen (for startup/loading progress)
	 *
	 * @param	InType		Type of text to change
	 * @param	InText		Text to display
	 */
	FORCEINLINE static void SetSplashText( const SplashTextType::Type InType, const TCHAR* InText )
	{

	}

};
