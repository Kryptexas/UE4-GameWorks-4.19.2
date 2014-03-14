// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

class FSlateRHIResourceManager;

#include "RenderingPolicy.h"

/** 
 * Vertex buffer containing all Slate vertices
 * All vertices are added through RHILockVertexBuffer
 */
class FSlateElementVertexBuffer : public FVertexBuffer
{
public:
	FSlateElementVertexBuffer();
	~FSlateElementVertexBuffer();

	/** Initializes the vertex buffers RHI resource. */
	virtual void InitDynamicRHI();

	/** Releases the vertex buffers RHI resource. */
	virtual void ReleaseDynamicRHI();

	/** Returns a friendly name for this buffer. */
	virtual FString GetFriendlyName() const { return TEXT("SlateElementVertices"); }

	/** Returns the size of the buffer in bytes. */
	uint32 GetBufferSize() const { return BufferSize; }

	/** Returns the used size of this buffer */
	uint32 GetBufferUsageSize() const { return BufferUsageSize; }

	/** Resets the usage of the buffer */
	void ResetBufferUsage() { BufferUsageSize = 0; }

	/** Fills the buffer with slate vertices */
	void FillBuffer( const TArray<FSlateVertex>& InVertices, bool bShrinkToFit );
private:
	/** Resizes the buffer to the passed in size.  Preserves internal data*/
	void ResizeBuffer( uint32 NewSizeBytes );

private:
	/** The size of the buffer in bytes. */
	uint32 BufferSize;
	
	/** The size of the used portion of the buffer */
	uint32 BufferUsageSize;

	/** Hidden copy methods. */
	FSlateElementVertexBuffer( const FSlateElementVertexBuffer& );
	void operator=(const FSlateElementVertexBuffer& );
};

class FSlateElementIndexBuffer : public FIndexBuffer
{
public:
	FSlateElementIndexBuffer();
	~FSlateElementIndexBuffer();

	/** Initializes the index buffers RHI resource. */
	virtual void InitDynamicRHI();

	/** Releases the index buffers RHI resource. */
	virtual void ReleaseDynamicRHI();

	/** Returns a friendly name for this buffer. */
	virtual FString GetFriendlyName() const { return TEXT("SlateElementIndices"); }

	/** Returns the size of this buffer */
	uint32 GetBufferSize() const { return BufferSize; }

	/** Returns the used size of this buffer */
	uint32 GetBufferUsageSize() const { return BufferUsageSize; }

	/** Resets the usage of the buffer */
	void ResetBufferUsage() { BufferUsageSize = 0; }

	/** Fills the buffer with slate vertices */
	void FillBuffer( const TArray<SlateIndex>& InIndices, bool bShrinkToFit );
private:
	/** Resizes the buffer to the passed in size.  Preserves internal data */
	void ResizeBuffer( uint32 NewSizeBytes );
private:
	/** The size of the buffer in bytes. */
	uint32 BufferSize;
	
	/** The size of the used portion of the buffer */
	uint32 BufferUsageSize;

	/** Hidden copy methods. */
	FSlateElementIndexBuffer( const FSlateElementIndexBuffer& );
	void operator=(const FSlateElementIndexBuffer& );

};

class FSlateRenderTarget : public FRenderTarget
{
public:
	FSlateRenderTarget( FTexture2DRHIRef& InRenderTargetTexture, FIntPoint InSizeXY )
		: SizeXY( InSizeXY )
	{
		RenderTargetTextureRHI = InRenderTargetTexture;
	}
	virtual FIntPoint GetSizeXY() const OVERRIDE { return SizeXY; }
private:
	FIntPoint SizeXY;
};

class FSlateRHIRenderingPolicy : public FSlateRenderingPolicy
{
public:
	FSlateRHIRenderingPolicy( TSharedPtr<FSlateFontCache> InFontCache, TSharedRef<FSlateRHIResourceManager> InTextureManager );
	~FSlateRHIRenderingPolicy();
	
	virtual void UpdateBuffers( const FSlateWindowElementList& WindowElementList ) OVERRIDE;
	virtual void DrawElements( const FIntPoint& InViewportSize, FSlateRenderTarget& BackBuffer, const FMatrix& ViewProjectionMatrix, const TArray<FSlateRenderBatch>& RenderBatches );

	class FSlateShaderResource* GetViewportResource( const ISlateViewport* InViewportInterface );
	virtual class FSlateShaderResourceProxy* GetTextureResource( const FSlateBrush& Brush ) OVERRIDE;
	TSharedPtr<FSlateFontCache>& GetFontCache() { return FontCache; }
	
	void InitResources();
	void ReleaseResources();

	void BeginDrawingWindows();
	void EndDrawingWindows();
private:
	/**
	 * Returns the pixel shader that should be used for the specified ShaderType and DrawEffects
	 * 
	 * @param ShaderType	The shader type being used
	 * @param DrawEffects	Draw effects being used
	 * @return The pixel shader for use with the shader type and draw effects
	 */
	class FSlateElementPS* GetPixelShader( ESlateShader::Type ShaderType, ESlateDrawEffect::Type DrawEffects );
private:
	/** Global shader states */
	static FGlobalBoundShaderState NormalShaderStates[4][2 /* UseTextureAlpha */];
	static FGlobalBoundShaderState DisabledShaderStates[4][2 /* UseTextureAlpha */];

	/** Buffers used for rendering */
	FSlateElementVertexBuffer VertexBuffers[2];
	FSlateElementIndexBuffer IndexBuffers[2];

	TSharedRef<FSlateRHIResourceManager> TextureManager;
	TSharedPtr<FSlateFontCache> FontCache;
	uint8 CurrentBufferIndex;
	
	/** If we should shrink resources that are no longer used (do not set this from the game thread)*/
	bool bShouldShrinkResources;
};


