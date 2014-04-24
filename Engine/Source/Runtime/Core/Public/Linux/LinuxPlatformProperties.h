// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	LinuxPlatformProperties.h - Basic static properties of a platform 
	These are shared between:
		the runtime platform - via FPlatformProperties
		the target platforms - via ITargetPlatform
==================================================================================*/

#pragma once

#include "GenericPlatformProperties.h"


/**
 * Implements Linux platform properties.
 */
template<bool IS_DEDICATED_SERVER, bool IS_CLIENT_ONLY>
struct FLinuxPlatformProperties
	: public FGenericPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		return "Linux";
	}

	static FORCEINLINE bool HasEditorOnlyData( )
	{
		return false;
	}

	static FORCEINLINE const char* IniPlatformName( )
	{
		return "Linux";
	}

	static FORCEINLINE bool IsGameOnly( )
	{
		return UE_GAME;
	}

	static FORCEINLINE bool IsServerOnly( )
	{
		return IS_DEDICATED_SERVER;
	}

	static FORCEINLINE bool IsClientOnly()
	{
		return IS_CLIENT_ONLY;
	}

	static FORCEINLINE const char* PlatformName( )
	{
		if (IS_DEDICATED_SERVER)
		{
			return "LinuxServer";
		}
		else
		{
			return "Linux";
		}
	}

	static FORCEINLINE bool RequiresCookedData( )
	{
		return true;
	}

	static FORCEINLINE bool RequiresUserCredentials()
	{
		return true;
	}

	static FORCEINLINE bool SupportsMultipleGameInstances( )
	{
		return true;
	}

	static FORCEINLINE bool HasFixedResolution()
	{
		return false;
	}

	static FORCEINLINE bool SupportsWindowedMode()
	{
		return !IS_DEDICATED_SERVER;
	}

	static FORCEINLINE bool SupportsQuit()
	{
		return true;
	}
};
