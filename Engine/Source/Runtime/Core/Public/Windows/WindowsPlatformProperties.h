// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	WindowsPlatformProperties.h - Basic static properties of a platform 
	These are shared between:
		the runtime platform - via FPlatformProperties
		the target platforms - via ITargetPlatform
==================================================================================*/

#pragma once

#include "GenericPlatformProperties.h"


/**
 * Implements Windows platform properties.
 */
template<bool HAS_EDITOR_DATA, bool IS_DEDICATED_SERVER, bool IS_CLIENT_ONLY>
struct FWindowsPlatformProperties
	: public FGenericPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		if (IS_DEDICATED_SERVER)
		{
			return "Windows (Dedicated Server)";
		}
		
		if (HAS_EDITOR_DATA)
		{
			return "Windows (Editor)";
		}
		
		if (IS_CLIENT_ONLY)
		{
			return "Windows (Client-only)";
		}

		return "Windows";
	}

	static FORCEINLINE bool HasEditorOnlyData()
	{
		return HAS_EDITOR_DATA;
	}

	static FORCEINLINE const char* IniPlatformName()
	{
		return "Windows";
	}

	static FORCEINLINE bool IsGameOnly()
	{
		return UE_GAME;
	}

	static FORCEINLINE bool IsServerOnly()
	{
		return IS_DEDICATED_SERVER;
	}

	static FORCEINLINE bool IsClientOnly()
	{
		return IS_CLIENT_ONLY;
	}

	static FORCEINLINE const char* PlatformName()
	{
		if (IS_DEDICATED_SERVER)
		{
			return "WindowsServer";
		}
		
		if (HAS_EDITOR_DATA)
		{
			return "Windows";
		}
		
		if (IS_CLIENT_ONLY)
		{
			return "WindowsClient";
		}

		return "WindowsNoEditor";
	}

	static FORCEINLINE bool RequiresCookedData()
	{
		return !HAS_EDITOR_DATA;
	}

	static FORCEINLINE bool SupportsGrayscaleSRGB()
	{
		return false; // Requires expand from G8 to RGBA
	}

	static FORCEINLINE bool SupportsMultipleGameInstances()
	{
		return true;
	}

	static FORCEINLINE bool SupportsTessellation()
	{
		return true; // DX11 compatible
	}

	static FORCEINLINE bool SupportsWindowedMode()
	{
		return true;
	}

	static FORCEINLINE bool SupportsQuit()
	{
		return true;
	}
};
