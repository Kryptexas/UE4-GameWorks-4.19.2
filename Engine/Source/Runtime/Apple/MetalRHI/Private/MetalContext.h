// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#pragma once

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "MetalViewport.h"
#include "MetalBufferPools.h"
#include "MetalCommandEncoder.h"
#include "MetalCommandQueue.h"
#include "MetalCommandList.h"
#include "MetalRenderPass.h"
#if PLATFORM_IOS
#include "IOSView.h"
#endif
#include "LockFreeList.h"

#define NUM_SAFE_FRAMES 4

class FMetalRHICommandContext;

class FMetalContext
{
	friend class FMetalCommandContextContainer;
public:
	FMetalContext(FMetalCommandQueue& Queue, bool const bIsImmediate);
	virtual ~FMetalContext();
	
	static FMetalContext* GetCurrentContext();
	
	id<MTLDevice> GetDevice();
	FMetalCommandQueue& GetCommandQueue();
	FMetalCommandList& GetCommandList();
	id<MTLCommandBuffer> GetCurrentCommandBuffer();
	FMetalStateCache& GetCurrentState() { return StateCache; }
	FMetalRenderPass& GetCurrentRenderPass() { return RenderPass; }
	
	void InsertCommandBufferFence(FMetalCommandBufferFence& Fence);
	
	/**
	 * Handle rendering thread starting/stopping
	 */
	static void CreateAutoreleasePool();
	static void DrainAutoreleasePool();

	/**
	 * Do anything necessary to prepare for any kind of draw call 
	 * @param PrimitiveType The UE4 primitive type for the draw call, needed to compile the correct render pipeline.
	 * @param IndexType The index buffer type (none, uint16, uint32), needed to compile the correct tessellation compute pipeline.
	 * @returns True if the preparation completed and the draw call can be encoded, false to skip.
	 */
	bool PrepareToDraw(uint32 PrimitiveType, EMetalIndexType IndexType = EMetalIndexType_None);
	
	/**
	 * Set the color, depth and stencil render targets, and then make the new command buffer/encoder
	 */
	void SetRenderTargetsInfo(const FRHISetRenderTargetsInfo& RenderTargetsInfo, bool const bReset = true);
	
	/**
	 * Allocate from a dynamic ring buffer - by default align to the allowed alignment for offset field when setting buffers
	 */
	uint32 AllocateFromRingBuffer(uint32 Size, uint32 Alignment=0);
	id<MTLBuffer> GetRingBuffer();

	TSharedRef<FMetalQueryBufferPool, ESPMode::ThreadSafe> GetQueryBufferPool()
	{
		return QueryBuffer.ToSharedRef();
	}
	
	/** @returns True if the Metal validation layer is enabled else false. */
	bool IsValidationLayerEnabled() const { return bValidationEnabled; }

    void SubmitCommandsHint(uint32 const bFlags = EMetalSubmitFlagsCreateCommandBuffer);
	void SubmitCommandBufferAndWait();
	void ResetRenderCommandEncoder();
	
