// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * BMP implementation of the helper class
 */
class FBmpImageWrapper
	: public FImageWrapperBase
{
public:

	/**
	 * Default Constructor.
	 */
	FBmpImageWrapper();

public:

	// Begin FImageWrapper Interface

	virtual void Compress( int32 Quality ) OVERRIDE;

	virtual void Uncompress( const ERGBFormat::Type InFormat, int32 InBitDepth ) OVERRIDE;
	
	virtual bool SetCompressed( const void* InCompressedData, int32 InCompressedSize ) OVERRIDE;

	// End FImageWrapper Interface

private:

	/** 
	 * Load the header information, returns true if successful.
	 *
	 * @return true if successful
	 */
	bool LoadHeader();
};
