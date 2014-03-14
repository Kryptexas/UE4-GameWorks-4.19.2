// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IOSTargetPlatform.h: Declares the FIOSTargetPlatform class.
=============================================================================*/

#pragma once

#include "Ticker.h"


/**
 * FIOSTargetPlatform, abstraction for cooking iOS platforms
 */
class FIOSTargetPlatform
	: public TTargetPlatformBase<FIOSPlatformProperties>
{
public:

	/**
	 * Default constructor.
	 */
	FIOSTargetPlatform();

	/**
	 * Destructor.
	 */
	~FIOSTargetPlatform();

public:

	// Begin TTargetPlatformBase interface

	virtual bool IsServerOnly( ) const OVERRIDE
	{
		return false;
	}

	// End TTargetPlatformBase interface

public:

	// Begin ITargetPlatform interface

	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const OVERRIDE;

	virtual ECompressionFlags GetBaseCompressionMethod() const OVERRIDE
	{
		return COMPRESS_ZLIB;
	}

	virtual bool GetBuildArtifacts( const FString& ProjectPath, EBuildTargets::Type BuildTarget, EBuildConfigurations::Type BuildConfiguration, ETargetPlatformBuildArtifacts::Type Artifacts, TMap<FString, FString>& OutFiles, TArray<FString>& OutMissingFiles ) const OVERRIDE;

	virtual bool GenerateStreamingInstallManifest(const TMultiMap<FString, int32>& ChunkMap, const TSet<int32>& ChunkIDsInUse) const OVERRIDE
	{
		return true;
	}

	virtual ITargetDevicePtr GetDefaultDevice( ) const OVERRIDE;

	virtual ITargetDevicePtr GetDevice( const FTargetDeviceId& DeviceId ) OVERRIDE;

	virtual FString GetIconPath( ETargetPlatformIcons::IconType IconType ) const OVERRIDE;

	virtual bool IsRunningPlatform( ) const OVERRIDE
	{
		#if PLATFORM_IOS && WITH_EDITOR
			return true;
		#else
			return false;
		#endif
	}

	virtual bool SupportsFeature( ETargetPlatformFeatures::Type Feature ) const OVERRIDE
	{
		if (Feature == ETargetPlatformFeatures::Packaging)
		{
			// not implemented yet
			return true;
		}

		return TTargetPlatformBase<FIOSPlatformProperties>::SupportsFeature(Feature);
	}


#if WITH_ENGINE
	virtual void GetReflectionCaptureFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		OutFormats.Add(FName(TEXT("EncodedHDR")));
	}

	virtual void GetShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE;

	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings( ) const OVERRIDE
	{
		return StaticMeshLODSettings;
	}

	virtual void GetTextureFormats( const UTexture* Texture, TArray<FName>& OutFormats ) const OVERRIDE;

	virtual const struct FTextureLODSettings& GetTextureLODSettings( ) const OVERRIDE;

	virtual FName GetWaveFormat( class USoundWave* Wave ) const OVERRIDE;
#endif // WITH_ENGINE


	DECLARE_DERIVED_EVENT(FIOSTargetPlatform, ITargetPlatform::FOnTargetDeviceDiscovered, FOnTargetDeviceDiscovered);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) OVERRIDE
	{
		return DeviceDiscoveredEvent;
	}

	DECLARE_DERIVED_EVENT(FIOSTargetPlatform, ITargetPlatform::FOnTargetDeviceLost, FOnTargetDeviceLost);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) OVERRIDE
	{
		return DeviceLostEvent;
	}

	// Begin ITargetPlatform interface

protected:

	/**
	 * Sends a ping message over the network to find devices running the launch daemon.
	 */
	void PingNetworkDevices( );

private:

	// Handles when the ticker fires.
	bool HandleTicker( float DeltaTime );

	// Handles received pong messages from the LauncherDaemon.
	void HandlePongMessage( const FIOSLaunchDaemonPong& Message, const IMessageContextRef& Context );

    void HandleDeviceConnected( const FIOSLaunchDaemonPong& Message );
    void HandleDeviceDisconnected( const FIOSLaunchDaemonPong& Message );

private:

	// Contains all discovered IOSTargetDevices over the network.
	TMap<FTargetDeviceId, FIOSTargetDevicePtr> Devices;

	// Holds a delegate to be invoked when the widget ticks.
	FTickerDelegate TickDelegate;

	// Holds the message endpoint used for communicating with the LaunchDaemon.
	FMessageEndpointPtr MessageEndpoint;

#if WITH_ENGINE
	// Holds the Engine INI settings, for quick use.
	FConfigFile EngineSettings;

	// Holds the cache of the target LOD settings.
	FTextureLODSettings TextureLODSettings;

	// Holds the static mesh LOD settings.
	FStaticMeshLODSettings StaticMeshLODSettings;
#endif // WITH_ENGINE

    // holds usb device helper
    FIOSDeviceHelper DeviceHelper;

private:

	// Holds an event delegate that is executed when a new target device has been discovered.
	FOnTargetDeviceDiscovered DeviceDiscoveredEvent;

	// Holds an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	FOnTargetDeviceLost DeviceLostEvent;
};
