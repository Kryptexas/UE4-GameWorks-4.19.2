// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * ICNS implementation of the helper class
 */
class FIcnsImageWrapper
	: public FImageWrapperBase
{
public:

	/**
	 * Default Constructor.
	 */
	FIcnsImageWrapper();

public:

	// FImageWrapper Interface

	virtual void Compress( int32 Quality ) override;
	virtual void Uncompress( const ERGBFormat::Type InFormat, int32 InBitDepth ) override;
};
