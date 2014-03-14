// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ImageWrapperBase.h: Declares the FImageWrapperBase class.
=============================================================================*/

#pragma once


/**
 * The abstract helper class for handling the different image formats
 */
class FImageWrapperBase
	: public IImageWrapper
{
public:

	/**
	 * Default Constructor.
	 */
	FImageWrapperBase( );


public:

	/**
	 * Compresses the data.
	 */
	virtual void Compress( int32 Quality ) = 0;

	/**
	 * Resets the local variables.
	 */
	virtual void Reset( );

	/**
	 * Sets last error message.
	 */
	virtual void SetError( const TCHAR* ErrorMessage );

	/**  
	 * Function to uncompress our data 
	 *
	 * @param	InFormat		how we want to manipulate the RGB data
	 */
	virtual void Uncompress( const ERGBFormat::Type InFormat, int32 InBitDepth ) = 0;


public:

	// Begin IImageWrapper interface

	virtual const TArray<uint8>& GetCompressed( int32 Quality = 0 ) OVERRIDE;

	virtual int32 GetBitDepth( ) const OVERRIDE
	{
		return BitDepth;
	}

	virtual ERGBFormat::Type GetFormat() const OVERRIDE
	{
		return Format;
	}

	virtual int32 GetHeight( ) const OVERRIDE
	{
		return Height;
	}

	virtual bool GetRaw( const ERGBFormat::Type InFormat, int32 InBitDepth, const TArray<uint8>*& OutRawData ) OVERRIDE;

	virtual int32 GetWidth( ) const OVERRIDE
	{
		return Width;
	}

	virtual bool SetCompressed( const void* InCompressedData, int32 InCompressedSize ) OVERRIDE;

	virtual bool SetRaw( const void* InRawData, int32 InRawSize, const int32 InWidth, const int32 InHeight, const ERGBFormat::Type InFormat, const int32 InBitDepth ) OVERRIDE;

	// End IImageWrapper interface


protected:

	/** Arrays of compressed/raw data */
	TArray<uint8> RawData;
	TArray<uint8> CompressedData;

	/** Format of the raw data */
	TEnumAsByte<ERGBFormat::Type> RawFormat;
	int8 RawBitDepth;

	/** Format of the image */
	TEnumAsByte<ERGBFormat::Type> Format;

	/** Bit depth of the image */
	int8 BitDepth;

	/** Width/Height of the image data */
	int32 Width;
	int32 Height;

	/** Last Error Message. */
	FString LastError;
};
