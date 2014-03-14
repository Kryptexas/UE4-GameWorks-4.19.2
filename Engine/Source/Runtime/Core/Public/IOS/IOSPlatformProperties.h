// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	IOSPlatformProperties.h - Basic static properties of a platform 
	These are shared between:
		the runtime platform - via FPlatformProperties
		the target platforms - via ITargetPlatform
==================================================================================*/

#pragma once

#include "GenericPlatformProperties.h"


/**
 * Implements iOS platform properties.
 */
struct FIOSPlatformProperties
	: public FGenericPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		return "iOS";
	}

	static FORCEINLINE bool HasEditorOnlyData( )
	{
		return false;
	}

	static FORCEINLINE uint32 MaxGpuSkinBones( )
	{
		return 20;
	}

	static FORCEINLINE const char* PlatformName( )
	{
		return "IOS";
	}

	static FORCEINLINE const char* IniPlatformName( )
	{
		return "IOS";
	}

	static FORCEINLINE bool RequiresCookedData( )
	{
		return true;
	}

	static FORCEINLINE bool SupportsBuildTarget( EBuildTargets::Type BuildTarget )
	{
		return (BuildTarget == EBuildTargets::Game);
	}

	static FORCEINLINE bool SupportsHighQualityLightmaps()
	{
		return false;
	}

	static FORCEINLINE bool SupportsTextureStreaming()
	{
		return true;
	}

	static FORCEINLINE bool SupportsVertexShaderTextureSampling()
	{
		return false;
	}
};
