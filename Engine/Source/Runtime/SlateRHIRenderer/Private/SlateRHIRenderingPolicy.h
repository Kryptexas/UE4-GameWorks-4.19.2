// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once

class FSlateRHIResourceManager;

#include "RenderingPolicy.h"

namespace SlateRHIConstants
{
	// Number of vertex and index buffers we swap between when drawing windows
	const int32 NumBuffers = 2;
}

/** 
 * Vertex buffer containing all Slate vertices
 */
template <typename VertexType>
class TSlateElementVertexBuffer : public FVertexBuffer
{
public:
	TSlateElementVertexBuffer()
		: BufferSize(0)
		, MinBufferSize(0)
		, BufferUsageSize(0)
	{}

	~TSlateElementVertexBuffer() {};

	void Init( int32 MinNumVertices )
	{
		MinBufferSize = sizeof(VertexType) * FMath::Max( MinNumVertices, 200 );

		BeginInitResource(this);
	}

	void Destroy()
	{
		BeginReleaseResource(this);
	}

	/** Initializes the vertex buffers RHI resource. */
	virtual void InitDynamicRHI()
	{
		if( !IsValidRef(VertexBufferRHI) )
		{
			check( MinBufferSize > 0 )
	
			BufferSize = MinBufferSize;

			FRHIResourceCreateInfo CreateInfo;
			VertexBufferRHI = RHICreateVertexBuffer( MinBufferSize, BUF_Dynamic, CreateInfo );

			// Ensure the vertex buffer could be created
			check(IsValidRef(VertexBufferRHI));
		}
	}

	/** Releases the vertex buffers RHI resource. */
	virtual void ReleaseDynamicRHI()
	{
		VertexBufferRHI.SafeRelease();
		BufferSize = 0;
	}

	/** Returns a friendly name for this buffer. */
	virtual FString GetFriendlyName() const { return TEXT("SlateElementVertices"); }

	/** Returns the size of the buffer in bytes. */
	int32 GetBufferSize() const { return BufferSize; }

	/** Returns the used size of this buffer */
	int32 GetBufferUsageSize() const { return BufferUsageSize; }

	/** Resets the usage of the buffer */
	void ResetBufferUsage() { BufferUsageSize = 0; }

	/** Resizes buffer, accumulates states safely on render thread */
	void PreFillBuffer(int32 RequiredVertexCount, bool bShrinkToMinSize)
	{
		checkSlow(IsInRenderingThread());

		if (RequiredVertexCount > 0 )
		{
#if !SLATE_USE_32BIT_INDICES
			// make sure our index buffer can handle this
			checkf(RequiredVertexCount < 0xFFFF, TEXT("Slate vertex buffer is too large (%d) to work with uint16 indices"), RequiredVertexCount);
#endif
			int32 RequiredBufferSize = RequiredVertexCount*sizeof(VertexType);

			// resize if needed
			if(RequiredBufferSize > GetBufferSize() || bShrinkToMinSize)
			{
				ResizeBuffer(RequiredBufferSize);
			}

			BufferUsageSize = RequiredBufferSize;
		}

	}

	int32 GetMinBufferSize() const { return MinBufferSize; }

	void* LockBuffer_RenderThread(int32 NumVertices)
	{
		uint32 RequiredBufferSize = NumVertices*sizeof(VertexType);	

		return RHILockVertexBuffer( VertexBufferRHI, 0, RequiredBufferSize, RLM_WriteOnly );
	}

	void UnlockBuffer_RenderThread()
	{
		RHIUnlockVertexBuffer( VertexBufferRHI );
	}

	void* LockBuffer_RHIThread(int32 NumVertices)
	{
		int32 RequiredBufferSize = NumVertices*sizeof(VertexType);

		//use non-flushing DynamicRHI version when on RHIThread.
		return GDynamicRHI->RHILockVertexBuffer(VertexBufferRHI, 0, RequiredBufferSize, RLM_WriteOnly);
	}

	void UnlockBuffer_RHIThread()
	{
		GDynamicRHI->RHIUnlockVertexBuffer(VertexBufferRHI);		
	}

private:
	/** Resizes the buffer to the passed in size.  Preserves internal data*/
	void ResizeBuffer( int32 NewSizeBytes )
	{
		checkSlow( IsInRenderingThread() );

		int32 FinalSize = FMath::Max( NewSizeBytes, MinBufferSize );

		if( FinalSize != 0 && FinalSize != BufferSize )
		{
			VertexBufferRHI.SafeRelease();

			FRHIResourceCreateInfo CreateInfo;
			VertexBufferRHI = RHICreateVertexBuffer(FinalSize, BUF_Dynamic, CreateInfo);

			check(IsValidRef(VertexBufferRHI));

			BufferSize = FinalSize;
		}
	}

private:
	/** The size of the buffer in bytes. */
	int32 BufferSize;
	
