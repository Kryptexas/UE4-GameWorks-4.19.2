// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHICommandList.h: RHI Command List definitions for queueing up & executing later.
=============================================================================*/

#pragma once

enum ERHICommandType
{
	// Start the enum at non-zero to help debug
	ERCT_NopBlob = 1000,	// NoOp, maybe has a buffer to be auto alloc/freed
	ERCT_SetRasterizerState,
	ERCT_SetShaderParameter,
	ERCT_SetShaderUniformBuffer,
	ERCT_SetShaderTexture,
	ERCT_SetShaderSampler,
	ERCT_DrawPrimitive,
	ERCT_DrawIndexedPrimitive,
	ERCT_SetBoundShaderState,
	ERCT_SetBlendState,
	ERCT_SetStreamSource,

	ERCT_,
};

class FRHICommandList
{
public:
	enum
	{
		MemSize = 32 * 1024 * 1024,
		Alignment = sizeof(void*),	// Use SIZE_T?
	};

	FRHICommandList()
	{
		Reset();
	}

	template <typename TShaderRHIParamRef>
	FORCEINLINE static void SetShaderUniformBuffer(FRHICommandList* RHICmdList, TShaderRHIParamRef Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBufferRHI)
	{
		if (RHICmdList)
		{
			RHICmdList->SetShaderUniformBuffer(Shader, BaseIndex, UniformBufferRHI);
		}
		else
		{
			RHISetShaderUniformBuffer(Shader, BaseIndex, UniformBufferRHI);
		}
	}
	template <typename TShaderRHIParamRef>
	inline void SetShaderUniformBuffer(TShaderRHIParamRef Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBufferRHI);

	template <typename TShaderRHIParamRef>
	FORCEINLINE static void SetShaderResourceViewParameter(FRHICommandList* RHICmdList, TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV)
	{
		if (RHICmdList)
		{
			RHICmdList->SetShaderResourceViewParameter(Shader, SamplerIndex, SRV);
		}
		else
		{
			RHISetShaderResourceViewParameter(Shader, SamplerIndex, SRV);
		}
	}
	template <typename TShaderRHIParamRef>
	inline void SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV);

	template <typename TShaderRHIParamRef>
	FORCEINLINE static void SetShaderParameter(FRHICommandList* RHICmdList, TShaderRHIParamRef Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* Value, bool bValueInStack = false)
	{
		if (RHICmdList)
		{
			RHICmdList->SetShaderParameter(Shader, BufferIndex, BaseIndex, NumBytes, Value, bValueInStack);
		}
		else
		{
			RHISetShaderParameter(Shader, BufferIndex, BaseIndex, NumBytes, Value);
		}
	}
	template <typename TShaderRHIParamRef>
	inline void SetShaderParameter(TShaderRHIParamRef Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* Value, bool bValueInStack = false);

	template <typename TShaderRHIParamRef>
	FORCEINLINE static void SetShaderTexture(FRHICommandList* RHICmdList, TShaderRHIParamRef Shader, uint32 TextureIndex, FTextureRHIParamRef Texture)
	{
		if (RHICmdList)
		{
			RHICmdList->SetShaderTexture(Shader, TextureIndex, Texture);
		}
		else
		{
			RHISetShaderTexture(Shader, TextureIndex, Texture);
		}
	}
	template <typename TShaderRHIParamRef>
	inline void SetShaderTexture(TShaderRHIParamRef Shader, uint32 TextureIndex, FTextureRHIParamRef Texture);

	template <typename TShaderRHIParamRef>
	FORCEINLINE static void SetShaderSampler(FRHICommandList* RHICmdList, TShaderRHIParamRef Shader, uint32 SamplerIndex, FSamplerStateRHIParamRef State)
	{
		if (RHICmdList)
		{
			RHICmdList->SetShaderSampler(Shader, SamplerIndex, State);
		}
		else
		{
			RHISetShaderSampler(Shader, SamplerIndex, State);
		}
	}
	template <typename TShaderRHIParamRef>
	inline void SetShaderSampler(TShaderRHIParamRef Shader, uint32 SamplerIndex, FSamplerStateRHIParamRef State);

	FORCEINLINE static void SetUAVParameter(FRHICommandList* RHICmdList, FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
	{
		if (RHICmdList)
		{
			RHICmdList->SetUAVParameter(Shader, UAVIndex, UAV);
		}
		else
		{
			RHISetUAVParameter(Shader, UAVIndex, UAV);
		}
	}
	FORCEINLINE static void SetUAVParameter(FRHICommandList* RHICmdList, FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount)
	{
		if (RHICmdList)
		{
			RHICmdList->SetUAVParameter(Shader, UAVIndex, UAV, InitialCount);
		}
		else
		{
			RHISetUAVParameter(Shader, UAVIndex, UAV, InitialCount);
		}
	}
	inline void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV);
	inline void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount);

	FORCEINLINE static void SetBoundShaderState(FRHICommandList* RHICmdList, FBoundShaderStateRHIParamRef BoundShaderState)
	{
		if (RHICmdList)
		{
			RHICmdList->SetBoundShaderState(BoundShaderState);
		}
		else
		{
			RHISetBoundShaderState(BoundShaderState);
		}
	}
	inline void SetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState);

	FORCEINLINE static void SetRasterizerState(FRHICommandList* RHICmdList, FRasterizerStateRHIParamRef State)
	{
		if (RHICmdList)
		{
			RHICmdList->SetRasterizerState(State);
		}
		else
		{
			RHISetRasterizerState(State);
		}
	}
	inline void SetRasterizerState(FRasterizerStateRHIParamRef State);

	FORCEINLINE static void SetBlendState(FRHICommandList* RHICmdList, FBlendStateRHIParamRef State, const FLinearColor& BlendFactor)
	{
		if (RHICmdList)
		{
			RHICmdList->SetBlendState(State, BlendFactor);
		}
		else
		{
			RHISetBlendState(State, BlendFactor);
		}
	}
	FORCEINLINE static void SetBlendState(FRHICommandList* RHICmdList, FBlendStateRHIParamRef State)
	{
		SetBlendState(RHICmdList, State, FLinearColor::White);
	}
	inline void SetBlendState(FBlendStateRHIParamRef State, const FLinearColor& BlendFactor);
	inline void SetBlendState(FBlendStateRHIParamRef State)
	{
		SetBlendState(State, FLinearColor::White);
	}

	FORCEINLINE static void DrawPrimitive(FRHICommandList* RHICmdList, uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		if (RHICmdList)
		{
			RHICmdList->DrawPrimitive(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
		}
		else
		{
			RHIDrawPrimitive(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
		}
	}
	inline void DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances);

	FORCEINLINE static void DrawIndexedPrimitive(FRHICommandList* RHICmdList, FIndexBufferRHIParamRef IndexBuffer, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 MinIndex, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		if (RHICmdList)
		{
			RHICmdList->DrawIndexedPrimitive(IndexBuffer, PrimitiveType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, NumPrimitives, NumInstances);
		}
		else
		{
			RHIDrawIndexedPrimitive(IndexBuffer, PrimitiveType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, NumPrimitives, NumInstances);
		}
	}
	inline void DrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 MinIndex, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances);

	FORCEINLINE static void SetStreamSource(FRHICommandList* RHICmdList, uint32 StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint32 Offset)
	{
		if (RHICmdList)
		{
			RHICmdList->SetStreamSource(StreamIndex, VertexBuffer, Stride, Offset);
		}
		else
		{
			RHISetStreamSource(StreamIndex, VertexBuffer, Stride, Offset);
		}
	}
	inline void SetStreamSource(uint32 StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint32 Offset);


	uint8* Alloc(SIZE_T InSize)
	{
		InSize = (InSize + (Alignment - 1)) & ~(Alignment - 1);
		check(Tail + InSize + Alignment <= &Mem[MemSize]);
		uint8* New = Tail;
		Tail += InSize;
		return New;
	}

	const uint8* GetHead() const
	{
		return &Mem[0];
	}

	const uint8* GetTail() const
	{
		return Tail;
	}

