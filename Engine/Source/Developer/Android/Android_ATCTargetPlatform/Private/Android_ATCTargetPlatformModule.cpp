// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidATC_TargetPlatformModule.cpp: Implements the FAndroidATC_TargetPlatformModule class.
=============================================================================*/

#include "Android_ATCTargetPlatformPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FAndroid_ATCTargetPlatformModule" 


/**
 * Android cooking platform which cooks only ATC based textures.
 */
class FAndroid_ATCTargetPlatform
	: public FAndroidTargetPlatform
{
	virtual FString GetAndroidVariantName( ) OVERRIDE
	{
		return TEXT("ATC");
	}

	virtual FText DisplayName( ) const OVERRIDE
	{
		return LOCTEXT("Android_ATC", "Android (ATC)");
	}

	virtual FString PlatformName() const OVERRIDE
	{
		return FString(FAndroid_ATCCPlatformProperties::PlatformName());
	}

	virtual bool SupportsTextureFormat( FName Format ) const OVERRIDE
	{
		if( Format == AndroidTexFormat::NameATC_RGB ||
			Format == AndroidTexFormat::NameATC_RGBA_I ||
			Format == AndroidTexFormat::NameAutoATC )
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
class FAndroid_ATCTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FAndroid_ATCTargetPlatformModule( )
	{
		AndroidTargetSingleton = NULL;
	}


public:
	
	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform() OVERRIDE
	{
		if (AndroidTargetSingleton == NULL)
		{
			AndroidTargetSingleton = new FAndroid_ATCTargetPlatform();
		}
		
		return AndroidTargetSingleton;
	}

	// End ITargetPlatformModule interface
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FAndroid_ATCTargetPlatformModule, Android_ATCTargetPlatform);
