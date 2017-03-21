// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanPendingState.h: Private VulkanPendingState definitions.
=============================================================================*/

#pragma once

// Dependencies
#include "VulkanRHI.h"
#include "VulkanPipeline.h"
#include "VulkanGlobalUniformBuffer.h"
#include "VulkanPipelineState.h"

typedef uint32 FStateKey;

namespace VulkanRHI
{
	enum EStateKey
	{
		NUMBITS_BLEND_STATE				= 6,
		NUMBITS_RENDER_TARGET_FORMAT	= 4,
		NUMBITS_LOAD_OP					= 2,
		NUMBITS_STORE_OP				= 2,
		NUMBITS_CULL_MODE				= 2,
		NUMBITS_POLYFILL				= 1,
		NUMBITS_POLYTYPE				= 3,
		NUMBITS_DEPTH_COMPARE_OP		= 3,
		NUMBITS_STENCIL_STATE			= 5,
		NUMBITS_NUM_COLOR_BLENDS		= 3,
		NUMBITS_NUM_SAMPLES_MINUS_ONE	= 4,	// 0..15 maps to 1..16

		OFFSET_BLEND_STATE				= 0,
		OFFSET_RENDER_TARGET_FORMAT0	= OFFSET_BLEND_STATE + NUMBITS_BLEND_STATE,
		OFFSET_RENDER_TARGET_FORMAT1	= OFFSET_RENDER_TARGET_FORMAT0 + NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_RENDER_TARGET_FORMAT2	= OFFSET_RENDER_TARGET_FORMAT1 + NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_RENDER_TARGET_FORMAT3	= OFFSET_RENDER_TARGET_FORMAT2 + NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_RENDER_TARGET_LOAD0		= OFFSET_RENDER_TARGET_FORMAT3 + NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_RENDER_TARGET_LOAD1		= OFFSET_RENDER_TARGET_LOAD0 + NUMBITS_LOAD_OP,
		OFFSET_RENDER_TARGET_LOAD2		= OFFSET_RENDER_TARGET_LOAD1 + NUMBITS_LOAD_OP,
		OFFSET_RENDER_TARGET_LOAD3		= OFFSET_RENDER_TARGET_LOAD2 + NUMBITS_LOAD_OP,
		OFFSET_RENDER_TARGET_STORE0		= OFFSET_RENDER_TARGET_LOAD3 + NUMBITS_LOAD_OP,
		OFFSET_RENDER_TARGET_STORE1		= OFFSET_RENDER_TARGET_STORE0 + NUMBITS_STORE_OP,
		OFFSET_RENDER_TARGET_STORE2		= OFFSET_RENDER_TARGET_STORE1 + NUMBITS_STORE_OP,
		OFFSET_RENDER_TARGET_STORE3		= OFFSET_RENDER_TARGET_STORE2 + NUMBITS_STORE_OP,

		OFFSET_CULL_MODE				= OFFSET_RENDER_TARGET_STORE3 + NUMBITS_STORE_OP,
		OFFSET_POLYFILL					= OFFSET_CULL_MODE + NUMBITS_CULL_MODE,
		OFFSET_POLYTYPE					= OFFSET_POLYFILL + NUMBITS_POLYFILL,
		OFFSET_DEPTH_BIAS_ENABLED		= OFFSET_POLYTYPE + NUMBITS_POLYTYPE,
		OFFSET_DEPTH_TEST_ENABLED		= OFFSET_DEPTH_BIAS_ENABLED + 1,
		OFFSET_DEPTH_WRITE_ENABLED		= OFFSET_DEPTH_TEST_ENABLED + 1,
		OFFSET_DEPTH_COMPARE_OP			= OFFSET_DEPTH_WRITE_ENABLED + 1,
		OFFSET_FRONT_STENCIL_STATE		= OFFSET_DEPTH_COMPARE_OP + NUMBITS_DEPTH_COMPARE_OP,

		OFFSET_DEPTH_STENCIL_FORMAT		= OFFSET_FRONT_STENCIL_STATE + NUMBITS_STENCIL_STATE,
		OFFSET_DEPTH_STENCIL_LOAD		= OFFSET_DEPTH_STENCIL_FORMAT + NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_DEPTH_STENCIL_STORE		= OFFSET_DEPTH_STENCIL_LOAD + NUMBITS_LOAD_OP,

