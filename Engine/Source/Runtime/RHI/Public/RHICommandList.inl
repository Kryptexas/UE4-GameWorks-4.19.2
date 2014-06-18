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
		RHISetShaderUniformBuffer(Shader, BaseIndex, UniformBuffer);
		return;
	}
	auto* Cmd = AddCommand<FRHICommandSetShaderUniformBuffer>();
	Cmd->Set<TShaderRHIParamRef>(Shader, BaseIndex, UniformBuffer);
	Shader->AddRef();
	UniformBuffer->AddRef();
}

template <typename TShaderRHIParamRef>
FORCEINLINE void FRHICommandList::SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV)
{
	if (Bypass())
	{
		RHISetShaderResourceViewParameter(Shader, SamplerIndex, SRV);
		return;
	}
	checkf(0, TEXT("TODO: Non-bypass version"));
}

template <typename TShaderRHIParamRef>
FORCEINLINE void FRHICommandList::SetShaderParameter(TShaderRHIParamRef Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue, bool bValueInStack)
{
	if (Bypass())
	{
		RHISetShaderParameter(Shader, BufferIndex, BaseIndex, NumBytes, NewValue);
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
		RHISetShaderTexture(Shader, TextureIndex, Texture);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetShaderTexture>();
	Cmd->Set<TShaderRHIParamRef>(Shader, TextureIndex, Texture);
	Shader->AddRef();
	Texture->AddRef();
}

template <typename TShaderRHIParamRef>
FORCEINLINE void FRHICommandList::SetShaderSampler(TShaderRHIParamRef Shader, uint32 SamplerIndex, FSamplerStateRHIParamRef State)
{
	if (Bypass())
	{
		RHISetShaderSampler(Shader, SamplerIndex, State);
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
		RHISetUAVParameter(Shader, UAVIndex, UAV);
		return;
	}
	checkf(0, TEXT("TODO: Non-bypass version"));
}

FORCEINLINE void FRHICommandList::SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount)
{
	if (Bypass())
	{
		RHISetUAVParameter(Shader, UAVIndex, UAV, InitialCount);
		return;
	}
	checkf(0, TEXT("TODO: Non-bypass version"));
}

FORCEINLINE void FRHICommandList::SetRasterizerState(FRasterizerStateRHIParamRef State)
{
	if (Bypass())
	{
		RHISetRasterizerState(State);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetRasterizerState>();
	Cmd->Set(State);
	State->AddRef();
}

FORCEINLINE void FRHICommandList::DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances)
{
	if (Bypass())
	{
		RHIDrawPrimitive(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandDrawPrimitive>();
	Cmd->Set(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
}

FORCEINLINE void FRHICommandList::DrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 MinIndex, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
{
	if (Bypass())
	{
		RHIDrawIndexedPrimitive(IndexBuffer, PrimitiveType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, NumPrimitives, NumInstances);
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
		RHISetBoundShaderState(BoundShaderState);
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
		RHISetBlendState(State, BlendFactor);
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
		RHISetStreamSource(StreamIndex, VertexBuffer, Stride, Offset);
		return;
	}

	auto* Cmd = AddCommand<FRHICommandSetStreamSource>();
	Cmd->Set(StreamIndex, VertexBuffer, Stride, Offset);
	VertexBuffer->AddRef();
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
	template <typename T>
	FORCEINLINE T* NextCommand()
	{
		T* RHICmd = (T*)CmdPtr;
		CmdPtr += sizeof(T) + RHICmd->ExtraSize();
		CmdPtr = Align(CmdPtr, FRHICommandList::Alignment);
		++NumCommands;
		if (CmdPtr >= CmdTail)
		{
			Page = Page->NextPage;
			CmdPtr = Page ? Page->Head : nullptr;
			CmdTail = Page ? Page->Current : nullptr;
		}

		return RHICmd;
	}

	// Specialization for EndOfPage
	template <>
	FORCEINLINE FRHICommandNopEndOfPage* NextCommand<FRHICommandNopEndOfPage>()
	{
		auto* RHICmd = (FRHICommandNopEndOfPage*)CmdPtr;
		Page = Page->NextPage;
		CmdPtr = Page ? Page->Head : nullptr;
		CmdTail = Page ? Page->Current : nullptr;

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
