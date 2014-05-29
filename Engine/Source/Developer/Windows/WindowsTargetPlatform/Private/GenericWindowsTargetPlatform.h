// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericWindowsTargetPlatform.h: Declares the TGenericWindowsTargetPlatform class template.
=============================================================================*/

#pragma once

#include "StaticMeshResources.h"

/**
 * Template for Windows target platforms
 */
template<bool HAS_EDITOR_DATA, bool IS_DEDICATED_SERVER, bool IS_CLIENT_ONLY>
class TGenericWindowsTargetPlatform
	: public TTargetPlatformBase<FWindowsPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY> >
{
public:

	typedef public TTargetPlatformBase<FWindowsPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY> > TSuper;

	/**
	 * Default constructor.
	 */
	TGenericWindowsTargetPlatform( )
	{
		LocalDevice = MakeShareable(new TLocalPcTargetDevice<HAS_EDITOR_DATA>(*this));
		
		#if WITH_ENGINE
			FConfigCacheIni::LoadLocalIniFile(EngineSettings, TEXT("Engine"), true, *PlatformName());
			TextureLODSettings.Initialize(EngineSettings, TEXT("SystemSettings"));
			StaticMeshLODSettings.Initialize(EngineSettings);

			// Get the Target RHIs for this platform, we do not always want all those that are supported.
			GConfig->GetArray(TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings"), TEXT("TargetedRHIs"), TargetedShaderFormats, GEngineIni);
			
			// Gather the list of Target RHIs and filter out any that may be invalid.
			TArray<FName> PossibleShaderFormats;
			GetAllPossibleShaderFormats(PossibleShaderFormats);

			for(int32 ShaderFormatIdx = TargetedShaderFormats.Num()-1; ShaderFormatIdx >= 0; ShaderFormatIdx--)
			{
				FString ShaderFormat = TargetedShaderFormats[ShaderFormatIdx];
				if(PossibleShaderFormats.Contains(FName(*ShaderFormat)) == false)
				{
					TargetedShaderFormats.Remove(ShaderFormat);
				}
			}
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
				return FString(TEXT("Launcher/Windows/Platform_WindowsServer_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
				return FString(TEXT("Launcher/Windows/Platform_WindowsServer_128x"));
			}
		}
		else if (UE_GAME)
		{
			switch (IconType)
			{
			case ETargetPlatformIcons::Normal:
				return FString(TEXT("Launcher/Windows/Platform_WindowsNoEditor_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
				return FString(TEXT("Launcher/Windows/Platform_WindowsNoEditor_128x"));
			}
		}
		else
		{
			switch (IconType)
			{
			case ETargetPlatformIcons::Normal:
				return FString(TEXT("Launcher/Windows/Platform_Windows_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
				return FString(TEXT("Launcher/Windows/Platform_Windows_128x"));
			}
		}

		return FString();
	}

	virtual bool IsRunningPlatform( ) const OVERRIDE
	{
		// Must be Windows platform as editor for this to be considered a running platform
		return PLATFORM_WINDOWS && !UE_SERVER && !UE_GAME && WITH_EDITOR && HAS_EDITOR_DATA;
	}

	virtual bool SupportsFeature( ETargetPlatformFeatures::Type Feature ) const OVERRIDE
	{
		// we currently do not have a build target for WindowsServer
		if (Feature == ETargetPlatformFeatures::Packaging)
		{
			return (HAS_EDITOR_DATA || !IS_DEDICATED_SERVER);
		}

		return TSuper::SupportsFeature(Feature);
	}

#if WITH_ENGINE
	virtual void GetAllPossibleShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		// no shaders needed for dedicated server target
		if (!IS_DEDICATED_SERVER)
		{
			// Only shader formats needed for cooked builds should be added here.
			// TODO: this should be configurable per-project, e.g. you may or
			//       may not wish to support SM4 or OpenGL.
			static FName NAME_PCD3D_SM5(TEXT("PCD3D_SM5"));
			static FName NAME_PCD3D_SM4(TEXT("PCD3D_SM4"));
			static FName NAME_GLSL_150(TEXT("GLSL_150"));
			static FName NAME_GLSL_430(TEXT("GLSL_430"));

			OutFormats.AddUnique(NAME_PCD3D_SM5);
			OutFormats.AddUnique(NAME_PCD3D_SM4);
			OutFormats.AddUnique(NAME_GLSL_150);
			OutFormats.AddUnique(NAME_GLSL_430);
		}
	}

	virtual void GetAllTargetedShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		for(const FString& ShaderFormat : TargetedShaderFormats)
		{
			OutFormats.AddUnique(FName(*ShaderFormat));
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

	DECLARE_DERIVED_EVENT(TGenericWindowsTargetPlatform, ITargetPlatform::FOnTargetDeviceDiscovered, FOnTargetDeviceDiscovered);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) OVERRIDE
	{
		return DeviceDiscoveredEvent;
	}

	DECLARE_DERIVED_EVENT(TGenericWindowsTargetPlatform, ITargetPlatform::FOnTargetDeviceLost, FOnTargetDeviceLost);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) OVERRIDE
	{
		return DeviceLostEvent;
	}

	// End ITargetPlatform interface

protected:

	/**
	 * Temporary helper until we refactor Windows build targets.
	 * Basically maps WindowsNoEditor to Win32 and Windows to Win64.
	 */
	FString GetBinariesSubDir( ) const
	{
		if (HAS_EDITOR_DATA)
		{
			return TEXT("Win64");
		}

		return TEXT("Win32");
	}

private:

	// Holds the local device.
	ITargetDevicePtr LocalDevice;

#if WITH_ENGINE
	// Holds the Engine INI settings for quick use.
	FConfigFile EngineSettings;

	// Holds the texture LOD settings.
	FTextureLODSettings TextureLODSettings;

	// Holds static mesh LOD settings.
	FStaticMeshLODSettings StaticMeshLODSettings;

	// List of shader formats specified as targets
	TArray<FString> TargetedShaderFormats;
#endif // WITH_ENGINE

private:

	// Holds an event delegate that is executed when a new target device has been discovered.
	FOnTargetDeviceDiscovered DeviceDiscoveredEvent;

	// Holds an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	FOnTargetDeviceLost DeviceLostEvent;
};
