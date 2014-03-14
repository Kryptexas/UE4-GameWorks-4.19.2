// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITargetPlatform.h: Declares the ITargetPlatform interface.
=============================================================================*/

#pragma once


namespace ETargetPlatformBuildArtifacts
{
	/**
	 * Enumerates build artifact types.
	 */
	enum Type
	{
		/**
		 * Include content files.
		 */
		Content = 0x1,

		/**
		 * Include debug symbol files, i.e PDB.
		 *
		 * Requires 'Engine' artifact type.
		 */
		DebugSymbols = 0x2,

		/**
		 * Include Engine binaries and DLLs.
		 */
		Engine = 0x4,

		/**
		 * Include tools.
		 */
		Tools = 0x8,
	};
}


namespace ETargetPlatformFeatures
{
	/**
	 * Enumerates features that may be supported by target platforms.
	 */
	enum Type
	{
		/**
		 * Distance field shadows.
		 */
		DistanceFieldShadows,

		/**
		 * Gray scale SRGB texture formats support.
		 */
		GrayscaleSRGB,

		/**
		 * High quality light maps.
		 */
		HighQualityLightmaps,

		/**
		 * Low quality light maps.
		 */
		LowQualityLightmaps,

		/**
		 * Run multiple game instances on a single device.
		 */
		MultipleGameInstances,

		/**
		 * Builds can be packaged for this platform.
		 */
		Packaging,

		/**
		 * GPU tesselation.
		 */
		Tessellation,

		/**
		 * Texture streaming.
		 */
		 TextureStreaming,

		/**
		 * Vertex Shader Texture Sampling.
		 */
		 VertexShaderTextureSampling,
	};
};


namespace ETargetPlatformIcons
{
	/**
	 * Enumerates platform icon types.
	 */
	enum IconType
	{
		/**
		 * Normal sized icon.
		 */
		Normal,

		/**
		 * Large icon.
		 */
		Large,

		/**
		 * Extra large icon.
		 */
		XLarge
	};
}


/**
 * ITargetPlatform, abstraction for cooking platforms and enumerating actual target devices
**/
class ITargetPlatform
{
public:

	/**
	 * Add a target device by name.
	 *
	 * @param DeviceName - The name of the device to add.
	 * @param bDefault - Whether the added device should be the default.
	 *
	 * @return true if the device was added, false otherwise.
	 */
	virtual bool AddDevice( const FString& DeviceName, bool bDefault ) = 0;

	/**
	 * Gets the platform's display name.
	 *
	 * @see PlatformName
	 */
	virtual FText DisplayName( ) const = 0;

	/**
	 * Gets the platform's ini name (so an offline tool can load the ini for the given target platform)
	 *
	 * @see PlatformName
	 */
	virtual FString IniPlatformName( ) const = 0;

	/**
	 * Returns all discoverable physical devices.
	 *
	 * @param OutDevices - Will contain a list of discovered devices.
	 */
	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const = 0;

	/**
	 * Gets the best generic data compressor for this platform.
	 *
	 * @return Compression method.
	 */
	virtual ECompressionFlags GetBaseCompressionMethod( ) const = 0;

	/**
	 * Gets the paths of the binaries that need to be deployed for the specified game and build configuration.
	 *
	 * The returned collection maps the path of local build artifacts (including the artifact's file name)
	 * to a deployment folder path on a target device. Both paths are relative to the Root.
	 *
	 * @param ProjectPath - The path to the Unreal project file.
	 * @param BuildTarget - The build target to get the artifacts for, i.e Game or Server.
	 * @param BuildConfiguration - The build configuration to get the artifacts for, i.e. Debug or Shipping.
	 * @param Artifacts - The type of build artifacts to include.
	 * @param OutFiles - Will hold the collection of artifacts.
	 * @param OutMissingFiles - Will hold the paths to missing artifacts.
	 *
	 * @return true if all required artifacts were gathered, false otherwise.
	 */
	virtual bool GetBuildArtifacts( const FString& ProjectPath, EBuildTargets::Type BuildTarget, EBuildConfigurations::Type BuildConfiguration, ETargetPlatformBuildArtifacts::Type Artifacts, TMap<FString, FString>& OutFiles, TArray<FString>& OutMissingFiles ) const = 0;

	/** 
	 * Generates a platform specific asset manifest given an array of FAssetData.
	 *
	 * @param ChunkMap - A map of asset path to ChunkIDs for all of the assets
	 * @param ChunkIDsInUse - A set of all ChunkIDs used by this set of assets
	 *
	 * @return true if the manifest was successfully generated, or if the platform doesn't need a manifest 
	 */
	virtual bool GenerateStreamingInstallManifest( const TMultiMap<FString, int32>& ChunkMap, const TSet<int32>& ChunkIDsInUse ) const = 0;

	/**
	 * Gets the default device.
	 *
	 * Note that not all platforms may have a notion of default devices.
	 *
	 * @return Default device.
	 */
	virtual ITargetDevicePtr GetDefaultDevice( ) const = 0;

	/** 
	 * Gets an interface to the specified device.
	 *
	 * @param DeviceId - The identifier of the device to get.
	 *
	 * @return The target device (can be NULL). 
	 */
	virtual ITargetDevicePtr GetDevice( const FTargetDeviceId& DeviceId ) = 0;

	/**
	 * Gets the path to the platform's icon.
	 *
	 * @param IconType - The type of icon to get (i.e. Small or Large).
	 *
	 * @return The path to the icon.
	 */
	virtual FString GetIconPath( ETargetPlatformIcons::IconType IconType ) const = 0;

