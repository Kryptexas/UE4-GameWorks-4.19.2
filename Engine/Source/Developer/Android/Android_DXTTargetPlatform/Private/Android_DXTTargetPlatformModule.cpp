// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidDXT_TargetPlatformModule.cpp: Implements the FAndroidDXT_TargetPlatformModule class.
=============================================================================*/

#include "Android_DXTTargetPlatformPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FAndroid_DXTTargetPlatformModule" 


/**
 * Android cooking platform which cooks only DXT based textures.
 */
class FAndroid_DXTTargetPlatform
	: public FAndroidTargetPlatform
{
	virtual FString GetAndroidVariantName( ) OVERRIDE
	{
		return TEXT("DXT");
	}

	virtual FText DisplayName( ) const OVERRIDE
	{
		return LOCTEXT("Android_DXT", "Android (DXT)");
	}

	virtual FString PlatformName() const OVERRIDE
	{
		return FString(FAndroid_DXTPlatformProperties::PlatformName());
	}

	virtual bool SupportsTextureFormat( FName Format ) const OVERRIDE
	{
		if( Format == AndroidTexFormat::NameDXT1 ||
			Format == AndroidTexFormat::NameDXT5 ||
			Format == AndroidTexFormat::NameAutoDXT )
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
class FAndroid_DXTTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FAndroid_DXTTargetPlatformModule( )
	{
		AndroidTargetSingleton = NULL;
	}


public:
	
	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform() OVERRIDE
	{
		if (AndroidTargetSingleton == NULL)
		{
			AndroidTargetSingleton = new FAndroid_DXTTargetPlatform();
		}
		
		return AndroidTargetSingleton;
	}

	// End ITargetPlatformModule interface
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FAndroid_DXTTargetPlatformModule, Android_DXTTargetPlatform);