		OFFSET_RENDER_TARGET_FORMAT4	= 0x8000,
		OFFSET_RENDER_TARGET_FORMAT5	= OFFSET_RENDER_TARGET_FORMAT4	+ NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_RENDER_TARGET_FORMAT6	= OFFSET_RENDER_TARGET_FORMAT5	+ NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_RENDER_TARGET_FORMAT7	= OFFSET_RENDER_TARGET_FORMAT6	+ NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_RENDER_TARGET_LOAD4		= OFFSET_RENDER_TARGET_FORMAT7	+ NUMBITS_RENDER_TARGET_FORMAT,
		OFFSET_RENDER_TARGET_LOAD5		= OFFSET_RENDER_TARGET_LOAD4	+ NUMBITS_LOAD_OP,
		OFFSET_RENDER_TARGET_LOAD6		= OFFSET_RENDER_TARGET_LOAD5	+ NUMBITS_LOAD_OP,
		OFFSET_RENDER_TARGET_LOAD7		= OFFSET_RENDER_TARGET_LOAD6	+ NUMBITS_LOAD_OP,
		OFFSET_RENDER_TARGET_STORE4		= OFFSET_RENDER_TARGET_LOAD7	+ NUMBITS_LOAD_OP,
		OFFSET_RENDER_TARGET_STORE5		= OFFSET_RENDER_TARGET_STORE4	+ NUMBITS_STORE_OP,
		OFFSET_RENDER_TARGET_STORE6		= OFFSET_RENDER_TARGET_STORE5	+ NUMBITS_STORE_OP,
		OFFSET_RENDER_TARGET_STORE7		= OFFSET_RENDER_TARGET_STORE6	+ NUMBITS_STORE_OP,
		OFFSET_BACK_STENCIL_STATE		= OFFSET_RENDER_TARGET_STORE7	+ NUMBITS_STORE_OP,
		OFFSET_STENCIL_TEST_ENABLED		= OFFSET_BACK_STENCIL_STATE		+ NUMBITS_STENCIL_STATE,
		OFFSET_NUM_SAMPLES_MINUS_ONE	= OFFSET_STENCIL_TEST_ENABLED	+ 1,
		OFFSET_NUM_COLOR_BLENDS			= OFFSET_NUM_SAMPLES_MINUS_ONE	+ NUMBITS_NUM_SAMPLES_MINUS_ONE,
	};

	static_assert(OFFSET_DEPTH_STENCIL_STORE + NUMBITS_STORE_OP <= 64, "Out of bits!");
	static_assert(((OFFSET_NUM_COLOR_BLENDS + NUMBITS_NUM_COLOR_BLENDS) & ~0x8000) <= 64, "Out of bits!");

	static FORCEINLINE uint64* GetKey(FVulkanPipelineGraphicsKey& Key, uint64 Offset)
	{
		return Key.Key + (((Offset & 0x8000) != 0) ? 1 : 0);
	}

	static FORCEINLINE void SetKeyBits(FVulkanPipelineGraphicsKey& Key, uint64 Offset, uint64 NumBits, uint64 Value)
	{
		uint64& CurrentKey = *GetKey(Key, Offset);
		Offset = (Offset & ~0x8000);
		const uint64 BitMask = ((1ULL << NumBits) - 1) << Offset;
		CurrentKey = (CurrentKey & ~BitMask) | (((uint64)(Value) << Offset) & BitMask); \
	}

	static const uint32 RTFormatBitOffsets[MaxSimultaneousRenderTargets] =
	{
		OFFSET_RENDER_TARGET_FORMAT0,
		OFFSET_RENDER_TARGET_FORMAT1,
		OFFSET_RENDER_TARGET_FORMAT2,
		OFFSET_RENDER_TARGET_FORMAT3,
		OFFSET_RENDER_TARGET_FORMAT4,
		OFFSET_RENDER_TARGET_FORMAT5,
		OFFSET_RENDER_TARGET_FORMAT6,
		OFFSET_RENDER_TARGET_FORMAT7
	};
	static const uint32 RTLoadBitOffsets[MaxSimultaneousRenderTargets] =
	{
		OFFSET_RENDER_TARGET_LOAD0,
		OFFSET_RENDER_TARGET_LOAD1,
		OFFSET_RENDER_TARGET_LOAD2,
		OFFSET_RENDER_TARGET_LOAD3,
		OFFSET_RENDER_TARGET_LOAD4,
		OFFSET_RENDER_TARGET_LOAD5,
		OFFSET_RENDER_TARGET_LOAD6,
		OFFSET_RENDER_TARGET_LOAD7
	};
	static const uint32 RTStoreBitOffsets[MaxSimultaneousRenderTargets] =
	{
		OFFSET_RENDER_TARGET_STORE0,
		OFFSET_RENDER_TARGET_STORE1,
		OFFSET_RENDER_TARGET_STORE2,
		OFFSET_RENDER_TARGET_STORE3,
		OFFSET_RENDER_TARGET_STORE4,
		OFFSET_RENDER_TARGET_STORE5,
		OFFSET_RENDER_TARGET_STORE6,
		OFFSET_RENDER_TARGET_STORE7
	};
};

// All the current compute pipeline states in use
class FVulkanPendingComputeState : public VulkanRHI::FDeviceChild
{
public:
	FVulkanPendingComputeState(FVulkanDevice* InDevice)
		: VulkanRHI::FDeviceChild(InDevice)
	{
	}

	~FVulkanPendingComputeState()
	{
		//#todo-rco: Delete all
		ensure(0);
	}

