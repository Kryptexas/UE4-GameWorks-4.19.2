// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanPendingState.h: Private VulkanPendingState definitions.
=============================================================================*/

#pragma once

// Dependencies
#include "VulkanRHI.h"
#include "VulkanPipeline.h"

typedef uint32 FStateKey;

class FVulkanPendingState
{
public:
	typedef TMap<FStateKey, FVulkanRenderPass*> FMapRenderPass;
	typedef TMap<FStateKey, TArray<FVulkanFramebuffer*> > FMapFrameBufferArray;

	FVulkanPendingState(FVulkanDevice* InDevice);

	~FVulkanPendingState();

    FVulkanGlobalUniformPool& GetGlobalUniformPool();

#if !VULKAN_USE_NEW_RENDERPASSES
	void SetRenderTargetsInfo(const FRHISetRenderTargetsInfo& InRTInfo);
#endif

	void Reset();

#if !VULKAN_USE_NEW_RENDERPASSES
	bool RenderPassBegin(FVulkanCmdBuffer* CmdBuffer);
#endif

	void PrepareDraw(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, VkPrimitiveTopology Topology);

#if !VULKAN_USE_NEW_RENDERPASSES
	void RenderPassEnd(FVulkanCmdBuffer* CmdBuffer);

	inline bool IsRenderPassActive() const
	{
		return bBeginRenderPass;
	}
#endif
	
	void SetBoundShaderState(TRefCountPtr<FVulkanBoundShaderState> InBoundShaderState);

	// Pipeline states
	void SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ);
	void SetScissor(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY);
	void SetScissorRect(uint32 MinX, uint32 MinY, uint32 Width, uint32 Height);
	void SetBlendState(FVulkanBlendState* NewState);
	void SetDepthStencilState(FVulkanDepthStencilState* NewState, uint32 StencilRef);
	void SetRasterizerState(FVulkanRasterizerState* NewState);

	// Returns shader state
	// TODO: Move binding/layout functionality to the shader (who owns what)?
	FVulkanBoundShaderState& GetBoundShaderState();

#if !VULKAN_USE_NEW_RENDERPASSES
	// Retuns constructed render pass
	inline FVulkanRenderPass& GetRenderPass()
	{
		check(CurrentState.RenderPass);
		return *CurrentState.RenderPass;
	}
#endif

#if !VULKAN_USE_NEW_RENDERPASSES
	inline FVulkanFramebuffer* GetFrameBuffer()
	{
		return CurrentState.FrameBuffer;
	}
#endif

	void NotifyDeletedRenderTarget(const FVulkanTextureBase* Texture);

#if !VULKAN_USE_NEW_RENDERPASSES
	inline void UpdateRenderPass(FVulkanCmdBuffer* CmdBuffer)
	{
		//#todo-rco: Don't test here, move earlier to SetRenderTarget
		if (bChangeRenderTarget)
		{
			if (bBeginRenderPass)
			{
				RenderPassEnd(CmdBuffer);
			}

			if (!RenderPassBegin(CmdBuffer))
			{
				return;
			}
			bChangeRenderTarget = false;
		}
		if (!bBeginRenderPass)
		{
			RenderPassBegin(CmdBuffer);
			bChangeRenderTarget = false;
		}
	}
#endif

	void SetStreamSource(uint32 StreamIndex, FVulkanBuffer* VertexBuffer, uint32 Stride, uint32 Offset)
	{
		PendingStreams[StreamIndex].Stream = VertexBuffer;
		PendingStreams[StreamIndex].Stream2  = nullptr;
		PendingStreams[StreamIndex].Stream3  = VK_NULL_HANDLE;
		PendingStreams[StreamIndex].BufferOffset = Offset;
		if (CurrentState.Shader)
		{
			CurrentState.Shader->MarkDirtyVertexStreams();
		}
	}

	void SetStreamSource(uint32 StreamIndex, FVulkanResourceMultiBuffer* VertexBuffer, uint32 Stride, uint32 Offset)
	{
		PendingStreams[StreamIndex].Stream = nullptr;
		PendingStreams[StreamIndex].Stream2 = VertexBuffer;
		PendingStreams[StreamIndex].Stream3  = VK_NULL_HANDLE;
		PendingStreams[StreamIndex].BufferOffset = Offset;
		if (CurrentState.Shader)
		{
			CurrentState.Shader->MarkDirtyVertexStreams();
		}
	}

	void SetStreamSource(uint32 StreamIndex, VkBuffer InBuffer, uint32 Stride, uint32 Offset)
	{
		PendingStreams[StreamIndex].Stream = nullptr;
		PendingStreams[StreamIndex].Stream2 = nullptr;
		PendingStreams[StreamIndex].Stream3  = InBuffer;
		PendingStreams[StreamIndex].BufferOffset = Offset;
		if (CurrentState.Shader)
		{
			CurrentState.Shader->MarkDirtyVertexStreams();
		}
	}

	struct FVertexStream
	{
		FVertexStream() : Stream(nullptr), Stream2(nullptr), Stream3(VK_NULL_HANDLE), BufferOffset(0) {}
		FVulkanBuffer* Stream;
		FVulkanResourceMultiBuffer* Stream2;
		VkBuffer Stream3;
		uint32 BufferOffset;
	};

	inline const FVertexStream* GetVertexStreams() const
	{
		return PendingStreams;
	}

	void InitFrame();

private:
#if !VULKAN_USE_NEW_RENDERPASSES
	FVulkanRenderPass* GetOrCreateRenderPass(const FVulkanRenderTargetLayout& RTLayout);
#endif
	FVulkanFramebuffer* GetOrCreateFramebuffer(const FRHISetRenderTargetsInfo& RHIRTInfo,
		const FVulkanRenderTargetLayout& RTInfo, const FVulkanRenderPass& RenderPass);

#if !VULKAN_USE_NEW_RENDERPASSES
	bool NeedsToSetRenderTarget(const FRHISetRenderTargetsInfo& InRTInfo);
#endif

private:
	FVulkanDevice* Device;

#if !VULKAN_USE_NEW_RENDERPASSES
	bool bBeginRenderPass;
	bool bChangeRenderTarget;
#endif
	bool bScissorEnable;

	FVulkanGlobalUniformPool* GlobalUniformPool;

	FVulkanPipelineState CurrentState;

#if !VULKAN_USE_NEW_RENDERPASSES
	//@TODO: probably needs to go somewhere else
	FRHISetRenderTargetsInfo PrevRenderTargetsInfo;
#endif

	friend class FVulkanCommandListContext;

#if !VULKAN_USE_NEW_RENDERPASSES
	FRHISetRenderTargetsInfo RTInfo;

	// Resources caching
	FMapRenderPass RenderPassMap;
#endif
	FMapFrameBufferArray FrameBufferMap;

	FVertexStream PendingStreams[MaxVertexElementCount];

	// running key of the current pipeline state
	FVulkanPipelineGraphicsKey CurrentKey;

	// bResetMap true if only reset the map, false to free the map's memory
	void DestroyFrameBuffers(bool bResetMap);
};
