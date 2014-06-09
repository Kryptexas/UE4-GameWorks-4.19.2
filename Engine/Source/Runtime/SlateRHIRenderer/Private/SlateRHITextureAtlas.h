// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateRHITextureAtlas.h: Declares the FSlateTextureAtlasRHI class.
=============================================================================*/

#pragma once


/**
 * Represents a texture atlas for use with RHI.
 */
class FSlateTextureAtlasRHI
	: public FSlateTextureAtlas
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param Width
	 * @param Height
	 * @param StrideBytes
	 * @param Padding
	 */
	FSlateTextureAtlasRHI( uint32 Width, uint32 Height, uint32 SrideBytes, uint32 Padding );

	/**
	 * Destructor.
	 */
	~FSlateTextureAtlasRHI( );

public:

	/**
	 * Gets the atlas' underlying texture resource.
	 *
	 * @return The texture resource.
	 */
	FSlateTexture2DRHIRef* GetAtlasTexture( ) const
	{
		return AtlasTexture;
	}

	/**
	 * Releases rendering resources from the texture.
	 */	
	void ReleaseAtlasTexture( );
	
	/**
	 * Updates the texture on the render thread.
	 *
	 * @param RenderThreadData
	 */
	void UpdateTexture_RenderThread( FSlateTextureData* RenderThreadData );

public:

	// Begin FSlateTextureAtlas overrides.

	virtual void ConditionalUpdateTexture( );

	// End FSlateTextureAtlas interface.

private:

	/** The texture rendering resource */
	FSlateTexture2DRHIRef* AtlasTexture;
};
