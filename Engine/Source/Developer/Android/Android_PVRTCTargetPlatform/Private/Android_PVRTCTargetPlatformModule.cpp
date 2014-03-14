// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidTargetPlatformModule.cpp: Implements the FAndroidTargetPlatformModule class.
=============================================================================*/

#include "Android_PVRTCTargetPlatformPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FAndroid_PVRTCTargetPlatformModule" 

/**
 * Android cooking platform which cooks only PVRTC based textures.
 */
class FAndroid_PVRTCTargetPlatform
	: public FAndroidTargetPlatform
{
	virtual FString GetAndroidVariantName( ) OVERRIDE
	{
		return TEXT("PVRTC");
	}

	virtual FText DisplayName( ) const OVERRIDE
	{
		return LOCTEXT("Android_PVRTC", "Android (PVRTC)");
	}

	virtual FString PlatformName() const OVERRIDE
	{
		return FString(FAndroid_PVRTCPlatformProperties::PlatformName());
	}

	virtual bool SupportsTextureFormat( FName Format ) const OVERRIDE
	{
		if( Format == AndroidTexFormat::NamePVRTC2 ||
			Format == AndroidTexFormat::NamePVRTC4 ||
			Format == AndroidTexFormat::NameAutoPVRTC )
		{
			return true;
		}
		return false;
	}
};

/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* AndroidTargetSingleton = NULL;


/**
 * Module for the Android target platform.
 */
class FAndroid_PVRTCTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FAndroid_PVRTCTargetPlatformModule( )
	{
		AndroidTargetSingleton = NULL;
	}


public:
	
	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform() OVERRIDE
	{
		if (AndroidTargetSingleton == NULL)
		{
			AndroidTargetSingleton = new FAndroid_PVRTCTargetPlatform();
		}
		
		return AndroidTargetSingleton;
	}

	// End ITargetPlatformModule interface
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FAndroid_PVRTCTargetPlatformModule, Android_PVRTCTargetPlatform);
