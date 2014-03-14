// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	MacPlatformProperties.h - Basic static properties of a platform 
	These are shared between:
		the runtime platform - via FPlatformProperties
		the target platforms - via ITargetPlatform
==================================================================================*/

#pragma once

#include "GenericPlatformProperties.h"


/**
 * Implements Mac platform properties.
 */
template<bool HAS_EDITOR_DATA, bool IS_DEDICATED_SERVER>
struct FMacPlatformProperties
	: public FGenericPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		if (IS_DEDICATED_SERVER)
		{
			return "Mac (Dedicated Server)";
		}
		
		if (HAS_EDITOR_DATA)
		{
			return "Mac (Editor)";
		}

		return "Mac";
	}

	static FORCEINLINE bool HasEditorOnlyData( )
	{
		return HAS_EDITOR_DATA;
	}

	static FORCEINLINE const char* IniPlatformName( )
	{
		return "Mac";
	}

	static FORCEINLINE bool IsGameOnly( )
	{
		return UE_GAME;
	}

	static FORCEINLINE bool IsServerOnly( )
	{
		return IS_DEDICATED_SERVER;
	}

	static FORCEINLINE const char* PlatformName( )
	{
		if (IS_DEDICATED_SERVER)
		{
			return "MacServer";
		}
		
		if (HAS_EDITOR_DATA)
		{
			return "Mac";
		}

		return "MacNoEditor";
	}

	static FORCEINLINE bool RequiresCookedData( )
	{
		return !HAS_EDITOR_DATA;
	}

	static FORCEINLINE bool SupportsMultipleGameInstances( )
	{
		return false;
	}
	static FORCEINLINE bool SupportsWindowedMode( )
	{
		return true;
	}

	static FORCEINLINE bool SupportsQuit()
	{
		return true;
	}
};
