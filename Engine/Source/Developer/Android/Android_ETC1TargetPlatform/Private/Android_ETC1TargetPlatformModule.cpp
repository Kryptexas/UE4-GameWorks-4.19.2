// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidETC1_TargetPlatformModule.cpp: Implements the FAndroidETC1_TargetPlatformModule class.
=============================================================================*/

#include "Android_ETC1TargetPlatformPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FAndroid_ETC1TargetPlatformModule" 


/**
 * Android cooking platform which cooks only ETC1 based textures.
 */
class FAndroid_ETC1TargetPlatform
	: public FAndroidTargetPlatform
{
public:

	// Begin FAndroidTargetPlatform overrides

	virtual FText DisplayName( ) const OVERRIDE
	{
		return LOCTEXT("Android_ETC1", "Android (ETC1)");
	}

	virtual FString GetAndroidVariantName( )
	{
		return TEXT("ETC1");
	}

	virtual FString PlatformName() const OVERRIDE
	{
		return FString(FAndroid_ETC1PlatformProperties::PlatformName());
	}

	virtual bool SupportsTextureFormat( FName Format ) const OVERRIDE
	{
		if (Format == AndroidTexFormat::NameETC1 || 
			Format == AndroidTexFormat::NameAutoETC1)
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
class FAndroid_ETC1TargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FAndroid_ETC1TargetPlatformModule( )
	{
		AndroidTargetSingleton = NULL;
	}

public:
	
	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform() OVERRIDE
	{
		if (AndroidTargetSingleton == NULL)
		{
			AndroidTargetSingleton = new FAndroid_ETC1TargetPlatform();
		}
		
		return AndroidTargetSingleton;
	}

	// End ITargetPlatformModule interface
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FAndroid_ETC1TargetPlatformModule, Android_ETC1TargetPlatform);
