// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Represents a texture atlas for use with RHI
 */
class FSlateTextureAtlasRHI : public FSlateTextureAtlas
{
public:
	FSlateTextureAtlasRHI( uint32 Width, uint32 Height, uint32 SrideBytes, uint32 Padding );
	~FSlateTextureAtlasRHI();

	/**
	 * Releases rendering resources from the texture
	 */	
	void ReleaseAtlasTexture();
	
	/** @return The texture resource for rendering */
	FSlateTexture2DRHIRef* GetAtlasTexture() const { return AtlasTexture; }
	
	/** FSlateTextureAtlas interface */
	virtual void ConditionalUpdateTexture();

	/**
	 * Updates the texture on the render thread
	 */
	void UpdateTexture_RenderThread( FSlateTextureData* RenderThreadData );
private:
	/** The texture rendering resource */
	FSlateTexture2DRHIRef* AtlasTexture;
};