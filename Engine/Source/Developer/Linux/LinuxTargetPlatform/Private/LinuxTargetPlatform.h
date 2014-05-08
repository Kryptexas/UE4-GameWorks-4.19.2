// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxTargetPlatform.h: Declares the FLinuxTargetPlatform class.
=============================================================================*/

#pragma once


/**
 * Template for Linux target platforms
 */
template<bool HAS_EDITOR_DATA, bool IS_DEDICATED_SERVER, bool IS_CLIENT_ONLY>
class TLinuxTargetPlatform
	: public TTargetPlatformBase<FLinuxPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY> >
{
public:

	typedef public TTargetPlatformBase<FLinuxPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY> > TSuper;

	/**
	 * Default constructor.
	 */
	TLinuxTargetPlatform( )
	{		
#if PLATFORM_LINUX
		// only add local device if actually running on Linux
		LocalDevice = MakeShareable(new FLinuxTargetDevice(*this, FTargetDeviceId(PlatformName(), FPlatformProcess::ComputerName())));
#endif
	
		#if WITH_ENGINE
			FConfigCacheIni::LoadLocalIniFile(EngineSettings, TEXT("Engine"), true, *PlatformName());
			TextureLODSettings.Initialize(EngineSettings, TEXT("SystemSettings"));
			StaticMeshLODSettings.Initialize(EngineSettings);
		#endif
	}


public:

	// Begin ITargetPlatform interface

	virtual void EnableDeviceCheck(bool OnOff) OVERRIDE {}

	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const OVERRIDE
	{
		// TODO: ping all the machines in a local segment and/or try to connect to port 22 of those that respond
		OutDevices.Reset();
		if (LocalDevice.IsValid())
		{
			OutDevices.Add(LocalDevice);
		}
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
		if (LocalDevice.IsValid())
		{
			return LocalDevice;
		}

		return nullptr;
	}

	virtual ITargetDevicePtr GetDevice( const FTargetDeviceId& DeviceId ) OVERRIDE
	{
		if (LocalDevice.IsValid() && (DeviceId == LocalDevice->GetId()))
		{
			return LocalDevice;
		}

		FTargetDeviceId UATFriendlyId(TEXT("linux"), DeviceId.GetDeviceName());
		return MakeShareable(new FLinuxTargetDevice(*this, UATFriendlyId, DeviceId.GetDeviceName()));
	}

	virtual FString GetIconPath( ETargetPlatformIcons::IconType IconType ) const OVERRIDE
	{
		switch (IconType)
		{
		case ETargetPlatformIcons::Normal:
			return FString(TEXT("Launcher/Linux/Platform_Linux_24x"));

		case ETargetPlatformIcons::Large:
		case ETargetPlatformIcons::XLarge:
			return FString(TEXT("Launcher/Linux/Platform_Linux_128x"));
		}

		return FString();
	}

	virtual bool IsRunningPlatform( ) const OVERRIDE
	{
		return false;
	}

	bool SupportsFeature(ETargetPlatformFeatures::Type Feature) const OVERRIDE
	{
		if (Feature == ETargetPlatformFeatures::UserCredentials || Feature == ETargetPlatformFeatures::Packaging)
		{
			return true;
		}

		return TTargetPlatformBase<FLinuxPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY>>::SupportsFeature(Feature);
	}

#if WITH_ENGINE
	virtual void GetShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		// no shaders needed for dedicated server target
		if (!IS_DEDICATED_SERVER)
		{
			static FName NAME_GLSL_150(TEXT("GLSL_150"));
			static FName NAME_GLSL_430(TEXT("GLSL_430"));

			OutFormats.AddUnique(NAME_GLSL_150);
			OutFormats.AddUnique(NAME_GLSL_430);
		}
	}

	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings( ) const OVERRIDE
	{
		return StaticMeshLODSettings;
	}

	virtual void GetTextureFormats( const UTexture* InTexture, TArray<FName>& OutFormats ) const OVERRIDE
	{
		if (!IS_DEDICATED_SERVER)
		{
			// just use the standard texture format name for this texture
			OutFormats.Add(GetDefaultTextureFormatName(InTexture, EngineSettings));
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

	DECLARE_DERIVED_EVENT(FAndroidTargetPlatform, ITargetPlatform::FOnTargetDeviceDiscovered, FOnTargetDeviceDiscovered);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) OVERRIDE
	{
		return DeviceDiscoveredEvent;
	}

	DECLARE_DERIVED_EVENT(FAndroidTargetPlatform, ITargetPlatform::FOnTargetDeviceLost, FOnTargetDeviceLost);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) OVERRIDE
	{
		return DeviceLostEvent;
	}

	// End ITargetPlatform interface

private:

	// Holds the local device.
	FLinuxTargetDevicePtr LocalDevice;

#if WITH_ENGINE
	// Holds the Engine INI settings for quick use.
	FConfigFile EngineSettings;

	// Holds the texture LOD settings.
	FTextureLODSettings TextureLODSettings;

	// Holds static mesh LOD settings.
	FStaticMeshLODSettings StaticMeshLODSettings;
#endif // WITH_ENGINE

private:

	// Holds an event delegate that is executed when a new target device has been discovered.
	FOnTargetDeviceDiscovered DeviceDiscoveredEvent;

	// Holds an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	FOnTargetDeviceLost DeviceLostEvent;
};
