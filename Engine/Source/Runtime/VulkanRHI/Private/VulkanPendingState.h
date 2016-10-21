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
	FVulkanPendingState(FVulkanDevice* InDevice);
	virtual ~FVulkanPendingState();

	inline FVulkanGlobalUniformPool& GetGlobalUniformPool()
	{
		return *GlobalUniformPool;
	}

protected:
	FVulkanDevice* Device;

	FVulkanGlobalUniformPool* GlobalUniformPool;

	friend class FVulkanCommandListContext;
};

class FVulkanPendingComputeState : public FVulkanPendingState
{
public:
	FVulkanPendingComputeState(FVulkanDevice* InDevice)
		: FVulkanPendingState(InDevice)
	{
	}

	inline void SetComputeShader(FVulkanComputeShader* InComputeShader)
	{
		TRefCountPtr<FVulkanComputeShaderState>* Found = ComputeShaderStates.Find(InComputeShader);
		FVulkanComputeShaderState* CSS = nullptr;
		if (Found)
		{
			CSS = *Found;
		}
		else
		{
			CSS = new FVulkanComputeShaderState(Device, InComputeShader);
			ComputeShaderStates.Add(InComputeShader, CSS);
		}

		CSS->ResetState();
		CurrentState.CSS = CSS;
	}

	void PrepareDispatch(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer);

private:
	FVulkanComputePipelineState CurrentState;
	TMap<FVulkanComputeShader*, TRefCountPtr<FVulkanComputeShaderState>> ComputeShaderStates;

	friend class FVulkanCommandListContext;
};

class FVulkanPendingGfxState : public FVulkanPendingState
{
public:
	typedef TMap<FStateKey, FVulkanRenderPass*> FMapRenderPass;
	typedef TMap<FStateKey, TArray<FVulkanFramebuffer*> > FMapFrameBufferArray;

	FVulkanPendingGfxState(FVulkanDevice* InDevice);
	virtual ~FVulkanPendingGfxState();

	void Reset();

	void PrepareDraw(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, VkPrimitiveTopology Topology);

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

	void SetStreamSource(uint32 StreamIndex, FVulkanBuffer* VertexBuffer, uint32 Stride, uint32 Offset)
	{
		PendingStreams[StreamIndex].Stream = VertexBuffer;
		PendingStreams[StreamIndex].Stream2  = nullptr;
		PendingStreams[StreamIndex].Stream3  = VK_NULL_HANDLE;
		PendingStreams[StreamIndex].BufferOffset = Offset;
		if (CurrentState.BSS)
		{
			CurrentState.BSS->MarkDirtyVertexStreams();
		}
	}

	void SetStreamSource(uint32 StreamIndex, FVulkanResourceMultiBuffer* VertexBuffer, uint32 Stride, uint32 Offset)
	{
		PendingStreams[StreamIndex].Stream = nullptr;
		PendingStreams[StreamIndex].Stream2 = VertexBuffer;
		PendingStreams[StreamIndex].Stream3  = VK_NULL_HANDLE;
		PendingStreams[StreamIndex].BufferOffset = Offset;
		if (CurrentState.BSS)
		{
			CurrentState.BSS->MarkDirtyVertexStreams();
		}
	}

	void SetStreamSource(uint32 StreamIndex, VkBuffer InBuffer, uint32 Stride, uint32 Offset)
	{
		PendingStreams[StreamIndex].Stream = nullptr;
		PendingStreams[StreamIndex].Stream2 = nullptr;
		PendingStreams[StreamIndex].Stream3  = InBuffer;
		PendingStreams[StreamIndex].BufferOffset = Offset;
		if (CurrentState.BSS)
		{
			CurrentState.BSS->MarkDirtyVertexStreams();
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
	bool bScissorEnable;

	FVulkanGfxPipelineState CurrentState;

	friend class FVulkanCommandListContext;

	FVertexStream PendingStreams[MaxVertexElementCount];

	// running key of the current pipeline state
	FVulkanPipelineGraphicsKey CurrentKey;
};
