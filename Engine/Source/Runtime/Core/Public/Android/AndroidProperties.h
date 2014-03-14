// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	AndroidProperties.h - Basic static properties of a platform 
	These are shared between:
		the runtime platform - via FPlatformProperties
		the target platforms - via ITargetPlatform
==================================================================================*/

#pragma once

#include "GenericPlatformProperties.h"


/**
 * Implements Android platform properties.
 */
struct FAndroidPlatformProperties
	: public FGenericPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		return "Android";
	}

	static FORCEINLINE const char* GetPhysicsFormat( )
	{
		return "PhysXPC";		//@todo android: physx format
	}

	static FORCEINLINE bool HasEditorOnlyData( )
	{
		return false;
	}

	static FORCEINLINE const char* PlatformName()
	{
		return "Android";
	}

	static FORCEINLINE const char* IniPlatformName()
	{
		return "Android";
	}

	static FORCEINLINE bool IsGameOnly( )
	{
		return UE_GAME;
	}

	static FORCEINLINE uint32 MaxGpuSkinBones( )
	{
		return 20;
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

	static FORCEINLINE bool SupportsLowQualityLightmaps()
	{
		return true;
	}

	static FORCEINLINE bool SupportsDistanceFieldShadows()
	{
		return false;
	}

	static FORCEINLINE bool SupportsTextureStreaming()
	{
		return false;
	}

	static FORCEINLINE bool SupportsVertexShaderTextureSampling()
	{
		return false;
	}
};

struct FAndroid_PVRTCPlatformProperties : public FAndroidPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		return "Android (PVRTC)";
	}

	static FORCEINLINE const char* PlatformName()
	{
		return "Android_PVRTC";
	}
};

struct FAndroid_ATCCPlatformProperties : public FAndroidPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		return "Android (ATC)";
	}

	static FORCEINLINE const char* PlatformName()
	{
		return "Android_ATC";
	}
};

struct FAndroid_DXTPlatformProperties : public FAndroidPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		return "Android (DXT)";
	}

	static FORCEINLINE const char* PlatformName()
	{
		return "Android_DXT";
	}
};

struct FAndroid_ETC1PlatformProperties : public FAndroidPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		return "Android (ETC1)";
	}

	static FORCEINLINE const char* PlatformName()
	{
		return "Android_ETC1";
	}
};

struct FAndroid_ETC2PlatformProperties : public FAndroidPlatformProperties
{
	static FORCEINLINE const char* DisplayName()
	{
		return "Android (ETC2)";
	}

	static FORCEINLINE const char* PlatformName()
	{
		return "Android_ETC2";
	}
};

