// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "IOSView.h"


struct FPipelineShadow
{
	FPipelineShadow()
	{
		// zero out our memory
		FMemory::Memzero(this, sizeof(*this));

		for (int Index = 0; Index < MaxMetalRenderTargets; Index++)
		{
			RenderTargets[Index] = [[MTLRenderPipelineAttachmentDescriptor alloc] init];
		}
	}

	/**
	 * @return the hash of the current pipeline state
	 */
	uint64 GetHash() const
	{
		return Hash;
	}

	/**
	 * FOrce a pipeline state for a known hash
	 */
	void SetHash(uint64 InHash);
	
	/**
	 * @return an pipeline state object that matches the current state and the BSS
	 */
	id<MTLRenderPipelineState> CreatePipelineStateForBoundShaderState(FMetalBoundShaderState* BSS) const;

	bool bIsDepthWriteEnabled;
	bool bIsStencilWriteEnabled;
	MTLRenderPipelineAttachmentDescriptor* RenderTargets[MaxMetalRenderTargets];
	MTLPixelFormat DepthTargetFormat;
    uint32 SampleCount;

	// running hash of the pipeline state
	uint64 Hash;
};

struct FRingBuffer
{
	FRingBuffer(id<MTLDevice> Device, uint32 Size, uint32 InDefaultAlignment)
	{
		DefaultAlignment = InDefaultAlignment;
		Buffer = [Device newBufferWithLength:Size options:BUFFER_CACHE_MODE];
		Offset = 0;
	}

	uint32 Allocate(uint32 Size, uint32 Alignment);

	uint32 DefaultAlignment;
	id<MTLBuffer> Buffer;
	uint32 Offset;
};

class FMetalManager
{
public:
	static FMetalManager* Get();
	static id<MTLDevice> GetDevice();
	static id<MTLRenderCommandEncoder> GetContext();
	static void ReleaseObject(id Object);
	
	/** RHIBeginScene helper */
	void BeginScene();
	/** RHIEndScene helper */
	void EndScene();

	void BeginFrame();
	void EndFrame(bool bPresent);
	
	/**
	 * Return a texture object that wraps the backbuffer
	 */
	FMetalTexture2D* GetBackBuffer();

//	/**
//	 * Sets a pending stream source, which will be hooked up to the vertex decl in PrepareToDraw
//	 */
//	void SetPendingStream(uint32 StreamIndex, TRefCountPtr<FMetalVertexBuffer> VertexBuffer, uint32 Offset, uint32 Stride);

	/**
	 * Do anything necessary to prepare for any kind of draw call 
	 */
	void PrepareToDraw(uint32 NumVertices);
	
	/**
	 * Functions to update the pipeline descriptor and/or context
	 */
	void SetDepthStencilWriteEnabled(bool bIsDepthWriteEnabled, bool bIsStencilWriteEnabled);
	void SetBlendState(FMetalBlendState* BlendState);
	void SetBoundShaderState(FMetalBoundShaderState* BoundShaderState);
	void SetCurrentRenderTarget(FMetalSurface* RenderSurface);
	void SetCurrentDepthStencilTarget(FMetalSurface* RenderSurface);
	
	/**
	 * Update the context with the current render targets
	 */
	void UpdateContext();
	
	/**
	 * Allocate from a dynamic ring buffer - by default align to the allowed alignment for offset field when setting buffers
	 */
	uint32 AllocateFromRingBuffer(uint32 Size, uint32 Alignment=0);
	id<MTLBuffer> GetRingBuffer()
	{
		return RingBuffer.Buffer;
	}

	uint32 AllocateFromQueryBuffer();
	id<MTLBuffer> GetQueryBuffer()
	{
		return QueryBuffer.Buffer;
	}

	uint64 GetCommandBufferIndex()
	{
		return CommandBufferIndex;
	}
	