	inline FVulkanGlobalUniformPool& GetGlobalUniformPool()
	{
		return GlobalUniformPool;
	}

	void SetComputePipeline(FVulkanComputePipeline* InComputePipeline)
	{
		if (InComputePipeline != CurrentPipeline)
		{
			CurrentPipeline = InComputePipeline;
			FVulkanComputePipelineState** Found = PipelineStates.Find(InComputePipeline);
			if (Found)
			{
				CurrentState = *Found;
			}
			else
			{
				CurrentState = new FVulkanComputePipelineState(Device, InComputePipeline);
				PipelineStates.Add(CurrentPipeline, CurrentState);
			}

			CurrentState->DSRingBuffer.Reset();
		}
	}

	void PrepareForDispatch(FVulkanCommandListContext* Context, FVulkanCmdBuffer* CmdBuffer);

	inline const FVulkanComputeShader* GetCurrentShader() const
	{
		return CurrentPipeline ? CurrentPipeline->GetShader() : nullptr;
	}

	inline void AddUAVForAutoFlush(FVulkanUnorderedAccessView* UAV)
	{
		UAVListForAutoFlush.Add(UAV);
	}

	inline void SetUAV(uint32 UAVIndex, FVulkanUnorderedAccessView* UAV)
	{
		if (UAV)
		{
			// make sure any dynamically backed UAV points to current memory
			UAV->UpdateView();
			if (UAV->BufferView)
			{
				CurrentState->SetUAVBufferViewState(UAVIndex, UAV->BufferView);
			}
			else if (UAV->SourceTexture)
			{
				CurrentState->SetUAVTextureView(UAVIndex, UAV->TextureView);
			}
			else
			{
				ensure(0);
			}
		}
	}

	inline void SetTexture(uint32 BindPoint, const FVulkanTextureBase* TextureBase)
	{
		CurrentState->SetTexture(BindPoint, TextureBase);
	}

	void SetSRV(uint32 TextureIndex, FVulkanShaderResourceView* SRV)
	{
		if (SRV)
		{
			// make sure any dynamically backed SRV points to current memory
			SRV->UpdateView();
			if (SRV->BufferView)
			{
				checkf(SRV->BufferView != VK_NULL_HANDLE, TEXT("Empty SRV"));
				CurrentState->SetSRVBufferViewState(TextureIndex, SRV->BufferView);
			}
			else
			{
				checkf(SRV->TextureView.View != VK_NULL_HANDLE, TEXT("Empty SRV"));
				CurrentState->SetSRVTextureView(TextureIndex, SRV->TextureView);
			}
		}
		else
		{
			CurrentState->SetSRVBufferViewState(TextureIndex, nullptr);
		}
	}

	inline void SetShaderParameter(uint32 BufferIndex, uint32 ByteOffset, uint32 NumBytes, const void* NewValue)
	{
		CurrentState->SetShaderParameter(BufferIndex, ByteOffset, NumBytes, NewValue);
	}

	inline void SetUniformBufferConstantData(uint32 BindPoint, const TArray<uint8>& ConstantData)
	{
		CurrentState->SetUniformBufferConstantData(BindPoint, ConstantData);
	}

	inline void SetSamplerState(uint32 BindPoint, FVulkanSamplerState* Sampler)
	{
		CurrentState->SetSamplerState(BindPoint, Sampler);
	}

	//#todo-rco: Move to pipeline cache
	FVulkanComputePipeline* GetOrCreateComputePipeline(FVulkanComputeShader* ComputeShader);

protected:
	FVulkanGlobalUniformPool GlobalUniformPool;
	TArray<FVulkanUnorderedAccessView*> UAVListForAutoFlush;

	FVulkanComputePipeline* CurrentPipeline;
	FVulkanComputePipelineState* CurrentState;

	TMap<FVulkanComputePipeline*, FVulkanComputePipelineState*> PipelineStates;

	//#todo-rco: Move to pipeline cache
	TMap<FVulkanComputeShader*, FVulkanComputePipeline*> ComputePipelineCache;

	friend class FVulkanCommandListContext;
};

class FOLDVulkanPendingGfxState
{
public:
	typedef TMap<FStateKey, FVulkanRenderPass*> FMapRenderPass;
	typedef TMap<FStateKey, TArray<FVulkanFramebuffer*> > FMapFrameBufferArray;

	FOLDVulkanPendingGfxState(FVulkanDevice* InDevice);

	void Reset();

	void PrepareDraw(FVulkanCommandListContext* CmdListContext, FVulkanCmdBuffer* CmdBuffer, VkPrimitiveTopology Topology);

	inline FVulkanGlobalUniformPool& GetGlobalUniformPool()
	{
		return GlobalUniformPool;
	}

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
	FVulkanGlobalUniformPool GlobalUniformPool;

	friend class FVulkanCommandListContext;

	FVertexStream PendingStreams[MaxVertexElementCount];

	// running key of the current pipeline state
	FVulkanPipelineGraphicsKey CurrentKey;
};
