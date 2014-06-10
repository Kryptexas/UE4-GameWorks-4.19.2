// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHICommandList.inl: RHI Command List inline definitions.
=============================================================================*/

#ifndef __RHICOMMANDLIST_INL__
#define __RHICOMMANDLIST_INL__
#include "RHICommandList.h"

struct FRHICommand
{
	union
	{
		SIZE_T			RawType;
		ERHICommandType	Type;
	};
};

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

struct FRHICommandNopBlob : public FRHICommand
{
	void* Data;
	uint32 Size;
	FORCEINLINE void Set(FRHICommandList* List, uint32 InSize, const void* InData)
	{
		Type = ERCT_NopBlob;
		Size = InSize;
		Data = List->Alloc(InSize);
		FMemory::Memcpy(Data, InData, InSize);
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
	check(0);
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
		auto* Blob = AddCommand<FRHICommandNopBlob>();
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

	check(0);
}
FORCEINLINE void FRHICommandList::SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount)
{
	if (Bypass())
	{
		RHISetUAVParameter(Shader, UAVIndex, UAV, InitialCount);
		return;
	}
	check(0);
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

#endif	// __RHICOMMANDLIST_INL__
