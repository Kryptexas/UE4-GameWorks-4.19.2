// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHICommandList.h: RHI Command List definitions for queueing up & executing later.
=============================================================================*/

#pragma once
#include "LockFreeFixedSizeAllocator.h"
#include "TaskGraphInterfaces.h"

DECLARE_STATS_GROUP(TEXT("RHICmdList"), STATGROUP_RHICMDLIST, STATCAT_Advanced);

#define USE_RHICOMMAND_STATE_REDUCTION (0)

extern RHI_API TAutoConsoleVariable<int32> CVarRHICmdWidth;

struct FRHICommandBase
{
	FRHICommandBase* Next;
	void(*ExecuteAndDestructPtr)(FRHICommandBase *Cmd);
	FORCEINLINE FRHICommandBase(void(*InExecuteAndDestructPtr)(FRHICommandBase *Cmd))
		: Next(nullptr)
		, ExecuteAndDestructPtr(InExecuteAndDestructPtr)
	{
	}
	FORCEINLINE void CallExecuteAndDestruct()
	{
		ExecuteAndDestructPtr(this);
	}
};

class RHI_API FRHICommandListBase : public FNoncopyable
{
public:

	FRHICommandListBase();
	~FRHICommandListBase();

	inline void Flush();
	inline bool IsImmediate();
	const int32 GetUsedMemory() const;
	void QueueAsyncCommandListSubmit(FGraphEventRef& AnyThreadCompletionEvent, class FRHICommandList* CmdList);
	void QueueRenderThreadCommandListSubmit(FGraphEventRef& RenderThreadCompletionEvent, class FRHICommandList* CmdList);
	void QueueCommandListSubmit(class FRHICommandList* CmdList);
	void WaitForTasks(bool bKnownToBeComplete = false);
	void WaitForRHIThreadTasks();
	void HandleRTThreadTaskCompletion(const FGraphEventRef& MyCompletionGraphEvent);

	FORCEINLINE_DEBUGGABLE void* Alloc(int32 AllocSize, int32 Alignment)
	{
		return MemManager.Alloc(AllocSize, Alignment);
	}

	template <typename T>
	FORCEINLINE_DEBUGGABLE void* Alloc()
	{
		return MemManager.Alloc(sizeof(T), ALIGNOF(T));
	}

	template <typename TCmd>
	FORCEINLINE_DEBUGGABLE void* AllocCommand()
	{
		checkSlow(!IsExecuting());
		TCmd* Result = (TCmd*)Alloc<TCmd>();
		++NumCommands;
		*CommandLink = Result;
		CommandLink = &Result->Next;
		return Result;
	}
	FORCEINLINE uint32 GetUID()
	{
		return UID;
	}
	FORCEINLINE bool HasCommands() const
	{
		return (NumCommands > 0);
	}
	FORCEINLINE bool IsExecuting() const
	{
		return bExecuting;
	}

	inline bool Bypass();

	FORCEINLINE void ExchangeCmdList(FRHICommandListBase& Other)
	{
		check(!RTTasks.Num() && !Other.RTTasks.Num());
		FMemory::Memswap(this, &Other, sizeof(FRHICommandListBase));
		if (CommandLink == &Other.Root)
		{
			CommandLink = &Root;
		}
		if (Other.CommandLink == &Root)
		{
			Other.CommandLink = &Other.Root;
		}
	}

	struct FDrawUpData
	{

		uint32 PrimitiveType;
		uint32 NumPrimitives;
		uint32 NumVertices;
		uint32 VertexDataStride;
		void* OutVertexData;
		uint32 MinVertexIndex;
		uint32 NumIndices;
		uint32 IndexDataStride;
		void* OutIndexData;

		FDrawUpData()
			: PrimitiveType(PT_Num)
			, OutVertexData(nullptr)
			, OutIndexData(nullptr)
		{
		}
	};

private:
	FRHICommandBase* Root;
	FRHICommandBase** CommandLink;
	bool bExecuting;
	uint32 NumCommands;
	uint32 UID;
	FMemStackBase MemManager; 
#if USE_RHICOMMAND_STATE_REDUCTION
protected:
	struct FRHICommandListStateCache* StateCache;
private:
#endif
	FGraphEventArray RTTasks;

	void Reset();

	friend class FRHICommandListExecutor;
	friend class FRHICommandListIterator;

public:
	FDrawUpData DrawUPData; 
};

#if USE_RHICOMMAND_STATE_REDUCTION
struct FRHICommandListStateCache
{
	// warning, no destructor is ever called for this struct

	FRHICommandListStateCache()
	{
		FlushShaderState();
		FlushSamplerState();
	}
	enum 
	{
		MAX_SAMPLERS_PER_SHADER_STAGE = 32, // it is okish if this is too small, those buffers simply won't get state reduction
		MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE = 14, // it is okish if this is too small, those buffers simply won't get state reduction
	};
	FSamplerStateRHIParamRef Samplers[SF_NumFrequencies][MAX_SAMPLERS_PER_SHADER_STAGE];
	FUniformBufferRHIParamRef BoundUniformBuffers[SF_NumFrequencies][MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE];

	FORCEINLINE void FlushSamplerState()
	{
		for (int32 Index = 0; Index < SF_NumFrequencies; Index++)
		{
			for (int32 IndexInner = 0; IndexInner < MAX_SAMPLERS_PER_SHADER_STAGE; IndexInner++)
			{
				Samplers[Index][IndexInner] = nullptr;
			}
		}
	}

	FORCEINLINE void FlushShaderState()
	{
		for (int32 Index = 0; Index < SF_NumFrequencies; Index++)
		{
			for (int32 IndexInner = 0; IndexInner < MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE; IndexInner++)
			{
				BoundUniformBuffers[Index][IndexInner] = nullptr;
			}
		}
	}
};
#endif

template<typename TCmd>
struct FRHICommand : public FRHICommandBase
{
	FORCEINLINE FRHICommand()
		: FRHICommandBase(&ExecuteAndDestruct)
	{

	}
	static FORCEINLINE void ExecuteAndDestruct(FRHICommandBase *Cmd)
	{
		TCmd *ThisCmd = (TCmd*)Cmd;
		ThisCmd->Execute();
		ThisCmd->~TCmd();
	}
};

