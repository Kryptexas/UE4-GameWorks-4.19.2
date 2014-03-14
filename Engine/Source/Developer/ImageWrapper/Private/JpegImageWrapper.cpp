// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	JpegImageWrapper.h: Declares the FJpegImageWrapper class.

	
	Source code for JPEG decompression from:
	http://code.google.com/p/jpeg-compressor/
=============================================================================*/

#include "ImageWrapperPrivatePCH.h"

#if WITH_UNREALJPEG

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#endif

#include "jpgd.h"
#include "jpgd.cpp"
#include "jpge.h"
#include "jpge.cpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

DEFINE_LOG_CATEGORY_STATIC(JPEGLog, Log, All);


// Only allow one thread to use JPEG decoder at a time (it's not thread safe)
FCriticalSection GJPEGSection; 


/* FJpegImageWrapper structors
 *****************************************************************************/

FJpegImageWrapper::FJpegImageWrapper( )
	: FImageWrapperBase()
{}


/* FImageWrapperBase interface
 *****************************************************************************/

bool FJpegImageWrapper::SetCompressed( const void* InCompressedData, int32 InCompressedSize )
{
	jpgd::jpeg_decoder_mem_stream jpeg_memStream( (uint8*)InCompressedData, InCompressedSize );

	jpgd::jpeg_decoder decoder(&jpeg_memStream);
	if ( decoder.get_error_code() != jpgd::JPGD_SUCCESS )
		return false;

	bool bResult = FImageWrapperBase::SetCompressed( InCompressedData, InCompressedSize );

	// We don't support 16 bit jpegs
	BitDepth = 8;

	Width = decoder.get_width();
	Height = decoder.get_height();

	switch ( decoder.get_num_components() )
	{
	case 1:
		Format = ERGBFormat::Gray;
		break;
	case 3:
		Format = ERGBFormat::RGBA;
		break;
	default:
		return false;
	}

	return bResult;
}

bool FJpegImageWrapper::SetRaw( const void* InRawData, int32 InRawSize, const int32 InWidth, const int32 InHeight, const ERGBFormat::Type InFormat, const int32 InBitDepth )
{
	check(InFormat == ERGBFormat::RGBA && InBitDepth == 8);

	bool bResult = FImageWrapperBase::SetRaw( InRawData, InRawSize, InWidth, InHeight, InFormat, InBitDepth );

	return bResult;
}

void FJpegImageWrapper::Compress( int32 Quality )
{
	if (CompressedData.Num() == 0)
	{
		FScopeLock JPEGLock(&GJPEGSection);
		
		if (Quality == 0) {Quality = 85;}
		ensure(Quality >= 1 && Quality <= 100);
		Quality = FMath::Clamp(Quality, 1, 100);

		check( RawData.Num() );
		check( Width > 0 );
		check( Height > 0 );

		CompressedData.Empty();
		CompressedData.AddUninitialized(RawData.Num());
		
		int OutBufferSize = CompressedData.Num();

		jpge::params Parameters;
		Parameters.m_quality = Quality;
		bool bSuccess = jpge::compress_image_to_jpeg_file_in_memory(
			CompressedData.GetTypedData(), OutBufferSize, Width, Height, 4, RawData.GetTypedData(), Parameters);
		
		check(bSuccess);

		CompressedData.RemoveAt(OutBufferSize, CompressedData.Num() - OutBufferSize);
	}
}

void FJpegImageWrapper::Uncompress( const ERGBFormat::Type InFormat, int32 InBitDepth )
{
	// Ensure we haven't already uncompressed the file.
	if ( RawData.Num() != 0 )
	{
		return;
	}

	// Get the number of channels we need to extract
	int Channels = 0;
	if ( ( InFormat == ERGBFormat::RGBA || InFormat == ERGBFormat::BGRA ) && InBitDepth == 8 )
	{
		Channels = 4;
	}
	else if ( InFormat == ERGBFormat::Gray && InBitDepth == 8 )
	{
		Channels = 1;
	}
	else
	{
		check( false );
	}

	FScopeLock JPEGLock( &GJPEGSection );

	check( CompressedData.Num() );

	int32 NumColors;
	uint8* OutData = jpgd::decompress_jpeg_image_from_memory(
		CompressedData.GetTypedData(), CompressedData.Num(), &Width, &Height, &NumColors, Channels );

	RawData.Empty();
	RawData.AddUninitialized( Width * Height * Channels );
	FMemory::Memcpy( RawData.GetTypedData(), OutData, RawData.Num() );
}


#endif //WITH_JPEG