	void SetCompletedCommandBufferIndex(uint64 Index)
	{
		CompletedCommandBufferIndex = Index;
	}

	bool WaitForCommandBufferComplete(uint64 IndexToWaitFor, double Timeout);

	void SetRasterizerState(const FRasterizerStateInitializerRHI& State);

	FMetalShaderParameterCache& GetShaderParameters(uint32 Stage)
	{
		return ShaderParameters[Stage];
	}

protected:
	FMetalManager();
	void InitFrame();
	void GenerateFetchShader();

	id<MTLDevice> Device;

	id<MTLCommandQueue> CommandQueue;
	
	uint32 NumDrawCalls;
	id<MTLCommandBuffer> CurrentCommandBuffer;
	id<CAMetalDrawable> CurrentDrawable;

	dispatch_semaphore_t CommandBufferSemaphore;

	MTLRenderPassDescriptor* CurrentRenderPass;
	id<MTLRenderCommandEncoder> CurrentContext;
	
	id<MTLTexture> CurrentColorRenderTexture, PreviousColorRenderTexture;
	id<MTLTexture> CurrentDepthRenderTexture, PreviousDepthRenderTexture;
	id<MTLTexture> CurrentMSAARenderTexture;
	
	TRefCountPtr<FMetalTexture2D> BackBuffer;
	TRefCountPtr<FMetalBoundShaderState> CurrentBoundShaderState;
	
	class FMetalShaderParameterCache*	ShaderParameters;

	// the running pipeline state descriptor object
	FPipelineShadow Pipeline;

	MTLRenderPipelineDescriptor* CurrentPipelineDescriptor;
	// a parellel descriptor for no depth buffer (since we can't reset the depth format to invalid YET)
	MTLRenderPipelineDescriptor* CurrentPipelineDescriptor2;
	

	// the amazing ring buffer for dynamic data
	FRingBuffer RingBuffer, QueryBuffer;

	TArray<id> DelayedFreeLists[4];
	uint32 WhichFreeList;

	// the slot to store a per-thread autorelease pool
	uint32 AutoReleasePoolTLSSlot;

	// always growing command buffer index
	uint64 CommandBufferIndex;
	uint64 CompletedCommandBufferIndex;

	/** Internal frame counter, incremented on each call to RHIBeginScene. */
	uint32 SceneFrameCounter;

	/**
	 * Internal counter used for resource table caching.
	 * INDEX_NONE means caching is not allowed.
	 */
	uint32 ResourceTableFrameCounter;

	// shadow the rasterizer state to reduce some render encoder pressure
	FRasterizerStateInitializerRHI ShadowRasterizerState;
	bool bFirstRasterizerState;

	/** Apply the SRT before drawing */
	void CommitGraphicsResourceTables();

	void CommitNonComputeShaderConstants();

};

// Stats
DECLARE_CYCLE_STAT_EXTERN(TEXT("MakeDrawable time"),STAT_MetalMakeDrawableTime,STATGROUP_MetalRHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Draw call time"),STAT_MetalDrawCallTime,STATGROUP_MetalRHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("PrepareDraw time"),STAT_MetalPrepareDrawTime,STATGROUP_MetalRHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("PipelineState time"),STAT_MetalPipelineStateTime,STATGROUP_MetalRHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("BpoundShaderState time"),STAT_MetalBoundShaderStateTime,STATGROUP_MetalRHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("VertexDeclaration time"),STAT_MetalVertexDeclarationTime,STATGROUP_MetalRHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Uniform buffer pool cleanup time"), STAT_MetalUniformBufferCleanupTime, STATGROUP_MetalRHI, );
DECLARE_MEMORY_STAT_EXTERN(TEXT("Uniform buffer pool memory"), STAT_MetalFreeUniformBufferMemory, STATGROUP_MetalRHI, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Uniform buffer pool num free"), STAT_MetalNumFreeUniformBuffers, STATGROUP_MetalRHI, );