struct FRHICommandSetRasterizerState : public FRHICommand<FRHICommandSetRasterizerState>
{
	FRasterizerStateRHIParamRef State;
	FORCEINLINE_DEBUGGABLE FRHICommandSetRasterizerState(FRasterizerStateRHIParamRef InState)
		: State(InState)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetDepthStencilState : public FRHICommand<FRHICommandSetDepthStencilState>
{
	FDepthStencilStateRHIParamRef State;
	uint32 StencilRef;
	FORCEINLINE_DEBUGGABLE FRHICommandSetDepthStencilState(FDepthStencilStateRHIParamRef InState, uint32 InStencilRef)
		: State(InState)
		, StencilRef(InStencilRef)
	{
	}
	RHI_API void Execute();
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetShaderParameter : public FRHICommand<FRHICommandSetShaderParameter<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	const void* NewValue;
	uint32 BufferIndex;
	uint32 BaseIndex;
	uint32 NumBytes;
	FORCEINLINE_DEBUGGABLE FRHICommandSetShaderParameter(TShaderRHIParamRef InShader, uint32 InBufferIndex, uint32 InBaseIndex, uint32 InNumBytes, const void* InNewValue)
		: Shader(InShader)
		, NewValue(InNewValue)
		, BufferIndex(InBufferIndex)
		, BaseIndex(InBaseIndex)
		, NumBytes(InNumBytes)
	{
	}
	RHI_API void Execute();
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetShaderUniformBuffer : public FRHICommand<FRHICommandSetShaderUniformBuffer<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	uint32 BaseIndex;
	FUniformBufferRHIParamRef UniformBuffer;
	FORCEINLINE_DEBUGGABLE FRHICommandSetShaderUniformBuffer(TShaderRHIParamRef InShader, uint32 InBaseIndex, FUniformBufferRHIParamRef InUniformBuffer)
		: Shader(InShader)
		, BaseIndex(InBaseIndex)
		, UniformBuffer(InUniformBuffer)
	{
	}
	RHI_API void Execute();
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetShaderTexture : public FRHICommand<FRHICommandSetShaderTexture<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	uint32 TextureIndex;
	FTextureRHIParamRef Texture;
	FORCEINLINE_DEBUGGABLE FRHICommandSetShaderTexture(TShaderRHIParamRef InShader, uint32 InTextureIndex, FTextureRHIParamRef InTexture)
		: Shader(InShader)
		, TextureIndex(InTextureIndex)
		, Texture(InTexture)
	{
	}
	RHI_API void Execute();
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetShaderResourceViewParameter : public FRHICommand<FRHICommandSetShaderResourceViewParameter<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	uint32 SamplerIndex;
	FShaderResourceViewRHIParamRef SRV;
	FORCEINLINE_DEBUGGABLE FRHICommandSetShaderResourceViewParameter(TShaderRHIParamRef InShader, uint32 InSamplerIndex, FShaderResourceViewRHIParamRef InSRV)
		: Shader(InShader)
		, SamplerIndex(InSamplerIndex)
		, SRV(InSRV)
	{
	}
	RHI_API void Execute();
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetUAVParameter : public FRHICommand<FRHICommandSetUAVParameter<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	uint32 UAVIndex;
	FUnorderedAccessViewRHIParamRef UAV;
	FORCEINLINE_DEBUGGABLE FRHICommandSetUAVParameter(TShaderRHIParamRef InShader, uint32 InUAVIndex, FUnorderedAccessViewRHIParamRef InUAV)
		: Shader(InShader)
		, UAVIndex(InUAVIndex)
		, UAV(InUAV)
	{
	}
	RHI_API void Execute();
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetUAVParameter_IntialCount : public FRHICommand<FRHICommandSetUAVParameter_IntialCount<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	uint32 UAVIndex;
	FUnorderedAccessViewRHIParamRef UAV;
	uint32 InitialCount;
	FORCEINLINE_DEBUGGABLE FRHICommandSetUAVParameter_IntialCount(TShaderRHIParamRef InShader, uint32 InUAVIndex, FUnorderedAccessViewRHIParamRef InUAV, uint32 InInitialCount)
		: Shader(InShader)
		, UAVIndex(InUAVIndex)
		, UAV(InUAV)
		, InitialCount(InInitialCount)
	{
	}
	RHI_API void Execute();
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetShaderSampler : public FRHICommand<FRHICommandSetShaderSampler<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	uint32 SamplerIndex;
	FSamplerStateRHIParamRef Sampler;
	FORCEINLINE_DEBUGGABLE FRHICommandSetShaderSampler(TShaderRHIParamRef InShader, uint32 InSamplerIndex, FSamplerStateRHIParamRef InSampler)
		: Shader(InShader)
		, SamplerIndex(InSamplerIndex)
		, Sampler(InSampler)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandDrawPrimitive : public FRHICommand<FRHICommandDrawPrimitive>
{
	uint32 PrimitiveType;
	uint32 BaseVertexIndex;
	uint32 NumPrimitives;
	uint32 NumInstances;
	FORCEINLINE_DEBUGGABLE FRHICommandDrawPrimitive(uint32 InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances)
		: PrimitiveType(InPrimitiveType)
		, BaseVertexIndex(InBaseVertexIndex)
		, NumPrimitives(InNumPrimitives)
		, NumInstances(InNumInstances)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandDrawIndexedPrimitive : public FRHICommand<FRHICommandDrawIndexedPrimitive>
{
	FIndexBufferRHIParamRef IndexBuffer;
	uint32 PrimitiveType;
	int32 BaseVertexIndex;
	uint32 MinIndex;
	uint32 NumVertices;
	uint32 StartIndex;
	uint32 NumPrimitives;
	uint32 NumInstances;
	FORCEINLINE_DEBUGGABLE FRHICommandDrawIndexedPrimitive(FIndexBufferRHIParamRef InIndexBuffer, uint32 InPrimitiveType, int32 InBaseVertexIndex, uint32 InMinIndex, uint32 InNumVertices, uint32 InStartIndex, uint32 InNumPrimitives, uint32 InNumInstances)
		: IndexBuffer(InIndexBuffer)
		, PrimitiveType(InPrimitiveType)
		, BaseVertexIndex(InBaseVertexIndex)
		, MinIndex(InMinIndex)
		, NumVertices(InNumVertices)
		, StartIndex(InStartIndex)
		, NumPrimitives(InNumPrimitives)
		, NumInstances(InNumInstances)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetBoundShaderState : public FRHICommand<FRHICommandSetBoundShaderState>
{
	FBoundShaderStateRHIParamRef BoundShaderState;
	FORCEINLINE_DEBUGGABLE FRHICommandSetBoundShaderState(FBoundShaderStateRHIParamRef InBoundShaderState)
		: BoundShaderState(InBoundShaderState)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetBlendState : public FRHICommand<FRHICommandSetBlendState>
{
	FBlendStateRHIParamRef State;
	FLinearColor BlendFactor;
	FORCEINLINE_DEBUGGABLE FRHICommandSetBlendState(FBlendStateRHIParamRef InState, const FLinearColor& InBlendFactor)
		: State(InState)
		, BlendFactor(InBlendFactor)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetStreamSource : public FRHICommand<FRHICommandSetStreamSource>
{
	uint32 StreamIndex;
	FVertexBufferRHIParamRef VertexBuffer;
	uint32 Stride;
	uint32 Offset;
	FORCEINLINE_DEBUGGABLE FRHICommandSetStreamSource(uint32 InStreamIndex, FVertexBufferRHIParamRef InVertexBuffer, uint32 InStride, uint32 InOffset)
		: StreamIndex(InStreamIndex)
		, VertexBuffer(InVertexBuffer)
		, Stride(InStride)
		, Offset(InOffset)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetViewport : public FRHICommand<FRHICommandSetViewport>
{
	uint32 MinX;
	uint32 MinY;
	float MinZ;
	uint32 MaxX;
	uint32 MaxY;
	float MaxZ;
	FORCEINLINE_DEBUGGABLE FRHICommandSetViewport(uint32 InMinX, uint32 InMinY, float InMinZ, uint32 InMaxX, uint32 InMaxY, float InMaxZ)
		: MinX(InMinX)
		, MinY(InMinY)
		, MinZ(InMinZ)
		, MaxX(InMaxX)
		, MaxY(InMaxY)
		, MaxZ(InMaxZ)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetScissorRect : public FRHICommand<FRHICommandSetScissorRect>
{
	bool bEnable;
	uint32 MinX;
	uint32 MinY;
	uint32 MaxX;
	uint32 MaxY;
	FORCEINLINE_DEBUGGABLE FRHICommandSetScissorRect(bool InbEnable, uint32 InMinX, uint32 InMinY, uint32 InMaxX, uint32 InMaxY)
		: bEnable(InbEnable)
		, MinX(InMinX)
		, MinY(InMinY)
		, MaxX(InMaxX)
		, MaxY(InMaxY)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetRenderTargets : public FRHICommand<FRHICommandSetRenderTargets>
{
	uint32 NewNumSimultaneousRenderTargets;
	FRHIRenderTargetView NewRenderTargetsRHI[MaxSimultaneousRenderTargets];
	FTextureRHIParamRef NewDepthStencilTargetRHI;
	uint32 NewNumUAVs;
	FUnorderedAccessViewRHIParamRef UAVs[MaxSimultaneousUAVs];

	FORCEINLINE_DEBUGGABLE FRHICommandSetRenderTargets(
		uint32 InNewNumSimultaneousRenderTargets,
		const FRHIRenderTargetView* InNewRenderTargetsRHI,
		FTextureRHIParamRef InNewDepthStencilTargetRHI,
		uint32 InNewNumUAVs,
		const FUnorderedAccessViewRHIParamRef* InUAVs
		)
		: NewNumSimultaneousRenderTargets(InNewNumSimultaneousRenderTargets)
		, NewDepthStencilTargetRHI(InNewDepthStencilTargetRHI)
		, NewNumUAVs(InNewNumUAVs)

	{
		check(InNewNumSimultaneousRenderTargets <= MaxSimultaneousRenderTargets && InNewNumUAVs <= MaxSimultaneousUAVs);
		for (uint32 Index = 0; Index < NewNumSimultaneousRenderTargets; Index++)
		{
			NewRenderTargetsRHI[Index] = InNewRenderTargetsRHI[Index];
		}
		for (uint32 Index = 0; Index < NewNumUAVs; Index++)
		{
			UAVs[Index] = InUAVs[Index];
		}
	}
	RHI_API void Execute();
};

struct FRHICommandEndDrawPrimitiveUP : public FRHICommand<FRHICommandEndDrawPrimitiveUP>
{
	uint32 PrimitiveType;
	uint32 NumPrimitives;
	uint32 NumVertices;
	uint32 VertexDataStride;
	void* OutVertexData;

	FORCEINLINE_DEBUGGABLE FRHICommandEndDrawPrimitiveUP(uint32 InPrimitiveType, uint32 InNumPrimitives, uint32 InNumVertices, uint32 InVertexDataStride, void* InOutVertexData)
		: PrimitiveType(InPrimitiveType)
		, NumPrimitives(InNumPrimitives)
		, NumVertices(InNumVertices)
		, VertexDataStride(InVertexDataStride)
		, OutVertexData(InOutVertexData)
	{
	}
	RHI_API void Execute();
};


struct FRHICommandEndDrawIndexedPrimitiveUP : public FRHICommand<FRHICommandEndDrawIndexedPrimitiveUP>
{
	uint32 PrimitiveType;
	uint32 NumPrimitives;
	uint32 NumVertices;
	uint32 VertexDataStride;
	void* OutVertexData;
	uint32 MinVertexIndex;
	uint32 NumIndices;
	uint32 IndexDataStride;
	void* OutIndexData;

	FORCEINLINE_DEBUGGABLE FRHICommandEndDrawIndexedPrimitiveUP(uint32 InPrimitiveType, uint32 InNumPrimitives, uint32 InNumVertices, uint32 InVertexDataStride, void* InOutVertexData, uint32 InMinVertexIndex, uint32 InNumIndices, uint32 InIndexDataStride, void* InOutIndexData)
		: PrimitiveType(InPrimitiveType)
		, NumPrimitives(InNumPrimitives)
		, NumVertices(InNumVertices)
		, VertexDataStride(InVertexDataStride)
		, OutVertexData(InOutVertexData)
		, MinVertexIndex(InMinVertexIndex)
		, NumIndices(InNumIndices)
		, IndexDataStride(InIndexDataStride)
		, OutIndexData(InOutIndexData)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetComputeShader : public FRHICommand<FRHICommandSetComputeShader>
{
	FComputeShaderRHIParamRef ComputeShader;
	FORCEINLINE_DEBUGGABLE FRHICommandSetComputeShader(FComputeShaderRHIParamRef InComputeShader)
		: ComputeShader(InComputeShader)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandDispatchComputeShader : public FRHICommand<FRHICommandDispatchComputeShader>
{
	uint32 ThreadGroupCountX;
	uint32 ThreadGroupCountY;
	uint32 ThreadGroupCountZ;
	FORCEINLINE_DEBUGGABLE FRHICommandDispatchComputeShader(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
		: ThreadGroupCountX(InThreadGroupCountX)
		, ThreadGroupCountY(InThreadGroupCountY)
		, ThreadGroupCountZ(InThreadGroupCountZ)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandDispatchIndirectComputeShader : public FRHICommand<FRHICommandDispatchIndirectComputeShader>
{
	FVertexBufferRHIParamRef ArgumentBuffer;
	uint32 ArgumentOffset;
	FORCEINLINE_DEBUGGABLE FRHICommandDispatchIndirectComputeShader(FVertexBufferRHIParamRef InArgumentBuffer, uint32 InArgumentOffset)
		: ArgumentBuffer(InArgumentBuffer)
		, ArgumentOffset(InArgumentOffset)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandAutomaticCacheFlushAfterComputeShader : public FRHICommand<FRHICommandAutomaticCacheFlushAfterComputeShader>
{
	bool bEnable;
	FORCEINLINE_DEBUGGABLE FRHICommandAutomaticCacheFlushAfterComputeShader(bool InbEnable)
		: bEnable(InbEnable)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandFlushComputeShaderCache : public FRHICommand<FRHICommandFlushComputeShaderCache>
{
	RHI_API void Execute();
};

struct FRHICommandDrawPrimitiveIndirect : public FRHICommand<FRHICommandDrawPrimitiveIndirect>
{
	uint32 PrimitiveType;
	FVertexBufferRHIParamRef ArgumentBuffer;
	uint32 ArgumentOffset;
	FORCEINLINE_DEBUGGABLE FRHICommandDrawPrimitiveIndirect(uint32 InPrimitiveType, FVertexBufferRHIParamRef InArgumentBuffer, uint32 InArgumentOffset)
		: PrimitiveType(InPrimitiveType)
		, ArgumentBuffer(InArgumentBuffer)
		, ArgumentOffset(InArgumentOffset)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandDrawIndexedIndirect : public FRHICommand<FRHICommandDrawIndexedIndirect>
{
	FIndexBufferRHIParamRef IndexBufferRHI;
	uint32 PrimitiveType;
	FStructuredBufferRHIParamRef ArgumentsBufferRHI;
	uint32 DrawArgumentsIndex;
	uint32 NumInstances;

	FORCEINLINE_DEBUGGABLE FRHICommandDrawIndexedIndirect(FIndexBufferRHIParamRef InIndexBufferRHI, uint32 InPrimitiveType, FStructuredBufferRHIParamRef InArgumentsBufferRHI, uint32 InDrawArgumentsIndex, uint32 InNumInstances)
		: IndexBufferRHI(InIndexBufferRHI)
		, PrimitiveType(InPrimitiveType)
		, ArgumentsBufferRHI(InArgumentsBufferRHI)
		, DrawArgumentsIndex(InDrawArgumentsIndex)
		, NumInstances(InNumInstances)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandDrawIndexedPrimitiveIndirect : public FRHICommand<FRHICommandDrawIndexedPrimitiveIndirect>
{
	uint32 PrimitiveType;
	FIndexBufferRHIParamRef IndexBuffer;
	FVertexBufferRHIParamRef ArgumentsBuffer;
	uint32 ArgumentOffset;

	FORCEINLINE_DEBUGGABLE FRHICommandDrawIndexedPrimitiveIndirect(uint32 InPrimitiveType, FIndexBufferRHIParamRef InIndexBuffer, FVertexBufferRHIParamRef InArgumentsBuffer, uint32 InArgumentOffset)
		: PrimitiveType(InPrimitiveType)
		, IndexBuffer(InIndexBuffer)
		, ArgumentsBuffer(InArgumentsBuffer)
		, ArgumentOffset(InArgumentOffset)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandEnableDepthBoundsTest : public FRHICommand<FRHICommandEnableDepthBoundsTest>
{
	bool bEnable;
	float MinDepth;
	float MaxDepth;

	FORCEINLINE_DEBUGGABLE FRHICommandEnableDepthBoundsTest(bool InbEnable, float InMinDepth, float InMaxDepth)
		: bEnable(InbEnable)
		, MinDepth(InMinDepth)
		, MaxDepth(InMaxDepth)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandClearUAV : public FRHICommand<FRHICommandClearUAV>
{
	FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI;
	uint32 Values[4];

	FORCEINLINE_DEBUGGABLE FRHICommandClearUAV(FUnorderedAccessViewRHIParamRef InUnorderedAccessViewRHI, const uint32* InValues)
		: UnorderedAccessViewRHI(InUnorderedAccessViewRHI)
	{
		Values[0] = InValues[0];
		Values[1] = InValues[1];
		Values[2] = InValues[2];
		Values[3] = InValues[3];
	}
	RHI_API void Execute();
};

struct FRHICommandCopyToResolveTarget : public FRHICommand<FRHICommandCopyToResolveTarget>
{
	FTextureRHIParamRef SourceTexture;
	FTextureRHIParamRef DestTexture;
	bool bKeepOriginalSurface;
	FResolveParams ResolveParams;

	FORCEINLINE_DEBUGGABLE FRHICommandCopyToResolveTarget(FTextureRHIParamRef InSourceTexture, FTextureRHIParamRef InDestTexture, bool InbKeepOriginalSurface, const FResolveParams& InResolveParams)
		: SourceTexture(InSourceTexture)
		, DestTexture(InDestTexture)
		, bKeepOriginalSurface(InbKeepOriginalSurface)
		, ResolveParams(InResolveParams)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandClear : public FRHICommand<FRHICommandClear>
{
	FLinearColor Color;
	FIntRect ExcludeRect;
	float Depth;
	uint32 Stencil;
	bool bClearColor;
	bool bClearDepth;
	bool bClearStencil;

	FORCEINLINE_DEBUGGABLE FRHICommandClear(
		bool InbClearColor,
		const FLinearColor& InColor,
		bool InbClearDepth,
		float InDepth,
		bool InbClearStencil,
		uint32 InStencil,
		FIntRect InExcludeRect
		)
		: Color(InColor)
		, ExcludeRect(InExcludeRect)
		, Depth(InDepth)
		, Stencil(InStencil)
		, bClearColor(InbClearColor)
		, bClearDepth(InbClearDepth)
		, bClearStencil(InbClearStencil)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandClearMRT : public FRHICommand<FRHICommandClearMRT>
{
	FLinearColor ColorArray[MaxSimultaneousRenderTargets];
	FIntRect ExcludeRect;
	float Depth;
	uint32 Stencil;
	int32 NumClearColors;
	bool bClearColor;
	bool bClearDepth;
	bool bClearStencil;

	FORCEINLINE_DEBUGGABLE FRHICommandClearMRT(
		bool InbClearColor,
		int32 InNumClearColors,
		const FLinearColor* InColorArray,
		bool InbClearDepth,
		float InDepth,
		bool InbClearStencil,
		uint32 InStencil,
		FIntRect InExcludeRect
		)
		: ExcludeRect(InExcludeRect)
		, Depth(InDepth)
		, Stencil(InStencil)
		, NumClearColors(InNumClearColors)
		, bClearColor(InbClearColor)
		, bClearDepth(InbClearDepth)
		, bClearStencil(InbClearStencil)
	{
		check(InNumClearColors < MaxSimultaneousRenderTargets);
		for (int32 Index = 0; Index < InNumClearColors; Index++)
		{
			ColorArray[Index] = InColorArray[Index];
		}
	}
	RHI_API void Execute();
};


struct FBoundShaderStateInput
{
	FVertexDeclarationRHIParamRef VertexDeclarationRHI;
	FVertexShaderRHIParamRef VertexShaderRHI;
	FHullShaderRHIParamRef HullShaderRHI;
	FDomainShaderRHIParamRef DomainShaderRHI;
	FPixelShaderRHIParamRef PixelShaderRHI;
	FGeometryShaderRHIParamRef GeometryShaderRHI;

	FORCEINLINE FBoundShaderStateInput()
	{
	}

	FORCEINLINE FBoundShaderStateInput(FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
		FVertexShaderRHIParamRef InVertexShaderRHI,
		FHullShaderRHIParamRef InHullShaderRHI,
		FDomainShaderRHIParamRef InDomainShaderRHI,
		FPixelShaderRHIParamRef InPixelShaderRHI,
		FGeometryShaderRHIParamRef InGeometryShaderRHI
		)
		: VertexDeclarationRHI(InVertexDeclarationRHI)
		, VertexShaderRHI(InVertexShaderRHI)
		, HullShaderRHI(InHullShaderRHI)
		, DomainShaderRHI(InDomainShaderRHI)
		, PixelShaderRHI(InPixelShaderRHI)
		, GeometryShaderRHI(InGeometryShaderRHI)
	{
	}
};

struct FComputedBSS
{
	FBoundShaderStateRHIRef BSS;
	int32 UseCount;
	FComputedBSS()
		: UseCount(0)
	{
	}
};

struct FLocalBoundShaderStateWorkArea
{
	FBoundShaderStateInput Args;
	FComputedBSS* ComputedBSS;
#if DO_CHECK // the below variables are used in check(), which can be enabled in Shipping builds (see Build.h)
	FRHICommandListBase* CheckCmdList;
	int32 UID;
#endif

	FORCEINLINE_DEBUGGABLE FLocalBoundShaderStateWorkArea(
		FRHICommandListBase* InCheckCmdList,
		FVertexDeclarationRHIParamRef VertexDeclarationRHI,
		FVertexShaderRHIParamRef VertexShaderRHI,
		FHullShaderRHIParamRef HullShaderRHI,
		FDomainShaderRHIParamRef DomainShaderRHI,
		FPixelShaderRHIParamRef PixelShaderRHI,
		FGeometryShaderRHIParamRef GeometryShaderRHI
		)
		: Args(VertexDeclarationRHI, VertexShaderRHI, HullShaderRHI, DomainShaderRHI, PixelShaderRHI, GeometryShaderRHI)
#if DO_CHECK
		, CheckCmdList(InCheckCmdList)
		, UID(InCheckCmdList->GetUID())
#endif
	{
		ComputedBSS = new (InCheckCmdList->Alloc<FComputedBSS>()) FComputedBSS;
	}
};

struct FLocalBoundShaderState
{
	FLocalBoundShaderStateWorkArea* WorkArea;
	FBoundShaderStateRHIRef BypassBSS; // this is only used in the case of Bypass, should eventually be deleted
	FLocalBoundShaderState()
		: WorkArea(nullptr)
	{
	}
	FLocalBoundShaderState(const FLocalBoundShaderState& Other)
		: WorkArea(Other.WorkArea)
		, BypassBSS(Other.BypassBSS)
	{
	}
};

struct FRHICommandBuildLocalBoundShaderState : public FRHICommand<FRHICommandBuildLocalBoundShaderState>
{
	FLocalBoundShaderStateWorkArea WorkArea;

	FORCEINLINE_DEBUGGABLE FRHICommandBuildLocalBoundShaderState(
		FRHICommandListBase* CheckCmdList,
		FVertexDeclarationRHIParamRef VertexDeclarationRHI,
		FVertexShaderRHIParamRef VertexShaderRHI,
		FHullShaderRHIParamRef HullShaderRHI,
		FDomainShaderRHIParamRef DomainShaderRHI,
		FPixelShaderRHIParamRef PixelShaderRHI,
		FGeometryShaderRHIParamRef GeometryShaderRHI
		)
		: WorkArea(CheckCmdList, VertexDeclarationRHI, VertexShaderRHI, HullShaderRHI, DomainShaderRHI, PixelShaderRHI, GeometryShaderRHI)

	{
	}
	RHI_API void Execute();
};

struct FRHICommandSetLocalBoundShaderState : public FRHICommand<FRHICommandSetLocalBoundShaderState>
{
	FLocalBoundShaderState LocalBoundShaderState;
	FORCEINLINE_DEBUGGABLE FRHICommandSetLocalBoundShaderState(FRHICommandListBase* CheckCmdList, FLocalBoundShaderState& InLocalBoundShaderState)
		: LocalBoundShaderState(InLocalBoundShaderState)
	{
		check(CheckCmdList == LocalBoundShaderState.WorkArea->CheckCmdList && CheckCmdList->GetUID() == LocalBoundShaderState.WorkArea->UID); // this BSS was not built for this particular commandlist
		LocalBoundShaderState.WorkArea->ComputedBSS->UseCount++;
	}
	RHI_API void Execute();
};


struct FComputedUniformBuffer
{
	FUniformBufferRHIRef UniformBuffer;
	mutable int32 UseCount;
	FComputedUniformBuffer()
		: UseCount(0)
	{
	}
};


struct FLocalUniformBufferWorkArea
{
	void* Contents;
	const FRHIUniformBufferLayout* Layout;
	FComputedUniformBuffer* ComputedUniformBuffer;
#if DO_CHECK // the below variables are used in check(), which can be enabled in Shipping builds (see Build.h)
	FRHICommandListBase* CheckCmdList;
	int32 UID;
#endif

	FLocalUniformBufferWorkArea(FRHICommandListBase* InCheckCmdList, const void* InContents, uint32 ContentsSize, const FRHIUniformBufferLayout* InLayout)
		: Layout(InLayout)
#if DO_CHECK
		, CheckCmdList(InCheckCmdList)
		, UID(InCheckCmdList->GetUID())
#endif
	{
		check(ContentsSize);
		Contents = InCheckCmdList->Alloc(ContentsSize, UNIFORM_BUFFER_STRUCT_ALIGNMENT);
		FMemory::Memcpy(Contents, InContents, ContentsSize);
		ComputedUniformBuffer = new (InCheckCmdList->Alloc<FComputedUniformBuffer>()) FComputedUniformBuffer;
	}
};

struct FLocalUniformBuffer
{
	FLocalUniformBufferWorkArea* WorkArea;
	FUniformBufferRHIRef BypassUniform; // this is only used in the case of Bypass, should eventually be deleted
	FLocalUniformBuffer()
		: WorkArea(nullptr)
	{
	}
	FLocalUniformBuffer(const FLocalUniformBuffer& Other)
		: WorkArea(Other.WorkArea)
		, BypassUniform(Other.BypassUniform)
	{
	}
	FORCEINLINE_DEBUGGABLE bool IsValid() const
	{
		return WorkArea || IsValidRef(BypassUniform);
	}
};

struct FRHICommandBuildLocalUniformBuffer : public FRHICommand<FRHICommandBuildLocalUniformBuffer>
{
	FLocalUniformBufferWorkArea WorkArea;
	FORCEINLINE_DEBUGGABLE FRHICommandBuildLocalUniformBuffer(
		FRHICommandListBase* CheckCmdList,
		const void* Contents,
		uint32 ContentsSize,
		const FRHIUniformBufferLayout& Layout
		)
		: WorkArea(CheckCmdList, Contents, ContentsSize, &Layout)

	{
	}
	RHI_API void Execute();
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetLocalUniformBuffer : public FRHICommand<FRHICommandSetLocalUniformBuffer<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	uint32 BaseIndex;
	FLocalUniformBuffer LocalUniformBuffer;
	FORCEINLINE_DEBUGGABLE FRHICommandSetLocalUniformBuffer(FRHICommandListBase* CheckCmdList, TShaderRHIParamRef InShader, uint32 InBaseIndex, const FLocalUniformBuffer& InLocalUniformBuffer)
		: Shader(InShader)
		, BaseIndex(InBaseIndex)
		, LocalUniformBuffer(InLocalUniformBuffer)

	{
		check(CheckCmdList == LocalUniformBuffer.WorkArea->CheckCmdList && CheckCmdList->GetUID() == LocalUniformBuffer.WorkArea->UID); // this uniform buffer was not built for this particular commandlist
		LocalUniformBuffer.WorkArea->ComputedUniformBuffer->UseCount++;
	}
	RHI_API void Execute();
};

struct FRHICommandBeginRenderQuery : public FRHICommand<FRHICommandBeginRenderQuery>
{
	FRenderQueryRHIParamRef RenderQuery;

	FORCEINLINE_DEBUGGABLE FRHICommandBeginRenderQuery(FRenderQueryRHIParamRef InRenderQuery)
		: RenderQuery(InRenderQuery)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandEndRenderQuery : public FRHICommand<FRHICommandEndRenderQuery>
{
	FRenderQueryRHIParamRef RenderQuery;

	FORCEINLINE_DEBUGGABLE FRHICommandEndRenderQuery(FRenderQueryRHIParamRef InRenderQuery)
		: RenderQuery(InRenderQuery)
	{
	}
	RHI_API void Execute();
};

#if !PLATFORM_SUPPORTS_RHI_THREAD
struct FRHICommandResetRenderQuery : public FRHICommand<FRHICommandResetRenderQuery>
{
	FRenderQueryRHIParamRef RenderQuery;

	FORCEINLINE_DEBUGGABLE FRHICommandResetRenderQuery(FRenderQueryRHIParamRef InRenderQuery)
		: RenderQuery(InRenderQuery)
	{
	}
	RHI_API void Execute();
};
#endif

struct FRHICommandBeginScene : public FRHICommand<FRHICommandBeginScene>
{
	FORCEINLINE_DEBUGGABLE FRHICommandBeginScene()
	{
	}
	RHI_API void Execute();
};

struct FRHICommandEndScene : public FRHICommand<FRHICommandEndScene>
{
	FORCEINLINE_DEBUGGABLE FRHICommandEndScene()
	{
	}
	RHI_API void Execute();
};

struct FRHICommandUpdateVertexBuffer : public FRHICommand<FRHICommandUpdateVertexBuffer>
{
	FVertexBufferRHIParamRef VertexBuffer;
	void const* Buffer;
	int32 BufferSize;

	FORCEINLINE_DEBUGGABLE FRHICommandUpdateVertexBuffer(FVertexBufferRHIParamRef InVertexBuffer, void const* InBuffer, int32 InBufferSize)
		: VertexBuffer(InVertexBuffer)
		, Buffer(InBuffer)
		, BufferSize(InBufferSize)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandBeginFrame : public FRHICommand<FRHICommandBeginFrame>
{
	FORCEINLINE_DEBUGGABLE FRHICommandBeginFrame()
	{
	}
	RHI_API void Execute();
};

struct FRHICommandEndFrame : public FRHICommand<FRHICommandEndFrame>
{
	FORCEINLINE_DEBUGGABLE FRHICommandEndFrame()
	{
	}
	RHI_API void Execute();
};

#if PLATFORM_SUPPORTS_RHI_THREAD
struct FRHICommandBeginDrawingViewport : public FRHICommand<FRHICommandBeginDrawingViewport>
{
	FViewportRHIParamRef Viewport;
	FTextureRHIParamRef RenderTargetRHI;

	FORCEINLINE_DEBUGGABLE FRHICommandBeginDrawingViewport(FViewportRHIParamRef InViewport, FTextureRHIParamRef InRenderTargetRHI)
		: Viewport(InViewport)
		, RenderTargetRHI(InRenderTargetRHI)
	{
	}
	RHI_API void Execute();
};

struct FRHICommandEndDrawingViewport : public FRHICommand<FRHICommandEndDrawingViewport>
{
	FViewportRHIParamRef Viewport;
	bool bPresent;
	bool bLockToVsync;

	FORCEINLINE_DEBUGGABLE FRHICommandEndDrawingViewport(FViewportRHIParamRef InViewport, bool InbPresent, bool InbLockToVsync)
		: Viewport(InViewport)
		, bPresent(InbPresent)
		, bLockToVsync(InbLockToVsync)
	{
	}
	RHI_API void Execute();
};
#endif


class RHI_API FRHICommandList : public FRHICommandListBase
{
public:

	/** Custom new/delete with recycling */
	void* operator new(size_t Size);
	void operator delete(void *RawMemory);

#if USE_RHICOMMAND_STATE_REDUCTION
	FORCEINLINE_DEBUGGABLE bool AllocStateCache()
	{
		if (!StateCache)
		{
			StateCache = new (Alloc<FRHICommandListStateCache>()) FRHICommandListStateCache();
			return false;
		}
		return true;
	}
#endif
	FORCEINLINE_DEBUGGABLE FLocalBoundShaderState BuildLocalBoundShaderState(const FBoundShaderStateInput& BoundShaderStateInput)
	{
		return BuildLocalBoundShaderState(
			BoundShaderStateInput.VertexDeclarationRHI,
			BoundShaderStateInput.VertexShaderRHI,
			BoundShaderStateInput.HullShaderRHI,
			BoundShaderStateInput.DomainShaderRHI,
			BoundShaderStateInput.PixelShaderRHI,
			BoundShaderStateInput.GeometryShaderRHI
			);
	}

	FORCEINLINE_DEBUGGABLE void BuildAndSetLocalBoundShaderState(const FBoundShaderStateInput& BoundShaderStateInput)
	{
		SetLocalBoundShaderState(BuildLocalBoundShaderState(
			BoundShaderStateInput.VertexDeclarationRHI,
			BoundShaderStateInput.VertexShaderRHI,
			BoundShaderStateInput.HullShaderRHI,
			BoundShaderStateInput.DomainShaderRHI,
			BoundShaderStateInput.PixelShaderRHI,
			BoundShaderStateInput.GeometryShaderRHI
			));
	}

	FORCEINLINE_DEBUGGABLE FLocalBoundShaderState BuildLocalBoundShaderState(
		FVertexDeclarationRHIParamRef VertexDeclarationRHI,
		FVertexShaderRHIParamRef VertexShaderRHI,
		FHullShaderRHIParamRef HullShaderRHI,
		FDomainShaderRHIParamRef DomainShaderRHI,
		FPixelShaderRHIParamRef PixelShaderRHI,
		FGeometryShaderRHIParamRef GeometryShaderRHI
		)
	{
		FLocalBoundShaderState Result;
		if (Bypass())
		{
			Result.BypassBSS = CreateBoundShaderState_Internal(VertexDeclarationRHI, VertexShaderRHI, HullShaderRHI, DomainShaderRHI, PixelShaderRHI, GeometryShaderRHI);
		}
		else
		{
			auto* Cmd = new (AllocCommand<FRHICommandBuildLocalBoundShaderState>()) FRHICommandBuildLocalBoundShaderState(this, VertexDeclarationRHI, VertexShaderRHI, HullShaderRHI, DomainShaderRHI, PixelShaderRHI, GeometryShaderRHI);
			Result.WorkArea = &Cmd->WorkArea;
		}
		return Result;
	}

	FORCEINLINE_DEBUGGABLE void SetLocalBoundShaderState(FLocalBoundShaderState LocalBoundShaderState)
	{
		if (Bypass())
		{
			SetBoundShaderState_Internal(LocalBoundShaderState.BypassBSS);
			return;
		}
#if USE_RHICOMMAND_STATE_REDUCTION
		if (StateCache)
		{
			StateCache->FlushShaderState();
		}
#endif
		new (AllocCommand<FRHICommandSetLocalBoundShaderState>()) FRHICommandSetLocalBoundShaderState(this, LocalBoundShaderState);
	}

	FORCEINLINE_DEBUGGABLE FLocalUniformBuffer BuildLocalUniformBuffer(const void* Contents, uint32 ContentsSize, const FRHIUniformBufferLayout& Layout)
	{
		FLocalUniformBuffer Result;
		if (Bypass())
		{
#if PLATFORM_SUPPORTS_RHI_THREAD
			Result.BypassUniform = RHICreateUniformBuffer(Contents, Layout, UniformBuffer_SingleFrame);
#else
			Result.BypassUniform = CreateUniformBuffer_Internal(Contents, Layout, UniformBuffer_SingleFrame);
#endif
		}
		else
		{
			auto* Cmd = new (AllocCommand<FRHICommandBuildLocalUniformBuffer>()) FRHICommandBuildLocalUniformBuffer(this, Contents, ContentsSize, Layout);
			Result.WorkArea = &Cmd->WorkArea;
		}
		return Result;
	}

	template <typename TShaderRHIParamRef>
	FORCEINLINE_DEBUGGABLE void SetLocalShaderUniformBuffer(TShaderRHIParamRef Shader, uint32 BaseIndex, const FLocalUniformBuffer& UniformBuffer)
	{
		if (Bypass())
		{
			SetShaderUniformBuffer_Internal(Shader, BaseIndex, UniformBuffer.BypassUniform);
			return;
		}
#if USE_RHICOMMAND_STATE_REDUCTION
		if (StateCache)
		{
			//local uniform buffers are rare, so we will just flush
			StateCache->BoundUniformBuffers[TRHIShaderToEnum<TShaderRHIParamRef>::ShaderFrequency][BaseIndex] = nullptr;
		}
#endif
		new (AllocCommand<FRHICommandSetLocalUniformBuffer<TShaderRHIParamRef> >()) FRHICommandSetLocalUniformBuffer<TShaderRHIParamRef>(this, Shader, BaseIndex, UniformBuffer);
	}

	template <typename TShaderRHI>
	FORCEINLINE_DEBUGGABLE void SetShaderUniformBuffer(TShaderRHI* Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
	{
		if (Bypass())
		{
			SetShaderUniformBuffer_Internal(Shader, BaseIndex, UniformBuffer);
			return;
		}
#if USE_RHICOMMAND_STATE_REDUCTION
		if (BaseIndex < FRHICommandListStateCache::MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE)
		{
			if (AllocStateCache() && StateCache->BoundUniformBuffers[TRHIShaderToEnum<TShaderRHI*>::ShaderFrequency][BaseIndex] == UniformBuffer)
			{
				return;
			}
			StateCache->BoundUniformBuffers[TRHIShaderToEnum<TShaderRHI*>::ShaderFrequency][BaseIndex] = UniformBuffer;
		}
#endif
		new (AllocCommand<FRHICommandSetShaderUniformBuffer<TShaderRHI*> >()) FRHICommandSetShaderUniformBuffer<TShaderRHI*>(Shader, BaseIndex, UniformBuffer);
	}
	template <typename TShaderRHI>
	FORCEINLINE void SetShaderUniformBuffer(TRefCountPtr<TShaderRHI>& Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
	{
		SetShaderUniformBuffer(Shader.GetReference(), BaseIndex, UniformBuffer);
	}

	template <typename TShaderRHI>
	FORCEINLINE_DEBUGGABLE void SetShaderParameter(TShaderRHI* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
	{
		if (Bypass())
		{
			SetShaderParameter_Internal(Shader, BufferIndex, BaseIndex, NumBytes, NewValue);
			return;
		}
		void* UseValue = Alloc(NumBytes, 16);
		FMemory::Memcpy(UseValue, NewValue, NumBytes);
		new (AllocCommand<FRHICommandSetShaderParameter<TShaderRHI*> >()) FRHICommandSetShaderParameter<TShaderRHI*>(Shader, BufferIndex, BaseIndex, NumBytes, UseValue);
	}
	template <typename TShaderRHI>
	FORCEINLINE void SetShaderParameter(TRefCountPtr<TShaderRHI>& Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
	{
		SetShaderParameter(Shader.GetReference(), BufferIndex, BaseIndex, NumBytes, NewValue);
	}

	template <typename TShaderRHIParamRef>
	FORCEINLINE_DEBUGGABLE void SetShaderTexture(TShaderRHIParamRef Shader, uint32 TextureIndex, FTextureRHIParamRef Texture)
	{
		if (Bypass())
		{
			SetShaderTexture_Internal(Shader, TextureIndex, Texture);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderTexture<TShaderRHIParamRef> >()) FRHICommandSetShaderTexture<TShaderRHIParamRef>(Shader, TextureIndex, Texture);
	}

	template <typename TShaderRHIParamRef>
	FORCEINLINE_DEBUGGABLE void SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV)
	{
		if (Bypass())
		{
			SetShaderResourceViewParameter_Internal(Shader, SamplerIndex, SRV);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderResourceViewParameter<TShaderRHIParamRef> >()) FRHICommandSetShaderResourceViewParameter<TShaderRHIParamRef>(Shader, SamplerIndex, SRV);
	}

	template <typename TShaderRHIParamRef>
	FORCEINLINE_DEBUGGABLE void SetShaderSampler(TShaderRHIParamRef Shader, uint32 SamplerIndex, FSamplerStateRHIParamRef State)
	{
		if (Bypass())
		{
			SetShaderSampler_Internal(Shader, SamplerIndex, State);
			return;
		}
#if USE_RHICOMMAND_STATE_REDUCTION
		if (SamplerIndex < FRHICommandListStateCache::MAX_SAMPLERS_PER_SHADER_STAGE)
		{
			if (AllocStateCache() && StateCache->Samplers[TRHIShaderToEnum<TShaderRHIParamRef>::ShaderFrequency][SamplerIndex] == State)
			{
				return;
			}
			StateCache->Samplers[TRHIShaderToEnum<TShaderRHIParamRef>::ShaderFrequency][SamplerIndex] = State;
		}
#endif
		new (AllocCommand<FRHICommandSetShaderSampler<TShaderRHIParamRef> >()) FRHICommandSetShaderSampler<TShaderRHIParamRef>(Shader, SamplerIndex, State);
	}

	FORCEINLINE_DEBUGGABLE void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
	{
		if (Bypass())
		{
			SetUAVParameter_Internal(Shader, UAVIndex, UAV);
			return;
		}
		new (AllocCommand<FRHICommandSetUAVParameter<FComputeShaderRHIParamRef> >()) FRHICommandSetUAVParameter<FComputeShaderRHIParamRef>(Shader, UAVIndex, UAV);
	}

	FORCEINLINE_DEBUGGABLE void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount)
	{
		if (Bypass())
		{
			SetUAVParameter_Internal(Shader, UAVIndex, UAV, InitialCount);
			return;
		}
		new (AllocCommand<FRHICommandSetUAVParameter_IntialCount<FComputeShaderRHIParamRef> >()) FRHICommandSetUAVParameter_IntialCount<FComputeShaderRHIParamRef>(Shader, UAVIndex, UAV, InitialCount);
	}

	FORCEINLINE_DEBUGGABLE void SetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState)
	{
		if (Bypass())
		{
			SetBoundShaderState_Internal(BoundShaderState);
			return;
		}
#if USE_RHICOMMAND_STATE_REDUCTION
		if (StateCache)
		{
			StateCache->FlushShaderState();
		}
#endif
		new (AllocCommand<FRHICommandSetBoundShaderState>()) FRHICommandSetBoundShaderState(BoundShaderState);
	}

	FORCEINLINE_DEBUGGABLE void SetRasterizerState(FRasterizerStateRHIParamRef State)
	{
		if (Bypass())
		{
			SetRasterizerState_Internal(State);
			return;
		}
		new (AllocCommand<FRHICommandSetRasterizerState>()) FRHICommandSetRasterizerState(State);
	}

	FORCEINLINE_DEBUGGABLE void SetBlendState(FBlendStateRHIParamRef State, const FLinearColor& BlendFactor = FLinearColor::White)
	{
		if (Bypass())
		{
			SetBlendState_Internal(State, BlendFactor);
			return;
		}
		new (AllocCommand<FRHICommandSetBlendState>()) FRHICommandSetBlendState(State, BlendFactor);
	}

	FORCEINLINE_DEBUGGABLE void DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		if (Bypass())
		{
			DrawPrimitive_Internal(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
			return;
		}
		new (AllocCommand<FRHICommandDrawPrimitive>()) FRHICommandDrawPrimitive(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
	}

	FORCEINLINE_DEBUGGABLE void DrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 MinIndex, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		if (Bypass())
		{
			DrawIndexedPrimitive_Internal(IndexBuffer, PrimitiveType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, NumPrimitives, NumInstances);
			return;
		}
		new (AllocCommand<FRHICommandDrawIndexedPrimitive>()) FRHICommandDrawIndexedPrimitive(IndexBuffer, PrimitiveType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, NumPrimitives, NumInstances);
	}

	FORCEINLINE_DEBUGGABLE void SetStreamSource(uint32 StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint32 Offset)
	{
		if (Bypass())
		{
			SetStreamSource_Internal(StreamIndex, VertexBuffer, Stride, Offset);
			return;
		}
		new (AllocCommand<FRHICommandSetStreamSource>()) FRHICommandSetStreamSource(StreamIndex, VertexBuffer, Stride, Offset);
	}

	FORCEINLINE_DEBUGGABLE void SetDepthStencilState(FDepthStencilStateRHIParamRef NewStateRHI, uint32 StencilRef = 0)
	{
		if (Bypass())
		{
			SetDepthStencilState_Internal(NewStateRHI, StencilRef);
			return;
		}
		new (AllocCommand<FRHICommandSetDepthStencilState>()) FRHICommandSetDepthStencilState(NewStateRHI, StencilRef);
	}

	FORCEINLINE_DEBUGGABLE void SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ)
	{
		if (Bypass())
		{
			SetViewport_Internal(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
			return;
		}
		new (AllocCommand<FRHICommandSetViewport>()) FRHICommandSetViewport(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
	}

	FORCEINLINE_DEBUGGABLE void SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
	{
		if (Bypass())
		{
			SetScissorRect_Internal(bEnable, MinX, MinY, MaxX, MaxY);
			return;
		}
		new (AllocCommand<FRHICommandSetScissorRect>()) FRHICommandSetScissorRect(bEnable, MinX, MinY, MaxX, MaxY);
	}

	FORCEINLINE_DEBUGGABLE void SetRenderTargets(
		uint32 NewNumSimultaneousRenderTargets,
		const FRHIRenderTargetView* NewRenderTargetsRHI,
		FTextureRHIParamRef NewDepthStencilTargetRHI,
		uint32 NewNumUAVs,
		const FUnorderedAccessViewRHIParamRef* UAVs
		)
	{
		if (Bypass())
		{
			SetRenderTargets_Internal(
				NewNumSimultaneousRenderTargets,
				NewRenderTargetsRHI,
				NewDepthStencilTargetRHI,
				NewNumUAVs,
				UAVs);
			return;
		}
		new (AllocCommand<FRHICommandSetRenderTargets>()) FRHICommandSetRenderTargets(
			NewNumSimultaneousRenderTargets,
			NewRenderTargetsRHI,
			NewDepthStencilTargetRHI,
			NewNumUAVs,
			UAVs);
	}

	FORCEINLINE_DEBUGGABLE void BeginDrawPrimitiveUP(uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData)
	{
		if (Bypass())
		{
			BeginDrawPrimitiveUP_Internal(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, OutVertexData);
			return;
		}
		check(!DrawUPData.OutVertexData && NumVertices * VertexDataStride > 0);

		OutVertexData = Alloc(NumVertices * VertexDataStride, 16);

		DrawUPData.PrimitiveType = PrimitiveType;
		DrawUPData.NumPrimitives = NumPrimitives;
		DrawUPData.NumVertices = NumVertices;
		DrawUPData.VertexDataStride = VertexDataStride;
		DrawUPData.OutVertexData = OutVertexData;
	}

	FORCEINLINE_DEBUGGABLE void EndDrawPrimitiveUP()
	{
		if (Bypass())
		{
			EndDrawPrimitiveUP_Internal();
			return;
		}
		check(DrawUPData.OutVertexData && DrawUPData.NumVertices);
		new (AllocCommand<FRHICommandEndDrawPrimitiveUP>()) FRHICommandEndDrawPrimitiveUP(DrawUPData.PrimitiveType, DrawUPData.NumPrimitives, DrawUPData.NumVertices, DrawUPData.VertexDataStride, DrawUPData.OutVertexData);

		DrawUPData.OutVertexData = nullptr;
		DrawUPData.NumVertices = 0;
	}

	FORCEINLINE_DEBUGGABLE void BeginDrawIndexedPrimitiveUP(uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData, uint32 MinVertexIndex, uint32 NumIndices, uint32 IndexDataStride, void*& OutIndexData)
	{
		if (Bypass())
		{
			BeginDrawIndexedPrimitiveUP_Internal(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, OutVertexData, MinVertexIndex, NumIndices, IndexDataStride, OutIndexData);
			return;
		}
		check(!DrawUPData.OutVertexData && !DrawUPData.OutIndexData && NumVertices * VertexDataStride > 0 && NumIndices * IndexDataStride > 0);

		OutVertexData = Alloc(NumVertices * VertexDataStride, 16);
		OutIndexData = Alloc(NumIndices * IndexDataStride, 16);

		DrawUPData.PrimitiveType = PrimitiveType;
		DrawUPData.NumPrimitives = NumPrimitives;
		DrawUPData.NumVertices = NumVertices;
		DrawUPData.VertexDataStride = VertexDataStride;
		DrawUPData.OutVertexData = OutVertexData;
		DrawUPData.MinVertexIndex = MinVertexIndex;
		DrawUPData.NumIndices = NumIndices;
		DrawUPData.IndexDataStride = IndexDataStride;
		DrawUPData.OutIndexData = OutIndexData;

	}

	FORCEINLINE_DEBUGGABLE void EndDrawIndexedPrimitiveUP()
	{
		if (Bypass())
		{
			EndDrawIndexedPrimitiveUP_Internal();
			return;
		}
		check(DrawUPData.OutVertexData && DrawUPData.OutIndexData && DrawUPData.NumIndices && DrawUPData.NumVertices);

		new (AllocCommand<FRHICommandEndDrawIndexedPrimitiveUP>()) FRHICommandEndDrawIndexedPrimitiveUP(DrawUPData.PrimitiveType, DrawUPData.NumPrimitives, DrawUPData.NumVertices, DrawUPData.VertexDataStride, DrawUPData.OutVertexData, DrawUPData.MinVertexIndex, DrawUPData.NumIndices, DrawUPData.IndexDataStride, DrawUPData.OutIndexData);

		DrawUPData.OutVertexData = nullptr;
		DrawUPData.OutIndexData = nullptr;
		DrawUPData.NumIndices = 0;
		DrawUPData.NumVertices = 0;
	}

	FORCEINLINE_DEBUGGABLE void SetComputeShader(FComputeShaderRHIParamRef ComputeShader)
	{
		if (Bypass())
		{
			SetComputeShader_Internal(ComputeShader);
			return;
		}
#if USE_RHICOMMAND_STATE_REDUCTION
		if (StateCache)
		{
			StateCache->FlushShaderState();
		}
#endif
		new (AllocCommand<FRHICommandSetComputeShader>()) FRHICommandSetComputeShader(ComputeShader);
	}

	FORCEINLINE_DEBUGGABLE void DispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
		if (Bypass())
		{
			DispatchComputeShader_Internal(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
			return;
		}
		new (AllocCommand<FRHICommandDispatchComputeShader>()) FRHICommandDispatchComputeShader(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	FORCEINLINE_DEBUGGABLE void DispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer, uint32 ArgumentOffset)
	{
		if (Bypass())
		{
			DispatchIndirectComputeShader_Internal(ArgumentBuffer, ArgumentOffset);
			return;
		}
		new (AllocCommand<FRHICommandDispatchIndirectComputeShader>()) FRHICommandDispatchIndirectComputeShader(ArgumentBuffer, ArgumentOffset);
	}

	FORCEINLINE_DEBUGGABLE void AutomaticCacheFlushAfterComputeShader(bool bEnable)
	{
		if (Bypass())
		{
			AutomaticCacheFlushAfterComputeShader_Internal(bEnable);
			return;
		}
		new (AllocCommand<FRHICommandAutomaticCacheFlushAfterComputeShader>()) FRHICommandAutomaticCacheFlushAfterComputeShader(bEnable);
	}

	FORCEINLINE_DEBUGGABLE void FlushComputeShaderCache()
	{
		if (Bypass())
		{
			FlushComputeShaderCache_Internal();
			return;
		}
		new (AllocCommand<FRHICommandFlushComputeShaderCache>()) FRHICommandFlushComputeShaderCache();
	}

	FORCEINLINE_DEBUGGABLE void DrawPrimitiveIndirect(uint32 PrimitiveType, FVertexBufferRHIParamRef ArgumentBuffer, uint32 ArgumentOffset)
	{
		if (Bypass())
		{
			DrawPrimitiveIndirect_Internal(PrimitiveType, ArgumentBuffer, ArgumentOffset);
			return;
		}
		new (AllocCommand<FRHICommandDrawPrimitiveIndirect>()) FRHICommandDrawPrimitiveIndirect(PrimitiveType, ArgumentBuffer, ArgumentOffset);
	}

	FORCEINLINE_DEBUGGABLE void DrawIndexedIndirect(FIndexBufferRHIParamRef IndexBufferRHI, uint32 PrimitiveType, FStructuredBufferRHIParamRef ArgumentsBufferRHI, uint32 DrawArgumentsIndex, uint32 NumInstances)
	{
		if (Bypass())
		{
			DrawIndexedIndirect_Internal(IndexBufferRHI, PrimitiveType, ArgumentsBufferRHI, DrawArgumentsIndex, NumInstances);
			return;
		}
		new (AllocCommand<FRHICommandDrawIndexedIndirect>()) FRHICommandDrawIndexedIndirect(IndexBufferRHI, PrimitiveType, ArgumentsBufferRHI, DrawArgumentsIndex, NumInstances);
	}

	FORCEINLINE_DEBUGGABLE void DrawIndexedPrimitiveIndirect(uint32 PrimitiveType, FIndexBufferRHIParamRef IndexBuffer, FVertexBufferRHIParamRef ArgumentsBuffer, uint32 ArgumentOffset)
	{
		if (Bypass())
		{
			DrawIndexedPrimitiveIndirect_Internal(PrimitiveType, IndexBuffer, ArgumentsBuffer, ArgumentOffset);
			return;
		}
		new (AllocCommand<FRHICommandDrawIndexedPrimitiveIndirect>()) FRHICommandDrawIndexedPrimitiveIndirect(PrimitiveType, IndexBuffer, ArgumentsBuffer, ArgumentOffset);
	}

	FORCEINLINE_DEBUGGABLE void EnableDepthBoundsTest(bool bEnable, float MinDepth, float MaxDepth)
	{
		if (Bypass())
		{
			EnableDepthBoundsTest_Internal(bEnable, MinDepth, MaxDepth);
			return;
		}
		new (AllocCommand<FRHICommandEnableDepthBoundsTest>()) FRHICommandEnableDepthBoundsTest(bEnable, MinDepth, MaxDepth);
	}

	FORCEINLINE_DEBUGGABLE void ClearUAV(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const uint32(&Values)[4])
	{
		if (Bypass())
		{
			ClearUAV_Internal(UnorderedAccessViewRHI, Values);
			return;
		}
		new (AllocCommand<FRHICommandClearUAV>()) FRHICommandClearUAV(UnorderedAccessViewRHI, Values);
	}

	FORCEINLINE_DEBUGGABLE void CopyToResolveTarget(FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI, bool bKeepOriginalSurface, const FResolveParams& ResolveParams)
	{
		if (Bypass())
		{
			CopyToResolveTarget_Internal(SourceTextureRHI, DestTextureRHI, bKeepOriginalSurface, ResolveParams);
			return;
		}
		new (AllocCommand<FRHICommandCopyToResolveTarget>()) FRHICommandCopyToResolveTarget(SourceTextureRHI, DestTextureRHI, bKeepOriginalSurface, ResolveParams);
	}

	FORCEINLINE_DEBUGGABLE void Clear(bool bClearColor, const FLinearColor& Color, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntRect ExcludeRect)
	{
		if (Bypass())
		{
			Clear_Internal(bClearColor, Color, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
			return;
		}
		new (AllocCommand<FRHICommandClear>()) FRHICommandClear(bClearColor, Color, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
	}

	FORCEINLINE_DEBUGGABLE void ClearMRT(bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntRect ExcludeRect)
	{
		if (Bypass())
		{
			ClearMRT_Internal(bClearColor, NumClearColors, ClearColorArray, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
			return;
		}
		new (AllocCommand<FRHICommandClearMRT>()) FRHICommandClearMRT(bClearColor, NumClearColors, ClearColorArray, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
	}

	FORCEINLINE_DEBUGGABLE void UpdateVertexBuffer(FVertexBufferRHIParamRef VertexBuffer, void const* Buffer, int32 BufferSize)
	{
		if (Bypass())
		{
			void* Data = LockVertexBuffer_Internal(VertexBuffer, 0, BufferSize, RLM_WriteOnly);
			FMemory::Memcpy(Data, Buffer, BufferSize);
			UnlockVertexBuffer_Internal(VertexBuffer);
			FMemory::Free((void*)Buffer);
			return;
		}
		new (AllocCommand<FRHICommandUpdateVertexBuffer>()) FRHICommandUpdateVertexBuffer(VertexBuffer, Buffer, BufferSize);
	}
	FORCEINLINE_DEBUGGABLE void BeginRenderQuery(FRenderQueryRHIParamRef RenderQuery)
	{
		if (Bypass())
		{
			BeginRenderQuery_Internal(RenderQuery);
			return;
		}
		new (AllocCommand<FRHICommandBeginRenderQuery>()) FRHICommandBeginRenderQuery(RenderQuery);
	}
	FORCEINLINE_DEBUGGABLE void EndRenderQuery(FRenderQueryRHIParamRef RenderQuery)
	{
		if (Bypass())
		{
			EndRenderQuery_Internal(RenderQuery);
			return;
		}
		new (AllocCommand<FRHICommandEndRenderQuery>()) FRHICommandEndRenderQuery(RenderQuery);
	}
#if !PLATFORM_SUPPORTS_RHI_THREAD
	FORCEINLINE_DEBUGGABLE void ResetRenderQuery(FRenderQueryRHIParamRef RenderQuery)
	{
		if (Bypass())
		{
			ResetRenderQuery_Internal(RenderQuery);
			return;
		}
		new (AllocCommand<FRHICommandResetRenderQuery>()) FRHICommandResetRenderQuery(RenderQuery);
	}
#endif

#if PLATFORM_SUPPORTS_RHI_THREAD
	FORCEINLINE_DEBUGGABLE void BeginScene()
	{
		if (Bypass())
		{
			BeginScene_Internal();
			return;
		}
		new (AllocCommand<FRHICommandBeginScene>()) FRHICommandBeginScene();
	}
	FORCEINLINE_DEBUGGABLE void EndScene()
	{
		if (Bypass())
		{
			EndScene_Internal();
			return;
		}
		new (AllocCommand<FRHICommandEndScene>()) FRHICommandEndScene();
	}
	void BeginDrawingViewport(FViewportRHIParamRef Viewport, FTextureRHIParamRef RenderTargetRHI);
	void EndDrawingViewport(FViewportRHIParamRef Viewport, bool bPresent, bool bLockToVsync);
	void BeginFrame();
	void EndFrame();
#endif
};

namespace EImmediateFlushType
{
	enum Type
	{ 
		WaitForOutstandingTasksOnly = 0, 
		DispatchToRHIThread, 
		WaitForRHIThread, 
		FlushRHIThread,
		FlushRHIThreadFlushResources
	};
};

class RHI_API FRHICommandListImmediate : public FRHICommandList
{
	friend class FRHICommandListExecutor;
	FRHICommandListImmediate()
	{
	}
	~FRHICommandListImmediate()
	{
		check(!HasCommands());
	}
public:

	inline void ImmediateFlush(EImmediateFlushType::Type FlushType);

	#define DEFINE_RHIMETHOD_CMDLIST(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
		{ \
			QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_##Name##_Flush); \
			ImmediateFlush(EImmediateFlushType::FlushRHIThread); \
			ReturnStatement Name##_Internal ParameterNames; \
		}
	#define DEFINE_RHIMETHOD_GLOBAL(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
		{ \
			if (GRHIThread) \
			{ \
				QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_##Name##_WaitRHI); \
				ImmediateFlush(EImmediateFlushType::WaitForRHIThread); \
			} \
			ReturnStatement Name##_Internal ParameterNames; \
		}
	#define DEFINE_RHIMETHOD_GLOBALTHREADSAFE(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
		{ \
			ReturnStatement RHI##Name ParameterNames; \
		}
	#define DEFINE_RHIMETHOD_GLOBALFLUSH(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
		{ \
			QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_##Name##_Flush); \
			ImmediateFlush(EImmediateFlushType::FlushRHIThread); \
			ReturnStatement Name##_Internal ParameterNames; \
		}
	#include "RHIMethods.h"
	#undef DEFINE_RHIMETHOD
	#undef DEFINE_RHIMETHOD_CMDLIST
	#undef DEFINE_RHIMETHOD_GLOBAL
	#undef DEFINE_RHIMETHOD_GLOBALFLUSH
	#undef DEFINE_RHIMETHOD_GLOBALTHREADSAFE

};


// typedef to mark the recursive use of commandlists in the RHI implmentations
typedef FRHICommandList FRHICommandList_RecursiveHazardous;

class RHI_API FRHICommandListExecutor
{
public:
	enum
	{
		DefaultBypass = 1
	};
	FRHICommandListExecutor()
		: bLatchedBypass(!!DefaultBypass)
		, bLatchedUseParallelAlgorithms(false)
	{
	}
	static inline FRHICommandListImmediate& GetImmediateCommandList();

	void ExecuteList(FRHICommandListBase& CmdList);
	void ExecuteList(FRHICommandListImmediate& CmdList);
	void LatchBypass();

	static FGraphEventRef RHIThreadFence();
	static void WaitOnRHIThreadFence(FGraphEventRef& Fence);

	FORCEINLINE_DEBUGGABLE bool Bypass()
	{
#if !UE_BUILD_SHIPPING
		return bLatchedBypass;
#else
		return !!DefaultBypass;
#endif
	}
	FORCEINLINE_DEBUGGABLE bool UseParallelAlgorithms()
	{
#if !UE_BUILD_SHIPPING
		return bLatchedUseParallelAlgorithms;
#else
		return  FApp::ShouldUseThreadingForPerformance() && !Bypass();
#endif
	}
	static void CheckNoOutstandingCmdLists();
	static bool IsRHIThreadActive();

private:

	void ExecuteInner(FRHICommandListBase& CmdList);
	friend class FExecuteRHIThreadTask;
	static void ExecuteInner_DoExecute(FRHICommandListBase& CmdList);

	bool bLatchedBypass;
	bool bLatchedUseParallelAlgorithms;
	friend class FRHICommandListBase;
	FThreadSafeCounter UIDCounter;
	FThreadSafeCounter OutstandingCmdListCount;
	FRHICommandListImmediate CommandListImmediate;
};

extern RHI_API FRHICommandListExecutor GRHICommandList;

FORCEINLINE_DEBUGGABLE FRHICommandListImmediate& FRHICommandListExecutor::GetImmediateCommandList()
{
	return GRHICommandList.CommandListImmediate;
}

struct FScopedCommandListWaitForTasks
{
	FRHICommandListImmediate& RHICmdList;

	FScopedCommandListWaitForTasks(FRHICommandListImmediate& InRHICmdList = FRHICommandListExecutor::GetImmediateCommandList())
		: RHICmdList(InRHICmdList)
	{
	}
	~FScopedCommandListWaitForTasks()
	{
		check(IsInRenderingThread());
		if (GRHIThread)
		{
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_FScopedCommandListWaitForTasks_Dispatch);
				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			}
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_FScopedCommandListWaitForTasks_WaitAsync);
				RHICmdList.ImmediateFlush(EImmediateFlushType::WaitForOutstandingTasksOnly);
			}
		}
		else
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_FScopedCommandListWaitForTasks_Flush);
			RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		}
	}
};

#define DEFINE_RHIMETHOD_CMDLIST(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation)
#define DEFINE_RHIMETHOD(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation)
#define DEFINE_RHIMETHOD_GLOBALTHREADSAFE(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation)

#define DEFINE_RHIMETHOD_GLOBAL(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE_DEBUGGABLE Type RHI##Name ParameterTypesAndNames \
	{ \
		ReturnStatement FRHICommandListExecutor::GetImmediateCommandList().Name ParameterNames; \
	}

#define DEFINE_RHIMETHOD_GLOBALFLUSH(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE_DEBUGGABLE Type RHI##Name ParameterTypesAndNames \
	{ \
		ReturnStatement FRHICommandListExecutor::GetImmediateCommandList().Name ParameterNames; \
	}
#include "RHIMethods.h"
#undef DEFINE_RHIMETHOD
#undef DEFINE_RHIMETHOD_CMDLIST
#undef DEFINE_RHIMETHOD_GLOBAL
#undef DEFINE_RHIMETHOD_GLOBALFLUSH
#undef DEFINE_RHIMETHOD_GLOBALTHREADSAFE


#include "RHICommandList.inl"
