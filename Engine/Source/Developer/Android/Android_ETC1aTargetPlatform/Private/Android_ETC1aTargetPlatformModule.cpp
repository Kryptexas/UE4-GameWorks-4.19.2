// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidETC1a_TargetPlatformModule.cpp: Implements the FAndroidETC1a_TargetPlatformModule class.
=============================================================================*/

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Android/AndroidProperties.h"
#include "Interfaces/ITargetPlatformModule.h"
#include "Common/TargetPlatformBase.h"
#include "Interfaces/IAndroidDeviceDetection.h"
#include "Interfaces/IAndroidDeviceDetectionModule.h"
#include "AndroidTargetDevice.h"
#include "AndroidTargetPlatform.h"

#define LOCTEXT_NAMESPACE "FAndroid_ETC1aTargetPlatformModule" 


/**
 * Android cooking platform which cooks only ETC1a based textures.
 */
class FAndroid_ETC1aTargetPlatform
	: public FAndroidTargetPlatform<FAndroid_ETC1aPlatformProperties>
{
public:

	// Begin FAndroidTargetPlatform overrides

	virtual FText DisplayName() const override
	{
		return LOCTEXT("Android_ETC1a", "Android (ETC1a)");
	}

	virtual FString GetAndroidVariantName()
	{
		return TEXT("ETC1a");
	}

	virtual FString PlatformName() const override
	{
		return FString(FAndroid_ETC1aPlatformProperties::PlatformName());
	}

	virtual bool SupportsTextureFormat(FName Format) const override
	{
		if (Format == AndroidTexFormat::NameAutoETC1a)
		{
			return true;
		}

		return false;
	}

	// End FAndroidTargetPlatform overrides

	virtual bool SupportedByExtensionsString(const FString& ExtensionsString, const int GLESVersion) const override
	{
		return GLESVersion >= 0x30000;
	}

	virtual FText GetVariantDisplayName() const override
	{
		return LOCTEXT("Android_ETC1a_ShortName", "ETC1a");
	}

	virtual float GetVariantPriority() const override
	{
		float Priority;
		return GConfig->GetFloat(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("TextureFormatPriority_ETC1a"), Priority, GEngineIni) ? Priority : 1.0f;
	}
};


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* AndroidTargetSingleton = NULL;


/**
 * Module for the Android target platform.
 */
class FAndroid_ETC1aTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FAndroid_ETC1aTargetPlatformModule( )
	{
		AndroidTargetSingleton = NULL;
	}

public:
	
	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform() override
	{
		if (AndroidTargetSingleton == NULL)
		{
			AndroidTargetSingleton = new FAndroid_ETC1aTargetPlatform();
		}
		
		return AndroidTargetSingleton;
	}

	// End ITargetPlatformModule interface
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FAndroid_ETC1aTargetPlatformModule, Android_ETC1aTargetPlatform);
