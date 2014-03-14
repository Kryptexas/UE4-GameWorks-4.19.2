// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxTargetPlatform.h: Declares the FLinuxTargetPlatform class.
=============================================================================*/

#pragma once


/**
 * Template for Linux target platforms
 */
class FLinuxTargetPlatform
	: public TTargetPlatformBase<FLinuxPlatformProperties<true> >
{
public:

	/**
	 * Default constructor.
	 */
	FLinuxTargetPlatform( );

public:

	// Begin ITargetPlatform interface

	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const OVERRIDE;

	virtual ECompressionFlags GetBaseCompressionMethod( ) const OVERRIDE;

	virtual bool GetBuildArtifacts( const FString& ProjectPath, EBuildTargets::Type BuildTarget, EBuildConfigurations::Type BuildConfiguration, ETargetPlatformBuildArtifacts::Type Artifacts, TMap<FString, FString>& OutFiles, TArray<FString>& OutMissingFiles ) const OVERRIDE;

	virtual bool GenerateStreamingInstallManifest(const TMultiMap<FString, int32>& ChunkMap, const TSet<int32>& ChunkIDsInUse) const OVERRIDE
	{
		return true;
	}

	virtual ITargetDevicePtr GetDefaultDevice( ) const OVERRIDE;

	virtual ITargetDevicePtr GetDevice( const FTargetDeviceId& DeviceId ) OVERRIDE;

	virtual FString GetIconPath( ETargetPlatformIcons::IconType IconType ) const OVERRIDE;

	virtual bool IsRunningPlatform( ) const OVERRIDE;

#if WITH_ENGINE
	virtual void GetShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE;

	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings( ) const OVERRIDE;

	virtual void GetTextureFormats( const UTexture* InTexture, TArray<FName>& OutFormats ) const OVERRIDE;

	virtual const struct FTextureLODSettings& GetTextureLODSettings( ) const OVERRIDE;

	virtual FName GetWaveFormat( class USoundWave* Wave ) const OVERRIDE;
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
