// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHICommandList.inl: RHI Command List inline definitions.
=============================================================================*/

#ifndef __RHICOMMANDLIST_INL__
#define __RHICOMMANDLIST_INL__
#include "RHICommandList.h"

// No constructors are given for classes derived from FRHICommand as they are not created using
// a classical 'new' for perf reasons.

struct FRHICommandPerShader : public FRHICommand
{
	FRHIResource* Shader;
	EShaderFrequency ShaderFrequency;

	template <typename TShaderRHIParamRef>
	FORCEINLINE void SetShader(TShaderRHIParamRef InShader)
	{
		Shader = InShader;
		ShaderFrequency = (EShaderFrequency)TRHIShaderToEnum<TShaderRHIParamRef>::ShaderFrequency;
	}
};

struct FRHICommandNopEndOfPage : public FRHICommand
{
	enum
	{
		IsEndOfPage = 1
	};
};

struct FRHICommandNopBlob : public FRHICommand
{
	void* Data;
	uint32 Size;
	FORCEINLINE void Set(FRHICommandList* List, uint32 InSize, const void* InData)
	{
		Type = ERCT_NopBlob;
		Size = InSize;

		// Data is stored after 'this'
		Data = &this[1];
		FMemory::Memcpy(Data, InData, InSize);
	}

	uint32 ExtraSize() const
	{
		return Size;
	}
};

struct FRHICommandSetRasterizerState : public FRHICommand
{
	FRasterizerStateRHIParamRef State;
	FORCEINLINE void Set(FRasterizerStateRHIParamRef InState)
	{
		Type = ERCT_SetRasterizerState;
		State = InState;
	}
};

struct FRHICommandSetDepthStencilState : public FRHICommand
{
	FDepthStencilStateRHIParamRef State;
	uint32 StencilRef;
	FORCEINLINE void Set(FDepthStencilStateRHIParamRef InState, uint32 InStencilRef)
	{
		Type = ERCT_SetDepthStencilState;
		State = InState;
		StencilRef = InStencilRef;
	}
};

struct FRHICommandSetShaderParameter : public FRHICommandPerShader
{
	const void* NewValue;
	uint32 BufferIndex;
	uint32 BaseIndex;
	uint32 NumBytes;
	template <typename TShaderRHIParamRef>
	FORCEINLINE void Set(TShaderRHIParamRef InShader, uint32 InBufferIndex, uint32 InBaseIndex, uint32 InNumBytes, const void* InNewValue)
	{
		FRHICommandPerShader::SetShader<TShaderRHIParamRef>(InShader);
		Type = ERCT_SetShaderParameter;
		Shader = InShader;
		BufferIndex = InBufferIndex;
		BaseIndex = InBaseIndex;
		NumBytes = InNumBytes;
		NewValue = InNewValue;
	}
};

struct FRHICommandSetShaderUniformBuffer : public FRHICommandPerShader
{
	uint32 BaseIndex;
	FUniformBufferRHIParamRef UniformBuffer;
	template <typename TShaderRHIParamRef>
	FORCEINLINE void Set(TShaderRHIParamRef InShader, uint32 InBaseIndex, FUniformBufferRHIParamRef InUniformBuffer)
	{
		FRHICommandPerShader::SetShader<TShaderRHIParamRef>(InShader);
		Type = ERCT_SetShaderUniformBuffer;
		Shader = InShader;
		BaseIndex = InBaseIndex;
		UniformBuffer = InUniformBuffer;
	}
};

struct FRHICommandSetShaderTexture : public FRHICommandPerShader
{
	uint32 TextureIndex;
	FTextureRHIParamRef Texture;
	template <typename TShaderRHIParamRef>
	FORCEINLINE void Set(TShaderRHIParamRef InShader, uint32 InTextureIndex, FTextureRHIParamRef InTexture)
	{
		FRHICommandPerShader::SetShader<TShaderRHIParamRef>(InShader);
		Type = ERCT_SetShaderTexture;
		Shader = InShader;
		TextureIndex = InTextureIndex;
		Texture = InTexture;
	}
};

struct FRHICommandSetShaderResourceViewParameter : public FRHICommandPerShader
{
	uint32 SamplerIndex;
	FShaderResourceViewRHIParamRef SRV;
	template <typename TShaderRHIParamRef>
	FORCEINLINE void Set(TShaderRHIParamRef InShader, uint32 InSamplerIndex, FShaderResourceViewRHIParamRef InSRV)
	{
		FRHICommandPerShader::SetShader<TShaderRHIParamRef>(InShader);
		Type = ERCT_SetShaderResourceViewParameter;
		Shader = InShader;
		SamplerIndex = InSamplerIndex;
		SRV = InSRV;
	}
};

