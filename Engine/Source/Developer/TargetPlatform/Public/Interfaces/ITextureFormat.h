// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITextureFormat.h: Declares the ITextureFormat interface.
=============================================================================*/

#pragma once


/**
 * Interface for texture compression modules.
 */
class ITextureFormat
{
public:

	/**
	 * Checks whether this audio format can compress in parallel.
	 *
	 * @return true if parallel compression is supported, false otherwise.
	 */
	virtual bool AllowParallelBuild() const
	{
		return false;
	}

	/**
	 * Gets the current version of the specified texture format.
	 *
	 * @param Format - The format to get the version for.
	 *
	 * @return Version number.
	 */
	virtual uint16 GetVersion(FName Format) const = 0;

	/**
	 * Gets an optional derived data key string, so that the compressor can
	 * rely upon the number of mips, size of texture, etc, when compressing the image
	 *
	 * @param Texture - Reference to the texture we are compressing
	 *
	 * @return A string that will be used with the DDC, the string should be in the format "<DATA>_"
	 */
	virtual FString GetDerivedDataKeyString(const class UTexture& Texture) const
	{
		return TEXT("");
	}

	/**
	 * Gets the list of supported formats.
	 *
	 * @param OutFormats - Will hold the list of formats.
	 */
	virtual void GetSupportedFormats(TArray<FName>& OutFormats) const = 0;

	/**
	 * Compresses a single image.
	 *
	 * @param Image - The input image.
	 * @param BuildSettings - Build settings.
	 * @param bImageHasAlphaChannel - true if the image has a non-white alpha channel.
	 * @param OutCompressedMip - The compressed image.
	 *
	 * @returns true on success, false otherwise.
	 */
	virtual bool CompressImage(
		const struct FImage& Image,
		const struct FTextureBuildSettings& BuildSettings,
		bool bImageHasAlphaChannel,
		struct FCompressedImage2D& OutCompressedImage
	) const = 0;


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ITextureFormat() { }
};
