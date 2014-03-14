// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	JpegImageWrapper.h: Declares the FJpegImageWrapper class.
=============================================================================*/

#pragma once

#if WITH_UNREALJPEG


/**
 * Uncompresses JPEG data to raw 24bit RGB image that can be used by Unreal textures
 */
class FJpegImageWrapper
	: public FImageWrapperBase
{
public:

	/**
	 * Default constructor.
	 */
	FJpegImageWrapper();


public:

	// Begin FImageWrapperBase interface
	virtual bool SetCompressed( const void* InCompressedData, int32 InCompressedSize ) OVERRIDE;
	virtual bool SetRaw( const void* InRawData, int32 InRawSize, const int32 InWidth, const int32 InHeight, const ERGBFormat::Type InFormat, const int32 InBitDepth ) OVERRIDE;

	virtual void Uncompress( const ERGBFormat::Type InFormat, int32 InBitDepth ) OVERRIDE;
	virtual void Compress(int32 Quality) OVERRIDE;
	// End FImageWrapperBase interface
};


#endif //WITH_JPEG
