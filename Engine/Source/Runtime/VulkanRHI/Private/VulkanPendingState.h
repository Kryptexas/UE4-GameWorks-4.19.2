// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanPendingState.h: Private VulkanPendingState definitions.
=============================================================================*/

#pragma once

// Dependencies
#include "VulkanRHI.h"
#include "VulkanPipeline.h"

class FVulkanPendingState
{
public:
	typedef TMap<FStateKey, FVulkanRenderPass*> FMapRenderRass;
	typedef TMap<FStateKey, TArray<FVulkanFramebuffer*> > FMapFrameBufferArray;

	FVulkanPendingState(FVulkanDevice* InDevice);

	~FVulkanPendingState();

	FVulkanDescriptorSets* AllocateDescriptorSet(const FVulkanBoundShaderState* BoundShaderState);
	void DeallocateDescriptorSet(FVulkanDescriptorSets*& DescriptorSet, const FVulkanBoundShaderState* BoundShaderState);

    FVulkanGlobalUniformPool& GetGlobalUniformPool();

	void SetRenderTargetsInfo(const FRHISetRenderTargetsInfo& InRTInfo);

	void Reset();

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	void RenderPassBegin(FVulkanCmdBuffer* CmdBuffer);

	void PrepareDraw(FVulkanCmdBuffer* CmdBuffer, VkPrimitiveTopology Topology);

	void RenderPassEnd(FVulkanCmdBuffer* CmdBuffer);
#else
	// Needs to be called before "PrepareDraw" and also starts writing in to the command buffer
	void RenderPassBegin();

	// Needs to be called before any calling a draw-call
	// Binds graphics pipeline
	void PrepareDraw(VkPrimitiveTopology Topology);

	// Is currently linked to the command buffer
	// Will get called in RHIEndDrawingViewport through SubmitPendingCommandBuffers
	void RenderPassEnd();
#endif

	inline bool IsRenderPassActive() const
	{
		return bBeginRenderPass;
	}
	
	void SetBoundShaderState(TRefCountPtr<FVulkanBoundShaderState> InBoundShaderState);

	//#todo-rco: Temp hack...
	typedef void (TCallback)(void*);
	void SubmitPendingCommandBuffers(TCallback* Callback, void* CallbackUserData);

	void SubmitPendingCommandBuffersBlockingNoRenderPass();

	// Pipeline states
	void SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ);
	void SetScissor(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY);
	void SetScissorRect(uint32 MinX, uint32 MinY, uint32 Width, uint32 Height);
	void SetBlendState(FVulkanBlendState* NewState);
	void SetDepthStencilState(FVulkanDepthStencilState* NewState);
	void SetRasterizerState(FVulkanRasterizerState* NewState);

	// Returns shader state
	// TODO: Move binding/layout functionality to the shader (who owns what)?
	FVulkanBoundShaderState& GetBoundShaderState();

	// Retuns constructed render pass
	FVulkanRenderPass& GetRenderPass();

#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
	// Returns currently bound command buffer
	VkCommandBuffer& GetCommandBuffer();
	const VkBool32 GetIsCommandBufferEmpty() const;
#endif
	FVulkanFramebuffer* GetFrameBuffer();

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	inline void UpdateRenderPass(FVulkanCmdBuffer* CmdBuffer)
	{
		//#todo-rco: Don't test here, move earlier to SetRenderTarget
		if (bChangeRenderTarget)
		{
			if (bBeginRenderPass)
			{
				RenderPassEnd(CmdBuffer);
			}

			RenderPassBegin(CmdBuffer);
			bChangeRenderTarget = false;
		}
		if (!bBeginRenderPass)
		{
			RenderPassBegin(CmdBuffer);
			bChangeRenderTarget = false;
		}
	}
#else
	inline void UpdateRenderPass()
	{
		//#todo-rco: Don't test here, move earlier to SetRenderTarget
		if (bChangeRenderTarget)
		{
			if (bBeginRenderPass)
			{
				RenderPassEnd();
			}

			RenderPassBegin();
			bChangeRenderTarget = false;
		}
	}
#endif

	inline const FVulkanPipelineGraphicsKey& GetCurrentKey() const
	{
		return CurrentKey;
	}

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

#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
	inline FVulkanCmdBuffer& GetCommandBuffer(uint32 Index)
	{
		check(Index<VULKAN_NUM_COMMAND_BUFFERS);
		check(CmdBuffers[Index]);
		return *CmdBuffers[Index];
	}

	inline uint32 GetCurrentCommandBufferIndex() const
	{
		return CurrentCmdBufferIndex;
	}

	// Returns current frame command buffer
	// The index of the command buffer changes after "Reset()" is called
	inline FVulkanCmdBuffer& GetCurrentCommandBuffer()
	{
		check(CmdBuffers[CurrentCmdBufferIndex]);
		return *CmdBuffers[CurrentCmdBufferIndex];
	}

	// Returns current frame command buffer
	// The index of the command buffer changes after "Reset()" is called
	inline const FVulkanCmdBuffer& GetCurrentCommandBuffer() const
	{
		check(CmdBuffers[CurrentCmdBufferIndex]);
		return *CmdBuffers[CurrentCmdBufferIndex];
	}
#endif
private:
	FVulkanRenderPass* GetOrCreateRenderPass(const FVulkanRenderTargetLayout& RTLayout);
	FVulkanFramebuffer* GetOrCreateFramebuffer(const FRHISetRenderTargetsInfo& RHIRTInfo,
		const FVulkanRenderTargetLayout& RTInfo, const FVulkanRenderPass& RenderPass);

	bool NeedsToSetRenderTarget(const FRHISetRenderTargetsInfo& InRTInfo);

private:
	FVulkanDevice* Device;

	bool bBeginRenderPass;
	bool bChangeRenderTarget;
	bool bScissorEnable;

	FVulkanGlobalUniformPool* GlobalUniformPool;

	FVulkanPipelineState CurrentState;

	//@TODO: probably needs to go somewhere else
	FRHISetRenderTargetsInfo PrevRenderTargetsInfo;

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	friend class FVulkanCommandListContext;
#else
	//#todo-rco: FIX ME! ASAP!!!
	FVulkanCmdBuffer* CmdBuffers[VULKAN_NUM_COMMAND_BUFFERS];
	uint32	CurrentCmdBufferIndex;
#endif
	
	FRHISetRenderTargetsInfo RTInfo;

	// Resources caching
	FMapRenderRass RenderPassMap;
	FMapFrameBufferArray FrameBufferMap;

	FVertexStream PendingStreams[MaxVertexElementCount];

	// running key of the current pipeline state
	FVulkanPipelineGraphicsKey CurrentKey;
};