	/** The minimum size the buffer should always be */
	int32 MinBufferSize;

	/** The size of the used portion of the buffer */
	int32 BufferUsageSize;

	/** Hidden copy methods. */
	TSlateElementVertexBuffer( const TSlateElementVertexBuffer& );
	void operator=(const TSlateElementVertexBuffer& );
};

class FSlateElementIndexBuffer : public FIndexBuffer
{
public:
	/**
	 * Constructor
	 * 
	 * @param MinNumIndices	The minimum number of indices this buffer should always support
	 */
	FSlateElementIndexBuffer();
	~FSlateElementIndexBuffer();

	void Init( int32 MinNumIndices );
	void Destroy();

	/** Initializes the index buffers RHI resource. */
	virtual void InitDynamicRHI() override;

	/** Releases the index buffers RHI resource. */
	virtual void ReleaseDynamicRHI() override;

	/** Returns a friendly name for this buffer. */
	virtual FString GetFriendlyName() const override { return TEXT("SlateElementIndices"); }

	/** Returns the size of this buffer */
	int32 GetBufferSize() const { return BufferSize; }

	/** Returns the used size of this buffer */
	int32 GetBufferUsageSize() const { return BufferUsageSize; }

	/** Resets the usage of the buffer */
	void ResetBufferUsage() { BufferUsageSize = 0; }

	/** Resizes buffer, accumulates states safely on render thread */
	void PreFillBuffer(int32 RequiredIndexCount, bool bShrinkToMinSize);

	void* LockBuffer_RenderThread(int32 NumIndices);
	void UnlockBuffer_RenderThread();

	void* LockBuffer_RHIThread(int32 NumIndices);
	void UnlockBuffer_RHIThread();

	int32 GetMinBufferSize() const { return MinBufferSize; }

private:
	/** Resizes the buffer to the passed in size.  Preserves internal data */
	void ResizeBuffer( int32 NewSizeBytes );
private:
	/** The current size of the buffer in bytes. */
	int32 BufferSize;
	
	/** The minimum size the buffer should always be */
	int32 MinBufferSize;

	/** The size of the currently used portion of the buffer */
	int32 BufferUsageSize;

	/** Hidden copy methods. */
	FSlateElementIndexBuffer( const FSlateElementIndexBuffer& );
	void operator=(const FSlateElementIndexBuffer& );

};

class FSlateRHIRenderingPolicy : public FSlateRenderingPolicy
{
public:
	FSlateRHIRenderingPolicy( TSharedPtr<FSlateFontCache> InFontCache, TSharedRef<FSlateRHIResourceManager> InResourceManager );
	~FSlateRHIRenderingPolicy();

	void UpdateVertexAndIndexBuffers(FRHICommandListImmediate& RHICmdList, FSlateBatchData& BatchData);

	virtual void DrawElements(FRHICommandListImmediate& RHICmdList, class FSlateBackBuffer& BackBuffer, const FMatrix& ViewProjectionMatrix, const TArray<FSlateRenderBatch>& RenderBatches, bool bAllowSwtichVerticalAxis=true);

	virtual TSharedRef<FSlateFontCache> GetFontCache() override { return FontCache.ToSharedRef(); }
	virtual TSharedRef<FSlateShaderResourceManager> GetResourceManager() override { return ResourceManager; }
	virtual bool IsVertexColorInLinearSpace() const override { return false; }
	
	void InitResources();
	void ReleaseResources();

	void BeginDrawingWindows();
	void EndDrawingWindows();

	void SetUseGammaCorrection( bool bInUseGammaCorrection ) { bGammaCorrect = bInUseGammaCorrection; }
private:
	/**
	 * Returns the pixel shader that should be used for the specified ShaderType and DrawEffects
	 * 
	 * @param ShaderType	The shader type being used
	 * @param DrawEffects	Draw effects being used
	 * @return The pixel shader for use with the shader type and draw effects
	 */
	class FSlateElementPS* GetTexturePixelShader( ESlateShader::Type ShaderType, ESlateDrawEffect::Type DrawEffects );
	class FSlateMaterialShaderPS* GetMaterialPixelShader( const class FMaterial* Material, ESlateShader::Type ShaderType, ESlateDrawEffect::Type DrawEffects );

	/** @return The RHI primitive type from the Slate primitive type */
	EPrimitiveType GetRHIPrimitiveType(ESlateDrawPrimitive::Type SlateType);
private:
	/** Buffers used for rendering */
	TSlateElementVertexBuffer<FSlateVertex> VertexBuffers[SlateRHIConstants::NumBuffers];
	FSlateElementIndexBuffer IndexBuffers[SlateRHIConstants::NumBuffers];

	TSharedRef<FSlateRHIResourceManager> ResourceManager;
	TSharedPtr<FSlateFontCache> FontCache;

	uint8 CurrentBufferIndex;
	
	bool bGammaCorrect;
};


