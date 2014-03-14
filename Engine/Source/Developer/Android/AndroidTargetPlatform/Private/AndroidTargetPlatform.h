// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidTargetPlatform.h: Declares the FAndroidTargetPlatform class.
=============================================================================*/

#pragma once

#include "Ticker.h"


/**
 * Defines supported texture format names.
 */
namespace AndroidTexFormat
{
	// Compressed Texture Formats
	static FName NamePVRTC2(TEXT("PVRTC2"));
	static FName NamePVRTC4(TEXT("PVRTC4"));
	static FName NameAutoPVRTC(TEXT("AutoPVRTC"));
	static FName NameDXT1(TEXT("DXT1"));
	static FName NameDXT5(TEXT("DXT5"));
	static FName NameAutoDXT(TEXT("AutoDXT"));
	static FName NameATC_RGB(TEXT("ATC_RGB"));
	static FName NameATC_RGBA_E(TEXT("ATC_RGBA_E"));		// explicit alpha
	static FName NameATC_RGBA_I(TEXT("ATC_RGBA_I"));		// interpolated alpha
	static FName NameAutoATC(TEXT("AutoATC"));
	static FName NameETC1(TEXT("ETC1"));
	static FName NameAutoETC1(TEXT("AutoETC1"));			// ETC1 or uncompressed RGBA, if alpha channel required
	static FName NameETC2_RGB(TEXT("ETC2_RGB"));
	static FName NameETC2_RGBA(TEXT("ETC2_RGBA"));
	static FName NameAutoETC2(TEXT("AutoETC2"));

	// Uncompressed Texture Formats
	static FName NameBGRA8(TEXT("BGRA8"));
	static FName NameG8(TEXT("G8"));
	static FName NameVU8(TEXT("VU8"));
	static FName NameRGBA16F(TEXT("RGBA16F"));
}


/**
 * FAndroidTargetPlatform, abstraction for cooking Android platforms
 */
class FAndroidTargetPlatform
	: public TTargetPlatformBase< FAndroidPlatformProperties >
{
public:

	/**
	 * Default constructor.
	 */
	FAndroidTargetPlatform( );

	/**
	 * Destructor
	 */
	~FAndroidTargetPlatform();

public:

	/**
	 * Gets the name of the Android platform variant, i.e. ATC, DXT or PVRTC.
	 *
	 * @param Variant name.
	 */
	virtual FString GetAndroidVariantName( )
	{
		return FString();	
	}

public:

	// Begin ITargetPlatform interface

	virtual bool AddDevice( const FString& DeviceName, bool bDefault ) OVERRIDE
	{
		return false;
	}

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

	virtual bool IsServerOnly( ) const OVERRIDE
	{
		return false;
	}

	virtual bool SupportsFeature( ETargetPlatformFeatures::Type Feature ) const OVERRIDE
	{
		if (Feature == ETargetPlatformFeatures::Packaging)
		{
			// not implemented yet
			return true;
		}

		return TTargetPlatformBase< FAndroidPlatformProperties >::SupportsFeature(Feature);
	}

	virtual bool IsSdkInstalled(bool bProjectHasCode, FString& OutDocumentationPath) const OVERRIDE;

#if WITH_ENGINE
	virtual void GetReflectionCaptureFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		OutFormats.Add(FName(TEXT("EncodedHDR")));
	}

	virtual void GetShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE;

	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings() const OVERRIDE;

	virtual void GetTextureFormats( const UTexture* InTexture, TArray<FName>& OutFormats ) const OVERRIDE;

	virtual const struct FTextureLODSettings& GetTextureLODSettings( ) const OVERRIDE;

	virtual FName GetWaveFormat( class USoundWave* Wave ) const OVERRIDE;
#endif //WITH_ENGINE

	virtual bool SupportsTextureFormat( FName Format ) const 
	{
		// By default we support all texture formats.
		return true;
	}

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

protected:

	/**
	 * Adds the specified texture format to the OutFormats if this android target platforms supports it.
	 *
	 * @param Format - The format to add.
	 * @param OutFormats - The collection of formats to add to.
	 */
	void AddTextureFormatIfSupports( FName Format, TArray<FName>& OutFormats ) const;

	/**
	 * Executes an SDK command with the specified command line.
	 *
	 * @param Params - The command line parameters.
	 * @param OutStdOut - Optional pointer to a string that will hold the command's output log.
	 * @param OutStdErr - Optional pointer to a string that will hold the error message, if any.
	 *
	 * @return true on success, false otherwise.
	 */
	bool ExecuteAdbCommand( const FString& Params, FString* OutStdOut, FString* OutStdErr ) const;

	/**
	 * Gathers a list of all devices that are currently connected.
	 */
	void QueryConnectedDevices( );

private:

	// Handles when the ticker fires.
	bool HandleTicker( float DeltaTime );

	// Handles received pong messages from the LauncherDaemon.
//	void HandlePongMessage( const FAndroidPong &Message, const IMessageContextRef &Context );

private:

	// Holds a map of valid devices.
	TMap<FString, FAndroidTargetDevicePtr> Devices;

	// Holds a delegate to be invoked when the widget ticks.
	FTickerDelegate TickDelegate;

#if WITH_ENGINE
	// Holds the Engine INI settings (for quick access).
	FConfigFile EngineSettings;

	// Holds a cache of the target LOD settings.
	FTextureLODSettings TextureLODSettings;

	// Holds the static mesh LOD settings.
	FStaticMeshLODSettings StaticMeshLODSettings;

	ITargetDevicePtr DefaultDevice;
#endif //WITH_ENGINE

private:

	// Holds an event delegate that is executed when a new target device has been discovered.
	FOnTargetDeviceDiscovered DeviceDiscoveredEvent;

	// Holds an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	FOnTargetDeviceLost DeviceLostEvent;
};


#include "AndroidTargetPlatform.inl"