struct FRHICommandSetUAVParameter : public FRHICommandPerShader
{
	uint32 UAVIndex;
	FUnorderedAccessViewRHIParamRef UAV;
	template <typename TShaderRHIParamRef>
	FORCEINLINE void Set(TShaderRHIParamRef InShader, uint32 InUAVIndex, FUnorderedAccessViewRHIParamRef InUAV)
	{
		FRHICommandPerShader::SetShader<TShaderRHIParamRef>(InShader);
		Type = ERCT_SetUAVParameter;
		Shader = InShader;
		UAVIndex = InUAVIndex;
		UAV = InUAV;
	}
};

struct FRHICommandSetUAVParameter_IntialCount : public FRHICommandPerShader
{
	uint32 UAVIndex;
	FUnorderedAccessViewRHIParamRef UAV;
	uint32 InitialCount;
	template <typename TShaderRHIParamRef>
	FORCEINLINE void Set(TShaderRHIParamRef InShader, uint32 InUAVIndex, FUnorderedAccessViewRHIParamRef InUAV, uint32 InInitialCount)
	{
		FRHICommandPerShader::SetShader<TShaderRHIParamRef>(InShader);
		Type = ERCT_SetUAVParameter_IntialCount;
		Shader = InShader;
		UAVIndex = InUAVIndex;
		UAV = InUAV;
		InitialCount = InInitialCount;
	}
};

struct FRHICommandSetShaderSampler : public FRHICommandPerShader
{
	uint32 SamplerIndex;
	FSamplerStateRHIParamRef Sampler;
	template <typename TShaderRHIParamRef>
	FORCEINLINE void Set(TShaderRHIParamRef InShader, uint32 InSamplerIndex, FSamplerStateRHIParamRef InSampler)
	{
		FRHICommandPerShader::SetShader<TShaderRHIParamRef>(InShader);
		Type = ERCT_SetShaderSampler;
		Shader = InShader;
		SamplerIndex = InSamplerIndex;
		Sampler = InSampler;
	}
};

struct FRHICommandDrawPrimitive : public FRHICommand
{
	uint32 PrimitiveType;
	uint32 BaseVertexIndex;
	uint32 NumPrimitives;
	uint32 NumInstances;
	FORCEINLINE void Set(uint32 InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances)
	{
		Type = ERCT_DrawPrimitive;
		PrimitiveType = InPrimitiveType;
		BaseVertexIndex = InBaseVertexIndex;
		NumPrimitives = InNumPrimitives;
		NumInstances = InNumInstances;
	}
};

struct FRHICommandDrawIndexedPrimitive : public FRHICommand
{
	FIndexBufferRHIParamRef IndexBuffer;
	uint32 PrimitiveType;
	int32 BaseVertexIndex;
	uint32 MinIndex;
	uint32 NumVertices;
	uint32 StartIndex;
	uint32 NumPrimitives;
	uint32 NumInstances;
	FORCEINLINE void Set(FIndexBufferRHIParamRef InIndexBuffer, uint32 InPrimitiveType, int32 InBaseVertexIndex, uint32 InMinIndex, uint32 InNumVertices, uint32 InStartIndex, uint32 InNumPrimitives, uint32 InNumInstances)
	{
		Type = ERCT_DrawIndexedPrimitive;
		IndexBuffer = InIndexBuffer;
		PrimitiveType = InPrimitiveType;
		BaseVertexIndex = InBaseVertexIndex;
		MinIndex = InMinIndex;
		NumVertices = InNumVertices;
		StartIndex = InStartIndex;
		NumPrimitives = InNumPrimitives;
		NumInstances = InNumInstances;
	}
};

struct FRHICommandSetBoundShaderState : public FRHICommand
{
	FBoundShaderStateRHIParamRef BoundShaderState;
	FORCEINLINE void Set(FBoundShaderStateRHIParamRef InBoundShaderState)
	{
		Type = ERCT_SetBoundShaderState;
		BoundShaderState = InBoundShaderState;
	}
};

struct FRHICommandSetBlendState : public FRHICommand
{
	FBlendStateRHIParamRef State;
	FLinearColor BlendFactor;
	FORCEINLINE void Set(FBlendStateRHIParamRef InState, const FLinearColor& InBlendFactor)
	{
		Type = ERCT_SetBlendState;
		State = InState;
		BlendFactor = InBlendFactor;
	}
};

