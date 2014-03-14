// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PngImageWrapper.h: Declares the FPngImageWrapper class.
=============================================================================*/

#pragma once

#if WITH_UNREALPNG

#include "ThirdParty/zlib/zlib-1.2.5/Inc/zlib.h"

#include "ThirdParty/libPNG/libpng-1.5.2/png.h"
#include "ThirdParty/libPNG/libpng-1.5.2/pnginfo.h"


/**
 * PNG implementation of the helper class
 */
class FPngImageWrapper
	: public FImageWrapperBase
{
public:

	/**
	 * Default Constructor.
	 */
	FPngImageWrapper();


public:

	// Begin FImageWrapper Interface

	virtual void Compress( int32 Quality ) OVERRIDE;

	virtual void Reset( ) OVERRIDE;
	
	virtual bool SetCompressed( const void* InCompressedData, int32 InCompressedSize ) OVERRIDE;
	
	virtual bool SetRaw( const void* InRawData, int32 InRawSize, const int32 InWidth, const int32 InHeight, const ERGBFormat::Type InFormat, const int32 InBitDepth ) OVERRIDE;

	virtual void Uncompress( const ERGBFormat::Type InFormat, int32 InBitDepth ) OVERRIDE;
	
	// End FImageWrapper Interface


protected:

	/** 
	 * Query whether this is a valid PNG type.
	 *
	 * @return true if data a PNG
	 */
	bool IsPNG( ) const;

	/** 
	 * Load the header information, returns true if successful.
	 *
	 * @return true if successful
	 */
	bool LoadHeader( );


protected:

	// Callbacks for the pnglibs
	static void user_read_compressed( png_structp png_ptr, png_bytep data, png_size_t length );
	static void user_write_compressed( png_structp png_ptr, png_bytep data, png_size_t length );
	static void user_flush_data( png_structp png_ptr );
	static void user_error_fn( png_structp png_ptr, png_const_charp error_msg );
	static void user_warning_fn( png_structp png_ptr, png_const_charp warning_msg );


private:

	// The read offset into our array.
	int32 ReadOffset;

	// The color type as defined in the header.
	int32 ColorType;

	// The number of channels.
	uint8 Channels;
};


#endif	//WITH_UNREALPNG