	void DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances);
	
	void DrawPrimitiveIndirect(uint32 PrimitiveType, FMetalVertexBuffer* VertexBuffer, uint32 ArgumentOffset);
	
	void DrawIndexedPrimitive(id<MTLBuffer> IndexBuffer, uint32 IndexStride, MTLIndexType IndexType, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 FirstInstance,
							  uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances);
	
	void DrawIndexedIndirect(FMetalIndexBuffer* IndexBufferRHI, uint32 PrimitiveType, FMetalStructuredBuffer* VertexBufferRHI, int32 DrawArgumentsIndex, uint32 NumInstances);
	
	void DrawIndexedPrimitiveIndirect(uint32 PrimitiveType,FMetalIndexBuffer* IndexBufferRHI,FMetalVertexBuffer* VertexBufferRHI,uint32 ArgumentOffset);
	
	void DrawPatches(uint32 PrimitiveType, id<MTLBuffer> IndexBuffer, uint32 IndexBufferStride, int32 BaseVertexIndex, uint32 FirstInstance, uint32 StartIndex,
					 uint32 NumPrimitives, uint32 NumInstances);
	
	void CopyFromTextureToBuffer(id<MTLTexture> Texture, uint32 sourceSlice, uint32 sourceLevel, MTLOrigin sourceOrigin, MTLSize sourceSize, id<MTLBuffer> toBuffer, uint32 destinationOffset, uint32 destinationBytesPerRow, uint32 destinationBytesPerImage, MTLBlitOption options);
	
	void CopyFromBufferToTexture(id<MTLBuffer> Buffer, uint32 sourceOffset, uint32 sourceBytesPerRow, uint32 sourceBytesPerImage, MTLSize sourceSize, id<MTLTexture> toTexture, uint32 destinationSlice, uint32 destinationLevel, MTLOrigin destinationOrigin);
	
	void CopyFromTextureToTexture(id<MTLTexture> Texture, uint32 sourceSlice, uint32 sourceLevel, MTLOrigin sourceOrigin, MTLSize sourceSize, id<MTLTexture> toTexture, uint32 destinationSlice, uint32 destinationLevel, MTLOrigin destinationOrigin);
	
    id<MTLCommandBuffer> BeginAsyncCommands(void);
    
    void AsyncCopyFromBufferToTexture(id<MTLCommandBuffer> CmdBuf, id<MTLBuffer> Buffer, uint32 sourceOffset, uint32 sourceBytesPerRow, uint32 sourceBytesPerImage, MTLSize sourceSize, id<MTLTexture> toTexture, uint32 destinationSlice, uint32 destinationLevel, MTLOrigin destinationOrigin);
    
    void AsyncCopyFromTextureToTexture(id<MTLCommandBuffer> CmdBuf, id<MTLTexture> Texture, uint32 sourceSlice, uint32 sourceLevel, MTLOrigin sourceOrigin, MTLSize sourceSize, id<MTLTexture> toTexture, uint32 destinationSlice, uint32 destinationLevel, MTLOrigin destinationOrigin);
    
    void GenerateMipmapsForTexture(id<MTLCommandBuffer> CmdBuf, id<MTLTexture> Texture);
    
    void EndAsyncCommands(id<MTLCommandBuffer> CmdBuf, bool const bWait);
    
	void SynchronizeTexture(id<MTLTexture> Texture, uint32 Slice, uint32 Level);
	
	void SynchroniseResource(id<MTLResource> Resource);
	
	void FillBuffer(id<MTLBuffer> Buffer, NSRange Range, uint8 Value);

	void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ);
#if METAL_API_1_1
	void DispatchIndirect(FMetalVertexBuffer* ArgumentBuffer, uint32 ArgumentOffset);
#endif

	void StartTiming(class FMetalEventNode* EventNode);
	void EndTiming(class FMetalEventNode* EventNode);

	static void MakeCurrent(FMetalContext* Context);
	void InitFrame(bool const bImmediateContext);
	void FinishFrame();

protected:
	/** The underlying Metal device */
	id<MTLDevice> Device;
	
	/** The wrapper around the device command-queue for creating & committing command buffers to */
	FMetalCommandQueue& CommandQueue;
	
	/** The wrapper around commabd buffers for ensuring correct parallel execution order */
	FMetalCommandList CommandList;
	
	/** The cache of all tracked & accessible state. */
	FMetalStateCache StateCache;
	
	/** The render pass handler that actually encodes our commands. */
	FMetalRenderPass RenderPass;
	
	/** A sempahore used to ensure that wait for previous frames to complete if more are in flight than we permit */
	dispatch_semaphore_t CommandBufferSemaphore;
	
	/** A pool of buffers for writing visibility query results. */
	TSharedPtr<FMetalQueryBufferPool, ESPMode::ThreadSafe> QueryBuffer;
	
	/** A fallback depth-stencil surface for draw calls that write to depth without a depth-stencil surface bound. */
	FTexture2DRHIRef FallbackDepthStencilSurface;
	
	/** the slot to store a per-thread autorelease pool */
	static uint32 AutoReleasePoolTLSSlot;
	
	/** the slot to store a per-thread context ref */
	static uint32 CurrentContextTLSSlot;
	
	/** Whether the validation layer is enabled */
	bool bValidationEnabled;
};