struct FRHICommandSetStreamSource : public FRHICommand
{
	uint32 StreamIndex;
	FVertexBufferRHIParamRef VertexBuffer;
	uint32 Stride;
	uint32 Offset;
	FORCEINLINE void Set(uint32 InStreamIndex, FVertexBufferRHIParamRef InVertexBuffer, uint32 InStride, uint32 InOffset)
	{
		Type = ERCT_SetStreamSource;
		StreamIndex = InStreamIndex;
		VertexBuffer = InVertexBuffer;
		Stride = InStride;
		Offset = InOffset;
	}
};

template <typename TShaderRHIParamRef>
FORCEINLINE void FRHICommandList::SetShaderUniformBuffer(TShaderRHIParamRef Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
{
	if (Bypass())
	{
		SetShaderUniformBuffer_Internal(Shader, BaseIndex, UniformBuffer);
		return;
	}
	auto* Cmd = AddCommand<FRHICommandSetShaderUniformBuffer>();
	Cmd->Set<TShaderRHIParamRef>(Shader, BaseIndex, UniformBuffer);
	Shader->AddRef();
	UniformBuffer->AddRef();
}

template <typename TShaderRHIParamRef>
FORCEINLINE void FRHICommandList::SetShaderParameter(TShaderRHIParamRef Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue, bool bValueInStack)
{
	if (Bypass())
	{
		SetShaderParameter_Internal(Shader, BufferIndex, BaseIndex, NumBytes, NewValue);
		return;
	}

	if (bValueInStack)
	{
		auto* Blob = AddCommand<FRHICommandNopBlob>(NumBytes);
		Blob->Set(this, NumBytes, NewValue);
		NewValue = Blob->Data;
	}
	auto* Cmd = AddCommand<FRHICommandSetShaderParameter>();
	Cmd->Set<TShaderRHIParamRef>(Shader, BufferIndex, BaseIndex, NumBytes, NewValue);
	Shader->AddRef();
}

template <typename TShaderRHIParamRef>
FORCEINLINE void FRHICommandList::SetShaderTexture(TShaderRHIParamRef Shader, uint32 TextureIndex, FTextureRHIParamRef Texture)
{
	if (Bypass())
	{
		SetShaderTexture_Internal(Shader, TextureIndex, Texture);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetShaderTexture>();
	Cmd->Set<TShaderRHIParamRef>(Shader, TextureIndex, Texture);
	Shader->AddRef();
	Texture->AddRef();
}

template <typename TShaderRHIParamRef>
FORCEINLINE void FRHICommandList::SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV)
{
	if (Bypass())
	{
		SetShaderResourceViewParameter_Internal(Shader, SamplerIndex, SRV);
		return;
	}
	auto* Cmd = AddCommand<FRHICommandSetShaderResourceViewParameter>();
	Cmd->Set<TShaderRHIParamRef>(Shader, SamplerIndex, SRV);
	Shader->AddRef();
	SRV->AddRef();
}


template <typename TShaderRHIParamRef>
FORCEINLINE void FRHICommandList::SetShaderSampler(TShaderRHIParamRef Shader, uint32 SamplerIndex, FSamplerStateRHIParamRef State)
{
	if (Bypass())
	{
		SetShaderSampler_Internal(Shader, SamplerIndex, State);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetShaderSampler>();
	Cmd->Set<TShaderRHIParamRef>(Shader, SamplerIndex, State);
	Shader->AddRef();
	State->AddRef();
}

FORCEINLINE void FRHICommandList::SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
{
	if (Bypass())
	{
		SetUAVParameter_Internal(Shader, UAVIndex, UAV);
		return;
	}
	auto* Cmd = AddCommand<FRHICommandSetUAVParameter>();
	Cmd->Set<FComputeShaderRHIParamRef>(Shader, UAVIndex, UAV);
	Shader->AddRef();
	UAV->AddRef();
}

FORCEINLINE void FRHICommandList::SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount)
{
	if (Bypass())
	{
		SetUAVParameter_Internal(Shader, UAVIndex, UAV, InitialCount);
		return;
	}
	auto* Cmd = AddCommand<FRHICommandSetUAVParameter_IntialCount>();
	Cmd->Set<FComputeShaderRHIParamRef>(Shader, UAVIndex, UAV, InitialCount);
	Shader->AddRef();
	UAV->AddRef();
}

FORCEINLINE void FRHICommandList::SetRasterizerState(FRasterizerStateRHIParamRef State)
{
	if (Bypass())
	{
		SetRasterizerState_Internal(State);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetRasterizerState>();
	Cmd->Set(State);
	State->AddRef();
}

FORCEINLINE void FRHICommandList::SetDepthStencilState(FDepthStencilStateRHIParamRef State, uint32 StencilRef)
{
	if (Bypass())
	{
		SetDepthStencilState_Internal(State, StencilRef);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetDepthStencilState>();
	Cmd->Set(State, StencilRef);
	State->AddRef();
}

FORCEINLINE void FRHICommandList::DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances)
{
	if (Bypass())
	{
		DrawPrimitive_Internal(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandDrawPrimitive>();
	Cmd->Set(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
}

FORCEINLINE void FRHICommandList::DrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 MinIndex, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
{
	if (Bypass())
	{
		DrawIndexedPrimitive_Internal(IndexBuffer, PrimitiveType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, NumPrimitives, NumInstances);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandDrawIndexedPrimitive>();
	Cmd->Set(IndexBuffer, PrimitiveType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, NumPrimitives, NumInstances);
	IndexBuffer->AddRef();
}

FORCEINLINE void FRHICommandList::SetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState)
{
	if (Bypass())
	{
		SetBoundShaderState_Internal(BoundShaderState);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetBoundShaderState>();
	Cmd->Set(BoundShaderState);
	BoundShaderState->AddRef();
}

FORCEINLINE void FRHICommandList::SetBlendState(FBlendStateRHIParamRef State, const FLinearColor& BlendFactor)
{
	if (Bypass())
	{
		SetBlendState_Internal(State, BlendFactor);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetBlendState>();
	Cmd->Set(State, BlendFactor);
	State->AddRef();
}

FORCEINLINE void FRHICommandList::SetStreamSource(uint32 StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint32 Offset)
{
	if (Bypass())
	{
		SetStreamSource_Internal(StreamIndex, VertexBuffer, Stride, Offset);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetStreamSource>();
	Cmd->Set(StreamIndex, VertexBuffer, Stride, Offset);
	VertexBuffer->AddRef();
}

//--

FORCEINLINE void FRHICommandList::Flush()
{
	if (HasCommands())
	{
		GRHICommandList.ExecuteList(*this);
	}
}


// Helper class for traversing a FRHICommandList
class FRHICommandListIterator
{
public:
	FRHICommandListIterator(FRHICommandList& CmdList)
	{
		NumCommands = 0;
		CmdListNumCommands = CmdList.NumCommands;
		Page = CmdList.MemManager.FirstPage;
		CmdPtr = Page->Head;
		CmdTail = Page->Current;
	}

	FORCEINLINE bool HasCommandsLeft() const
	{
		return (CmdPtr < CmdTail);
	}

	// Current command
	FORCEINLINE FRHICommand* operator * ()
	{
		return (FRHICommand*)CmdPtr;
	}

	// Get the next RHICommand and advance the iterator
	template <typename TCmd>
	FORCEINLINE TCmd* NextCommand()
	{
		TCmd* RHICmd = (TCmd*)CmdPtr;
		//::OutputDebugStringW(*FString::Printf(TEXT("Exec %d: %d @ 0x%p, %d bytes\n"), NumCommands, (int32)RHICmd->Type, (void*)RHICmd, sizeof(TCmd) + RHICmd->ExtraSize()));
		CmdPtr += sizeof(TCmd) + RHICmd->ExtraSize();
		CmdPtr = Align(CmdPtr, FRHICommandList::Alignment);

		//@todo-rco: Fix me!
		if (!TCmd::IsEndOfPage)
		{
			// Don't count EOP as that is an allocator construct
			++NumCommands;
		}

		if (TCmd::IsEndOfPage || CmdPtr >= CmdTail)
		{
			Page = Page->NextPage;
			CmdPtr = Page ? Page->Head : nullptr;
			CmdTail = Page ? Page->Current : nullptr;
		}

		return RHICmd;
	}

	void CheckNumCommands()
	{
		checkf(CmdListNumCommands == NumCommands, TEXT("Missed %d Commands!"), CmdListNumCommands - NumCommands);
		checkf(CmdPtr == CmdTail, TEXT("Mismatched Tail Pointer!"));
	}

protected:
	FRHICommandList::FMemManager::FPage* Page;
	uint32 NumCommands;
	const uint8* CmdPtr;
	const uint8* CmdTail;
	uint32 CmdListNumCommands;
};

#endif	// __RHICOMMANDLIST_INL__
