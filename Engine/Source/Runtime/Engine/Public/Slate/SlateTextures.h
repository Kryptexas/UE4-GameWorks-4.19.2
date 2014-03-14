// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FSlateTextureData;

/**
 * Encapsulates a Texture2DRHIRef for use by a Slate rendering implementation                   
 */
class ENGINE_API FSlateTexture2DRHIRef : public TSlateTexture<FTexture2DRHIRef>, public FRenderResource
{
public:
	FSlateTexture2DRHIRef( FTexture2DRHIRef InRef, uint32 InWidth, uint32 InHeight );
	FSlateTexture2DRHIRef( uint32 InWidth, uint32 InHeight, EPixelFormat InPixelFormat, TSharedPtr<FSlateTextureData, ESPMode::ThreadSafe> InTextureData, uint32 InTexCreateFlags = 0, bool bCreateEmptyTexture = false );

	~FSlateTexture2DRHIRef();

	virtual uint32 GetWidth() const { return Width; }
	virtual uint32 GetHeight() const { return Height; }

	/** FRenderResource Interface.  Called when render resources need to be initialized */
	virtual void InitDynamicRHI();

	/** FRenderResource Interface.  Called when render resources need to be released */
	virtual void ReleaseDynamicRHI();

	/**
	 * Resize the texture.  Can only be called on the render thread
	 */
	void Resize( uint32 Width, uint32 Height );

	/**
	 * @return true if the texture is valid
	 */
	bool IsValid() const { return IsValidRef( ShaderResource ); }

	/** 
	 *  Sets the RHI Ref to use. 
	 */
	void SetRHIRef( FTexture2DRHIRef InRenderTargetTexture, uint32 InWidth, uint32 InHeight );

	FTexture2DRHIRef GetRHIRef() const { return ShaderResource; }

	/**
	 * Sets the bulk data for this texture.  Note: Does not reinitialize the resource,  Can only be used on the render thread
	 *
	 * @param NewTextureData	The new bulk data
	 */
	void SetTextureData( FSlateTextureDataPtr NewTextureData );
	
	/**
	 * Sets the bulk data for this texture and the format of the rendering resource
	 * Note: Does not reinitialize the resource.  Can only be used on the render thread
	 *
	 * @param NewTextureData	The new texture data
	 * @param InPixelFormat		The format of the texture data
	 * @param InTexCreateFlags	Flags for creating the rendering resource  
	 */
	void SetTextureData( FSlateTextureDataPtr NewTextureData, EPixelFormat InPixelFormat, uint32 InTexCreateFlags );

	/**
	 * Clears texture data being used.  Can only be accessed on the render thread                   
	 */
	void ClearTextureData();
protected:
	/** Width of this texture */
	uint32 Width;
	/** Height of this texture */
	uint32 Height;
private:
	/** Texture creation flags for if this texture needs to be recreated dynamically */
	uint32 TexCreateFlags;
	/** Data used between ReleaseDynamicRHI and InitDynamicRHI.  May be null if the data is not used */
	TSharedPtr<FSlateTextureData, ESPMode::ThreadSafe> TextureData;
	/** Pixel format of the texture */
	EPixelFormat PixelFormat;
	/** Whether or not to create an empty texture when this resource is created.  Useful if the texture is being updated elsewhere  */
	bool bCreateEmptyTexture;
};

/**
 * Encapsulates a render target for use by a Slate rendering implementation                   
 */
class FSlateRenderTargetRHI : public TSlateTexture<FTexture2DRHIRef>, public FRenderResource
{
public:
	FSlateRenderTargetRHI( FTexture2DRHIRef InRenderTargetTexture, uint32 InWidth, uint32 InHeight )
		: TSlateTexture( InRenderTargetTexture )
		, Width( InWidth )
		, Height( InHeight )
	{
	}

	virtual void InitDynamicRHI() {}

	/** 
	 * Releases all dynamic RHI data
	 */
	virtual void ReleaseDynamicRHI()
	{
		ShaderResource.SafeRelease();
	}

	virtual uint32 GetWidth() const { return Width; }
	virtual uint32 GetHeight() const { return Height; }

	/** 
	 *  Sets the RHI Ref to use.  Useful for reusing this class for multiple render targets
	 */
	ENGINE_API void SetRHIRef( FTexture2DRHIRef InRenderTargetTexture, uint32 InWidth, uint32 InHeight );

	FTexture2DRHIRef GetRHIRef() const { return ShaderResource; }
private:
	/** Width of this texture */
	uint32 Width;
	/** Height of this texture */
	uint32 Height;
};

class FSlateTextureRenderTarget2DResource : public FTextureRenderTargetResource
{
public:
	
	/** Constructor */
	ENGINE_API FSlateTextureRenderTarget2DResource(const FLinearColor& InClearColor, int32 InTargetSizeX, int32 InTargetSizeY, uint8 InFormat, ESamplerFilter InFilter, TextureAddress InAddressX, TextureAddress InAddressY, float InTargetGamma);

	/** Resizes the render target */
	virtual void SetSize(int32 InSizeX,int32 InSizeY);

	/** Gets the RHI resource for this render target */
	FTexture2DRHIRef GetTextureRHI() { return Texture2DRHI; }

public:
	// FTextureRenderTargetResource implementation
	virtual void ClampSize(int32 SizeX,int32 SizeY) OVERRIDE;

	// FRenderResource implementation
	virtual void InitDynamicRHI() OVERRIDE;
	virtual void ReleaseDynamicRHI() OVERRIDE;

	// FDeferredUpdateResource implementation
	virtual void UpdateResource() OVERRIDE;

	// FRenderTarget interface
	virtual FIntPoint GetSizeXY() const OVERRIDE;
	virtual float GetDisplayGamma() const OVERRIDE;

private:
	FTexture2DRHIRef Texture2DRHI;
	FLinearColor ClearColor;
	int32 TargetSizeX,TargetSizeY;

	uint8 Format;
	ESamplerFilter Filter;
	TextureAddress AddressX;
	TextureAddress AddressY;
	float TargetGamma;
};