	/**
	 * Checks whether this platform has only editor data (typically desktop platforms).
	 *
	 * @return true if this platform has editor only data, false otherwise.
	 */
	virtual bool HasEditorOnlyData( ) const = 0;

	/**
	 * Checks whether this platform is little endian.
	 *
	 * @return true if this platform is little-endian, false otherwise.
	 */
	virtual bool IsLittleEndian( ) const = 0;

	/**
	 * Checks whether this platform is the platform that's currently running.
	 *
	 * For example, when running on Windows, the Windows ITargetPlatform will return true
	 * and all other platforms will return false.
	 *
	 * @return true if this platform is running, false otherwise.
	 */
	virtual bool IsRunningPlatform( ) const = 0;

	/**
	 * Checks whether this platform is only a server.
	 *
	 * @return true if this platform has no graphics or audio, etc, false otherwise.
	 */
	virtual bool IsServerOnly( ) const = 0;

	/**
	 * Checks whether this platform is only a client (and must connect to a server to run).
	 *
	 * @return true if this platform must connect to a server.
	 */
	virtual bool IsClientOnly( ) const = 0;

	/**
	 * Returns the maximum bones the platform supports.
	 *
	 * @return the maximum bones the platform supports.
	 */
	virtual uint32 MaxGpuSkinBones( ) const = 0;

	/**
	 * Returns the name of this platform
	 *
	 * @return Platform name.
	 *
	 * @see DisplayName
	 */
	virtual FString PlatformName( ) const = 0;

	/**
	 * Checks whether this platform requires cooked data (typically console platforms).
	 *
	 * @return true if this platform requires cooked data, false otherwise.
	 */
	virtual bool RequiresCookedData( ) const = 0;

	/**
	 * Checks whether this platform requires user credentials (typically server platforms).
	 *
	 * @return true if this platform requires user credentials, false otherwise.
	 */
	virtual bool RequiresUserCredentials() const = 0;

	/**
	 * Checks whether this platform supports the specified build target, i.e. Game or Editor.
	 *
	 * @param BuildTarget - The build target to check.
	 *
	 * @return true if the build target is supported, false otherwise.
	 */
	virtual bool SupportsBuildTarget( EBuildTargets::Type BuildTarget ) const = 0;

	/**
	 * Checks whether the target platform supports the specified feature.
	 *
	 * @param Feature - The feature to check.
	 *
	 * @return true if the feature is supported, false otherwise.
	 */
	virtual bool SupportsFeature( ETargetPlatformFeatures::Type Feature ) const = 0;


	/**
	 * Checks whether the platform's SDK requirements are met so that we can do things like
	 * package for the platform
	 *
	 * @param bProjectHasCode - true if the project has code, and therefore any compilation based SDK requirements should be checked
	 * @param OutDocumentationPath - Let's the platform tell the editor a path to show some documentation about how to set up the SDK
	 *
	 * @return true if the platform is ready for use
	 */
	virtual bool IsSdkInstalled(bool bProjectHasCode, FString& OutDocumentationPath) const = 0;

#if WITH_ENGINE
	/**
	 * Gets the format to use for a particular body setup.
	 *
	 * @return Physics format.
	 */
	virtual FName GetPhysicsFormat( class UBodySetup* Body ) const = 0;

	/**
	 * Gets the reflection capture formats this platform needs.
	 *
	 * @param OutFormats - Will contain the collection of formats.
	 */
	virtual void GetReflectionCaptureFormats( TArray<FName>& OutFormats ) const = 0;

	/**
	 * Gets the shader formats this platform uses.
	 *
	 * @param OutFormats - Will contain the shader formats.
	 */
	virtual void GetShaderFormats( TArray<FName>& OutFormats ) const = 0;

	/**
	 * Gets the format to use for a particular texture.
	 *
	 * @param Texture - The texture to get the format for.
	 * @param OutFormats - Will contain the list of supported formats.
	 */
	virtual void GetTextureFormats( const class UTexture* Texture, TArray<FName>& OutFormats ) const = 0;

	/**
	 * Gets the format to use for a particular piece of audio.
	 *
	 * @param Wave - The sound node wave to get the format for.
	 *
	 * @return Name of the wave format.
	 */
	virtual FName GetWaveFormat( class USoundWave* Wave ) const = 0;

	/**
	 * Gets the texture LOD settings used by this platform.
	 *
	 * @return A texture LOD settings structure.
	 */
	virtual const struct FTextureLODSettings& GetTextureLODSettings( ) const = 0;

	/**
	 * Gets the static mesh LOD settings used by this platform.
	 *
	 * @return A static mesh LOD settings structure.
	 */
	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings() const = 0;
#endif

	/** 
	 * Package a build for the given platform 
	 * 
	 * @param InPackgeDirectory The directory that contains what needs to be packaged 
	 * 
	 * @return bool True if successful, false if failed 
	 */ 
	virtual bool PackageBuild(const FString& InPackgeDirectory) = 0;

public:

	/**
	 * Gets an event delegate that is executed when a new target device has been discovered.
	 */
	DECLARE_EVENT_OneParam(ITargetPlatform, FOnTargetDeviceDiscovered, const ITargetDeviceRef& /*DiscoveredDevice*/);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) = 0;

	/**
	 * Gets an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	 */
	DECLARE_EVENT_OneParam(ITargetPlatform, FOnTargetDeviceLost, const ITargetDeviceRef& /*LostDevice*/);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ITargetPlatform() { }
};
