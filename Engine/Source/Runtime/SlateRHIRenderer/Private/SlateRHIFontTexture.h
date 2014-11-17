// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Override for font textures that saves data between Init and ReleaseDynamicRHI to 
 * ensure all characters in the font texture exist if the rendering resource has to be recreated 
 * between caching new characters
 */
class FSlateFontTextureRHI : public FSlateTexture2DRHIRef
{
public:
	/** Constructor.  Initializes the texture
	 *
	 * @param InWidth The width of the texture
	 * @param InHeight The height of the texture
	 */
	FSlateFontTextureRHI( uint32 InWidth, uint32 InHeight );

	/** FRenderResource interface */
	virtual void InitDynamicRHI() override;
	virtual void ReleaseDynamicRHI() override;
private:
	/** Temporary data stored between Release and InitDynamicRHI */
	TArray<uint8> TempData;
};

/** 
 * Representation of a texture for fonts in which characters are packed tightly based on their bounding rectangle 
 */
class FSlateFontAtlasRHI : public FSlateFontAtlas
{
public:
	FSlateFontAtlasRHI( uint32 Width, uint32 Height );
	~FSlateFontAtlasRHI();

	/**
	 * FSlateFontAtlas interface 
	 */
	virtual class FSlateShaderResource* GetTexture()  override { return FontTexture; }
	virtual void ConditionalUpdateTexture()  override;
	virtual void ReleaseResources() override;
private:
	FSlateTexture2DRHIRef* FontTexture;

};
