// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITargetPlatformManagerModule.h: Declares the ITargetPlatformManagerModule interface.
=============================================================================*/

#pragma once


/**
 * Module for the target platform manager
 */
class ITargetPlatformManagerModule
	: public IModuleInterface
{
public:

	/** 
	 * Invalidate should be called if any TargetPlatform modules get loaded/unloaded/reloaded during 
	 * runtime to give the implementation the chance to rebuild all its internal states and caches.
	 **/
	virtual void Invalidate() = 0;

	/**
	 * Finds an audio format with the specified name.
	 *
	 * @param Name - Name of the format to find.
	 *
	 * @return The audio format, or NULL if not found.
	 */
	virtual const class IAudioFormat* FindAudioFormat(FName Name) = 0;

	/**
	 * Finds a PhysX format with the specified name.
	 *
	 * @param Name - Name of the format to find.
	 *
	 * @return The PhysX format, or NULL if not found.
	 */
	virtual const class IPhysXFormat* FindPhysXFormat(FName Name) = 0;

	/**
	 * Finds a shader format with the specified name.
	 *
	 * @param Name - Name of the format to find.
	 *
	 * @return The shader format, or NULL if not found.
	 */
	virtual const class IShaderFormat* FindShaderFormat(FName Name) = 0;

	/**
	 * Finds a texture format with the specified name.
	 *
	 * @param Name - Name of the format to find.
	 *
	 * @return The texture format, or NULL if not found.
	 */
	virtual const class ITextureFormat* FindTextureFormat(FName Name) = 0;

	/**
	 * Finds a target device by identifier.
	 *
	 * @param DeviceId - The device identifier.
	 *
	 * @return The target device, or NULL if not found.
	 */
	virtual ITargetDevicePtr FindTargetDevice(const FTargetDeviceId& DeviceId) = 0;

	/**
	 * Finds a target platform by name.
	 *
	 * @param Name - The target platform's short or long name.
	 *
	 * @return The target platform, or NULL if not found.
	 */
	virtual ITargetPlatform* FindTargetPlatform(FString Name) = 0;

	/**
	 * Return the list of the ITargetPlatforms that we want to build data for.
	 *
	 * @return Collection of platforms.
	 */
	virtual const TArray<ITargetPlatform*>& GetActiveTargetPlatforms() = 0;

	/**
	 * Returns the list of all IAudioFormats that were located in DLLs.
	 *
	 * @return Collection of audio formats.
	 */
	virtual const TArray<const class IAudioFormat*>& GetAudioFormats() = 0;

	/**
	 * Returns the list of all IPhysXFormats that were located in DLLs.
	 *
	 * @return Collection of PhysX formats.
	 */
	virtual const TArray<const class IPhysXFormat*>& GetPhysXFormats() = 0;

	/**
	 * Returns the target platform that is currently running.
	 *
	 * Note: This method is not available on consoles.
	 *
	 * @return Running target platform.
	 */
	virtual ITargetPlatform* GetRunningTargetPlatform() = 0;

	/**
	 * Returns the list of all ITextureFormats that were located in DLLs.
	 *
	 * @return Collection of shader formats.
	 */
	virtual const TArray<const class IShaderFormat*>& GetShaderFormats() = 0;

	/**
	 * Returns the list of all ITargetPlatforms that were located in DLLs.
	 *
	 * @return Collection of platforms.
	 */
	virtual const TArray<ITargetPlatform*>& GetTargetPlatforms() = 0;

	/**
	 * Returns the list of all ITextureFormats that were located in DLLs.
	 *
	 * @return Collection of texture formats.
	 */
	virtual const TArray<const class ITextureFormat*>& GetTextureFormats() = 0;
	
	/**
	 * Checks whether we should only build formats that are actually required for use by the runtime.
	 *
	 * @return true if formats are restricted, false otherwise.
	 */
	virtual bool RestrictFormatsToRuntimeOnly() = 0;

	/**
	 * Gets the shader format version for the specified shader.
	 *
	 * @param Name - Name of the shader format to get the version for.
	 *
	 * @return Version number.
	 */
	virtual uint16 ShaderFormatVersion(FName Name) = 0;
};
