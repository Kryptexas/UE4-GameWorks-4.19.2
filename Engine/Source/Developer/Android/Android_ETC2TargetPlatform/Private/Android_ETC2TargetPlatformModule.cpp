// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidETC2_TargetPlatformModule.cpp: Implements the FAndroidETC2_TargetPlatformModule class.
=============================================================================*/

#include "Android_ETC2TargetPlatformPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FAndroid_ETC2TargetPlatformModule" 


/**
 * Android cooking platform which cooks only ETC2 based textures.
 */
class FAndroid_ETC2TargetPlatform
	: public FAndroidTargetPlatform
{
public:

	// Begin FAndroidTargetPlatform overrides

	virtual FText DisplayName( ) const OVERRIDE
	{
		return LOCTEXT("Android_ETC2", "Android (ETC2)");
	}

	virtual FString GetAndroidVariantName( )
	{
		return TEXT("ETC2");
	}

	virtual FString PlatformName() const OVERRIDE
	{
		return FString(FAndroid_ETC2PlatformProperties::PlatformName());
	}

	virtual bool SupportsTextureFormat( FName Format ) const OVERRIDE
	{
		if (Format == AndroidTexFormat::NameETC2_RGB ||
			Format == AndroidTexFormat::NameETC2_RGBA ||
			Format == AndroidTexFormat::NameAutoETC2)
		{
			return true;
		}

		return false;
	}

	// End FAndroidTargetPlatform overrides
};


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* AndroidTargetSingleton = NULL;


/**
 * Module for the Android target platform.
 */
class FAndroid_ETC2TargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FAndroid_ETC2TargetPlatformModule( )
	{
		AndroidTargetSingleton = NULL;
	}

public:
	
	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform() OVERRIDE
	{
		if (AndroidTargetSingleton == NULL)
		{
			AndroidTargetSingleton = new FAndroid_ETC2TargetPlatform();
		}
		
		return AndroidTargetSingleton;
	}

	// End ITargetPlatformModule interface
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FAndroid_ETC2TargetPlatformModule, Android_ETC2TargetPlatform);
