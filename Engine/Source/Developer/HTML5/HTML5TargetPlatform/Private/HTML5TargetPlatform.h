// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HTML5TargetPlatform.h: Declares the FHTML5TargetPlatform class.
=============================================================================*/

#pragma once


/**
 * Implements the HTML5 target platform.
 */
class FHTML5TargetPlatform
	: public TTargetPlatformBase<FHTML5PlatformProperties>
{
public:

	/**
	 * Default constructor.
	 */
	FHTML5TargetPlatform( );

public:

	// Begin ITargetPlatform interface

	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const OVERRIDE;

	virtual ECompressionFlags GetBaseCompressionMethod( ) const OVERRIDE;

	virtual bool GetBuildArtifacts( const FString& ProjectPath, EBuildTargets::Type BuildTarget, EBuildConfigurations::Type BuildConfiguration, ETargetPlatformBuildArtifacts::Type Artifacts, TMap<FString, FString>& OutFile, TArray<FString>& OutMissingFiless ) const;

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

	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings() const OVERRIDE;

	virtual void GetTextureFormats( const UTexture* InTexture, TArray<FName>& OutFormats ) const OVERRIDE;

	virtual void GetReflectionCaptureFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		OutFormats.Add(FName(TEXT("EncodedHDR")));
	}

	virtual const struct FTextureLODSettings& GetTextureLODSettings( ) const OVERRIDE;

	virtual FName GetWaveFormat( class USoundWave* Wave ) const OVERRIDE;
#endif // WITH_ENGINE

	DECLARE_DERIVED_EVENT(FHTML5TargetPlatform, ITargetPlatform::FOnTargetDeviceDiscovered, FOnTargetDeviceDiscovered);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) OVERRIDE
	{
		return DeviceDiscoveredEvent;
	}

	DECLARE_DERIVED_EVENT(FHTML5TargetPlatform, ITargetPlatform::FOnTargetDeviceLost, FOnTargetDeviceLost);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) OVERRIDE
	{
		return DeviceLostEvent;
	}

	// End ITargetPlatform interface

private:

	// Holds the HTML5 engine settings.
	FConfigFile HTML5EngineSettings;

	// Holds the local device.
	FHTML5TargetDevicePtr LocalDevice;

#if WITH_ENGINE
	// Holds the cached target LOD settings.
	FTextureLODSettings HTML5LODSettings;

	// Holds the static mesh LOD settings.
	FStaticMeshLODSettings StaticMeshLODSettings;
#endif

private:

	// Holds an event delegate that is executed when a new target device has been discovered.
	FOnTargetDeviceDiscovered DeviceDiscoveredEvent;

	// Holds an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	FOnTargetDeviceLost DeviceLostEvent;
};