class FMetalDeviceContext : public FMetalContext
{
public:
	static FMetalDeviceContext* CreateDeviceContext();
	virtual ~FMetalDeviceContext();
	
	inline bool SupportsFeature(EMetalFeatures InFeature) { return CommandQueue.SupportsFeature(InFeature); }
	
	FMetalPooledBuffer CreatePooledBuffer(FMetalPooledBufferArgs const& Args);
	void ReleasePooledBuffer(FMetalPooledBuffer Buf);
	void ReleaseObject(id Object);
	
	void BeginFrame();
	void ClearFreeList();
	void EndFrame();
	
	/** RHIBeginScene helper */
	void BeginScene();
	/** RHIEndScene helper */
	void EndScene();
	
	void BeginDrawingViewport(FMetalViewport* Viewport);
	void EndDrawingViewport(FMetalViewport* Viewport, bool bPresent);
	
	/** Take a parallel FMetalContext from the free-list or allocate a new one if required */
	FMetalRHICommandContext* AcquireContext();
	
	/** Release a parallel FMetalContext back into the free-list */
	void ReleaseContext(FMetalRHICommandContext* Context);
	
	/** Returns the number of concurrent contexts encoding commands, including the device context. */
	uint32 GetNumActiveContexts(void) const;
	
	/** Get the index of the bound Metal device in the global list of rendering devices. */
	uint32 GetDeviceIndex(void) const;
	
private:
	FMetalDeviceContext(id<MTLDevice> MetalDevice, uint32 DeviceIndex, FMetalCommandQueue* Queue);
	
private:
	/** The chosen Metal device */
	id<MTLDevice> Device;
	
	/** The index into the GPU device list for the selected Metal device */
	uint32 DeviceIndex;
	
	/** Mutex for access to the unsafe buffer pool */
	FCriticalSection PoolMutex;
	
	/** Dynamic buffer pool */
	FMetalBufferPool BufferPool;
	
	/** Free lists for releasing objects only once it is safe to do so */
	TSet<id> FreeList;
	NSMutableSet<id<MTLBuffer>>* FreeBuffers;
	struct FMetalDelayedFreeList
	{
		dispatch_semaphore_t Signal;
		TSet<id> FreeList;
		NSMutableSet<id<MTLBuffer>>* FreeBuffers;
#if METAL_DEBUG_OPTIONS
		int32 DeferCount;
#endif
	};
	TArray<FMetalDelayedFreeList*> DelayedFreeLists;
	
	/** Free-list of contexts for parallel encoding */
	TLockFreePointerListLIFO<FMetalRHICommandContext> ParallelContexts;
	
	/** Critical section for FreeList */
	FCriticalSection FreeListMutex;
	
	/** Event for coordinating pausing of render thread to keep inline with the ios display link. */
	FEvent* FrameReadyEvent;
	
	/** Internal frame counter, incremented on each call to RHIBeginScene. */
	uint32 SceneFrameCounter;
	
	/** Internal frame counter, used to ensure that we only drain the buffer pool one after each frame within RHIEndFrame. */
	uint32 FrameCounter;
	
	/** Bitfield of supported Metal features with varying availability depending on OS/device */
	uint32 Features;
	
	/** Count of concurrent contexts encoding commands. */
	int32 ActiveContexts;
	
	/** Count of concurrent contexts encoding commands. */
	uint32 AllocatedContexts;
};
