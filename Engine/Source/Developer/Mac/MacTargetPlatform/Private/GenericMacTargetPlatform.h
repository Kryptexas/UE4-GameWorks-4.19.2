// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericMacTargetPlatform.h: Declares the TGenericMacTargetPlatform class template.
=============================================================================*/

#pragma once


/**
 * Template for Mac target platforms
 */
template<bool HAS_EDITOR_DATA, bool IS_DEDICATED_SERVER, bool IS_CLIENT_ONLY>
class TGenericMacTargetPlatform
	: public TTargetPlatformBase<FMacPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY> >
{
public:

	typedef TTargetPlatformBase<FMacPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY> > TSuper;

	/**
	 * Default constructor.
	 */
	TGenericMacTargetPlatform( )
	{
		LocalDevice = MakeShareable(new FLocalMacTargetDevice(*this));

		#if WITH_ENGINE
			FConfigCacheIni::LoadLocalIniFile(EngineSettings, TEXT("Engine"), true, *this->PlatformName());
			TextureLODSettings.Initialize(EngineSettings, TEXT("SystemSettings"));
			StaticMeshLODSettings.Initialize(EngineSettings);
		#endif
	}

public:

	// Begin ITargetPlatform interface

	virtual void EnableDeviceCheck(bool OnOff) OVERRIDE {}

	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const OVERRIDE
	{
		OutDevices.Reset();
		OutDevices.Add(LocalDevice);
	}

	virtual ECompressionFlags GetBaseCompressionMethod( ) const OVERRIDE
	{
		return COMPRESS_ZLIB;
	}

	virtual bool GenerateStreamingInstallManifest(const TMultiMap<FString, int32>& ChunkMap, const TSet<int32>& ChunkIDsInUse) const OVERRIDE
	{
		return true;
	}

	virtual ITargetDevicePtr GetDefaultDevice( ) const OVERRIDE
	{
		return LocalDevice;
	}

	virtual ITargetDevicePtr GetDevice( const FTargetDeviceId& DeviceId )
	{
		if (LocalDevice.IsValid() && (DeviceId == LocalDevice->GetId()))
		{
			return LocalDevice;
		}

		return NULL;
	}

	virtual FString GetIconPath( ETargetPlatformIcons::IconType IconType ) const OVERRIDE
	{
		if (IS_DEDICATED_SERVER)
		{
			switch (IconType)
			{
			case ETargetPlatformIcons::Normal:
//				return FString(TEXT("Launcher/Mac/Platform_MacServer_24x"));
				return FString(TEXT("Launcher/Mac/Platform_Mac_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
//				return FString(TEXT("Launcher/Mac/Platform_MacServer_128x"));
				return FString(TEXT("Launcher/Mac/Platform_Mac_128x"));
			}
		}
		else if (UE_GAME)
		{
			switch (IconType)
			{
			case ETargetPlatformIcons::Normal:
//				return FString(TEXT("Launcher/Mac/Platform_MacNoEditor_24x"));
				return FString(TEXT("Launcher/Mac/Platform_Mac_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
//				return FString(TEXT("Launcher/Mac/Platform_MacNoEditor_128x"));
				return FString(TEXT("Launcher/Mac/Platform_Mac_128x"));
			}
		}
		else
		{
			switch (IconType)
			{
			case ETargetPlatformIcons::Normal:
				return FString(TEXT("Launcher/Mac/Platform_Mac_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
				return FString(TEXT("Launcher/Mac/Platform_Mac_128x"));
			}
		}

		return FString(TEXT(""));
	}

	virtual bool IsRunningPlatform( ) const OVERRIDE
	{
		// Must be Mac platform as editor for this to be considered a running platform
		return PLATFORM_MAC && !UE_SERVER && !UE_GAME && WITH_EDITOR && HAS_EDITOR_DATA;
	}

	virtual bool SupportsFeature( ETargetPlatformFeatures::Type Feature ) const OVERRIDE
	{
		// we currently do not have a build target for MacServer
		if (Feature == ETargetPlatformFeatures::Packaging)
		{
			return (HAS_EDITOR_DATA || !IS_DEDICATED_SERVER);
		}

return TSuper::SupportsFeature(Feature);
	}

#if WITH_ENGINE
	virtual void GetShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		// no shaders needed for dedicated server target
		if (!IS_DEDICATED_SERVER)
		{
			static FName NAME_GLSL_150_MAC(TEXT("GLSL_150_MAC"));
			OutFormats.AddUnique(NAME_GLSL_150_MAC);
		}
	}


	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings( ) const OVERRIDE
	{
		return StaticMeshLODSettings;
	}


	virtual void GetTextureFormats( const UTexture* InTexture, TArray<FName>& OutFormats ) const OVERRIDE
	{
		if (IS_DEDICATED_SERVER)
		{
			// no textures needed for dedicated server target
			OutFormats.Add(NAME_None);
		}
		else
		{
			// just use the standard texture format name for this texture
			OutFormats.Add(this->GetDefaultTextureFormatName(InTexture, EngineSettings));
		}
	}


	virtual const struct FTextureLODSettings& GetTextureLODSettings( ) const OVERRIDE
	{
		return TextureLODSettings;
	}


	virtual FName GetWaveFormat( class USoundWave* Wave ) const OVERRIDE
	{
		static FName NAME_OGG(TEXT("OGG"));

		return NAME_OGG;
	}
#endif //WITH_ENGINE

	DECLARE_DERIVED_EVENT(TGenericMacTargetPlatform, ITargetPlatform::FOnTargetDeviceDiscovered, FOnTargetDeviceDiscovered);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) OVERRIDE
	{
		return DeviceDiscoveredEvent;
	}

	DECLARE_DERIVED_EVENT(TGenericMacTargetPlatform, ITargetPlatform::FOnTargetDeviceLost, FOnTargetDeviceLost);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) OVERRIDE
	{
		return DeviceLostEvent;
	}

	// End ITargetPlatform interface

private:

	// Holds the local device.
	ITargetDevicePtr LocalDevice;

#if WITH_ENGINE
	// Holds the Engine INI settings for quick use.
	FConfigFile EngineSettings;

	// Holds the texture LOD settings.
	FTextureLODSettings TextureLODSettings;

	// Holds the static mesh LOD settings.
	FStaticMeshLODSettings StaticMeshLODSettings;
#endif // WITH_ENGINE

private:

	// Holds an event delegate that is executed when a new target device has been discovered.
	FOnTargetDeviceDiscovered DeviceDiscoveredEvent;

	// Holds an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	FOnTargetDeviceLost DeviceLostEvent;
};