protected:
	uint8 Mem[MemSize];

	enum EState
	{
		Ready,
		Executing,
		Kicked,
	};
	EState State;
	uint8* Tail;
	uint32 NumCommands;

	inline bool CanAddCommand() const
	{
		return (State == Ready);
	}

	bool CanFitCommand(SIZE_T Size) const
	{
		return (Size + (Tail - Mem) <= MemSize);
	}

	template <typename TCmd>
	FORCEINLINE TCmd* AddCommand()
	{
		check(CanAddCommand());
		check(CanFitCommand(sizeof(TCmd)));
		TCmd* Cmd = (TCmd*)Alloc(sizeof(TCmd));
		++NumCommands;
		return Cmd;
	}

	void Reset()
	{
		State = Kicked;
		NumCommands = 0;
		Tail = &Mem[0];
		Tail = Align(Tail, Alignment);
	}

	friend class FRHICommandListExecutor;
};

class RHI_API FRHICommandListExecutor
{
public:
	FRHICommandListExecutor();
	FRHICommandList* CreateList();
	void ExecuteAndFreeList(FRHICommandList* CmdList);

	void Verify();

private:
	FCriticalSection CriticalSection;
	bool bLock;
};
extern RHI_API FRHICommandListExecutor GRHICommandList;

DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Command List Execute"),STAT_RHICmdListExecuteTime,STATGROUP_RHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Command List Enqueue"),STAT_RHICmdListEnqueueTime,STATGROUP_RHI, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("RHI Command List memory"),STAT_RHICmdListMemory,STATGROUP_RHI,RHI_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("RHI Command List count"),STAT_RHICmdListCount,STATGROUP_RHI,RHI_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Counter TEMP"), STAT_RHICounterTEMP,STATGROUP_RHI,RHI_API);

#include "RHICommandList.inl"
