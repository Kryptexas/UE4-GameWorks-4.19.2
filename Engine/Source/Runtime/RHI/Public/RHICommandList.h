// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHICommandList.h: RHI Command List definitions for queueing up & executing later.
=============================================================================*/

#pragma once

enum ERHICommandType
{
	// Start the enum at non-zero to help debugging

	// List control
	ERCT_NopBlob				= 512,	// NoOp, maybe has a buffer to be auto alloc/freed (512=0x200)
	ERCT_NopEndOfPage,					// NoOp, stop traversing the current page

	// RHI Commands
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
	ERCT_SetDepthStencilState,
	ERCT_SetShaderResourceViewParameter,
	ERCT_SetUAVParameter,
	ERCT_SetUAVParameter_IntialCount,
	ERCT_SetViewport,
	ERCT_SetScissorRect,
	ERCT_SetRenderTargets,
	ERCT_,
};

struct FRHICommand
{
	// This is a union only to force alignment to SIZE_T
	union
	{
		ERHICommandType	Type;
		SIZE_T			RawType;
	};

	uint32 ExtraSize() const
	{
		return 0;
	}

	enum
	{
		IsEndOfPage = 0,
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

struct FRHICommandSetViewport : public FRHICommand
{
	uint32 MinX;
	uint32 MinY;
	float MinZ;
	uint32 MaxX;
	uint32 MaxY;
	float MaxZ;
	FORCEINLINE void Set(uint32 InMinX, uint32 InMinY, float InMinZ, uint32 InMaxX, uint32 InMaxY, float InMaxZ)
	{
		Type = ERCT_SetViewport;
		MinX = InMinX;
		MinY = InMinY;
		MinZ = InMinZ;
		MaxX = InMaxX;
		MaxY = InMaxY;
		MaxZ = InMaxZ;
	}
};

struct FRHICommandSetScissorRect : public FRHICommand
{
	bool bEnable;
	uint32 MinX;
	uint32 MinY;
	uint32 MaxX;
	uint32 MaxY;
	FORCEINLINE void Set(bool InbEnable, uint32 InMinX, uint32 InMinY, uint32 InMaxX, uint32 InMaxY)
	{
		Type = ERCT_SetScissorRect;
		bEnable = InbEnable;
		MinX = InMinX;
		MinY = InMinY;
		MaxX = InMaxX;
		MaxY = InMaxY;
	}
};

struct FRHICommandSetRenderTargets : public FRHICommand
{
	uint32 NewNumSimultaneousRenderTargets;
	FRHIRenderTargetView NewRenderTargetsRHI[MaxSimultaneousRenderTargets];
	FTextureRHIParamRef NewDepthStencilTargetRHI;
	uint32 NewNumUAVs;
	FUnorderedAccessViewRHIParamRef UAVs[MaxSimultaneousUAVs];

	FORCEINLINE void Set(
		uint32 InNewNumSimultaneousRenderTargets,
		const FRHIRenderTargetView* InNewRenderTargetsRHI,
		FTextureRHIParamRef InNewDepthStencilTargetRHI,
		uint32 InNewNumUAVs,
		const FUnorderedAccessViewRHIParamRef* InUAVs
		)

	{
		Type = ERCT_SetRenderTargets;
		check(NewNumSimultaneousRenderTargets <= MaxSimultaneousRenderTargets && NewNumUAVs <= MaxSimultaneousUAVs);
		NewNumSimultaneousRenderTargets = InNewNumSimultaneousRenderTargets;
		for (uint32 Index = 0; Index < NewNumSimultaneousRenderTargets; Index++)
		{
			NewRenderTargetsRHI[Index] = InNewRenderTargetsRHI[Index];
		}
		NewDepthStencilTargetRHI = InNewDepthStencilTargetRHI;
		NewNumUAVs = InNewNumUAVs;
		for (uint32 Index = 0; Index < NewNumUAVs; Index++)
		{
			UAVs[Index] = InUAVs[Index];
		}
	}
};



class RHI_API FRHICommandList : public FNoncopyable
{
private:

	struct FMemManager
	{
		struct FPage
		{
			enum
			{
				PageSize = 64 * 1024
			};

			FPage() :
				NextPage(nullptr)
			{
				Head = (uint8*)FMemory::Malloc(PageSize, Alignment);
				Current = Head;
				Tail = Head + PageSize;
				//::OutputDebugStringW(*FString::Printf(TEXT("PAGE %p, %p - %p\n"), this, Head, Tail));
			}

			~FPage()
			{
				FMemory::Free(Head);
				Head = Current = Tail = nullptr;

				// Caller/owner is responsible for freeing the linked list
				NextPage = nullptr;
			}

			void Reset()
			{
				Current = Head;
			}

			FORCEINLINE SIZE_T MemUsed() const
			{
				return Current - Head;
			}

			FORCEINLINE SIZE_T MemAvailable() const
			{
				return Tail - Current;
			}

			FORCEINLINE bool CanFitAllocation(SIZE_T AlignedSize) const
			{
				return (AlignedSize + Current <= Tail);
			}

			FORCEINLINE uint8* Alloc(SIZE_T AlignedSize)
			{
				auto* Ptr = Current;
				Current += AlignedSize;
				return Ptr;
			}

			uint8* Current;
			uint8* Tail;
			uint8* Head;

			FPage* NextPage;
		};

		FMemManager();
		~FMemManager();

		FORCEINLINE uint8* Alloc(SIZE_T InSize)
		{
			InSize = (InSize + (Alignment - 1)) & ~(Alignment - 1);
			bool bTryAgain = false;

			if (LastPage)
			{
				if (LastPage->CanFitAllocation(InSize))
				{
					return LastPage->Alloc(InSize);
				}

				// Insert End Of Page if there's trailing space
				if (LastPage->MemAvailable() >= sizeof(FRHICommand))
				{
					//@todo-rco: Fix me!
					// This is an allocator command, so it should not be counted during regular execution...
					auto* EndOfPageCmd = (FRHICommand*)LastPage->Alloc(sizeof(FRHICommand));
					EndOfPageCmd->Type = ERCT_NopEndOfPage;
				}
			}

			checkfSlow(InSize <= FPage::PageSize, TEXT("Can't allocate an FPage bigger than %d!"), FPage::PageSize);
			if (!FirstPage)
			{
				FirstPage = new FPage();
				check(!LastPage);
				LastPage = FirstPage;
			}
			else
			{
				LastPage->NextPage = new FPage();
				LastPage = LastPage->NextPage;
			}
			return LastPage->Alloc(InSize);
		}

		const SIZE_T GetUsedMemory() const;

		void Reset();

		FPage* FirstPage;
		FPage* LastPage;
	};

	FMemManager MemManager;

public:
	enum
	{
		Alignment = sizeof(SIZE_T),
	};

	FRHICommandList()
	{
		Reset();
	}
	~FRHICommandList()
	{
		Flush();
	}
	inline void Flush();

	template <typename TShaderRHIParamRef>
	FORCEINLINE void SetShaderUniformBuffer(TShaderRHIParamRef Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
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
	FORCEINLINE void SetShaderParameter(TShaderRHIParamRef Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue, bool bValueInStack = false)
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
	FORCEINLINE void SetShaderTexture(TShaderRHIParamRef Shader, uint32 TextureIndex, FTextureRHIParamRef Texture)
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
	FORCEINLINE void SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV)
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
	FORCEINLINE void SetShaderSampler(TShaderRHIParamRef Shader, uint32 SamplerIndex, FSamplerStateRHIParamRef State)
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

	FORCEINLINE void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
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

	FORCEINLINE void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount)
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

	FORCEINLINE void SetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState)
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

	FORCEINLINE void SetRasterizerState(FRasterizerStateRHIParamRef State)
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

	FORCEINLINE void SetBlendState(FBlendStateRHIParamRef State, const FLinearColor& BlendFactor = FLinearColor::White)
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

	FORCEINLINE void DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		if (Bypass())
		{
			DrawPrimitive_Internal(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
			return;
		}

		auto* Cmd = AddCommand<FRHICommandDrawPrimitive>();
		Cmd->Set(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
	}

	FORCEINLINE void DrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 MinIndex, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
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

	FORCEINLINE void SetStreamSource(uint32 StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint32 Offset)
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

	FORCEINLINE void SetDepthStencilState(FDepthStencilStateRHIParamRef NewStateRHI, uint32 StencilRef = 0)
	{
		if (Bypass())
		{
			SetDepthStencilState_Internal(NewStateRHI, StencilRef);
			return;
		}

		auto* Cmd = AddCommand<FRHICommandSetDepthStencilState>();
		Cmd->Set(NewStateRHI, StencilRef);
		NewStateRHI->AddRef();
	}

	FORCEINLINE void SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ)
	{
		if (Bypass())
		{
			SetViewport_Internal(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetViewport>();
		Cmd->Set(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
	}

	FORCEINLINE void SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
	{
		if (Bypass())
		{
			SetScissorRect_Internal(bEnable, MinX, MinY, MaxX, MaxY);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetScissorRect>();
		Cmd->Set(bEnable,  MinX, MinY, MaxX, MaxY);
	}

	FORCEINLINE void SetRenderTargets(
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
		auto* Cmd = AddCommand<FRHICommandSetRenderTargets>();
		Cmd->Set(
			NewNumSimultaneousRenderTargets,
			NewRenderTargetsRHI,
			NewDepthStencilTargetRHI,
			NewNumUAVs,
			UAVs);
		for (uint32 Index = 0; Index < NewNumSimultaneousRenderTargets; Index++)
		{
			NewRenderTargetsRHI[Index].Texture->AddRef();
		}
		NewDepthStencilTargetRHI->AddRef();
		for (uint32 Index = 0; Index < NewNumUAVs; Index++)
		{
			UAVs[Index]->AddRef();
		}

	}

	const SIZE_T GetUsedMemory() const;

private:
	bool bExecuting;
	uint32 NumCommands;

	FORCEINLINE uint8* Alloc(SIZE_T InSize)
	{
		return MemManager.Alloc(InSize);
	}

	FORCEINLINE bool HasCommands() const
	{
		return (NumCommands > 0);
	}

	template <typename TCmd>
	FORCEINLINE TCmd* AddCommand(uint32 ExtraData = 0)
	{
		checkSlow(!bExecuting);
		TCmd* Cmd = (TCmd*)Alloc(sizeof(TCmd) + ExtraData);
		//::OutputDebugStringW(*FString::Printf(TEXT("Add %d: @ 0x%p, %d bytes\n"), NumCommands, (void*)Cmd, (sizeof(TCmd) + ExtraData)));
		++NumCommands;
		return Cmd;
	}

	void Reset()
	{
		bExecuting = false;
		NumCommands = 0;
		MemManager.Reset();
	}

	inline bool Bypass();

	friend class FRHICommandListExecutor;
	friend class FRHICommandListIterator;

	// Used here as we really want Alloc() to only work for this class, nobody else should use it
	friend struct FRHICommandNopBlob;
};

class RHI_API FRHICommandListImmediate : public FRHICommandList
{
	friend class FRHICommandListExecutor;
	FRHICommandListImmediate()
	{
	}
public:

	#define DEFINE_RHIMETHOD_CMDLIST(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation)
	#define DEFINE_RHIMETHOD(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE Type Name ParameterTypesAndNames \
		{ \
			Flush(); \
			ReturnStatement Name##_Internal ParameterNames; \
		}
	#define DEFINE_RHIMETHOD_GLOBAL(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE Type Name ParameterTypesAndNames \
		{ \
			ReturnStatement RHI##Name ParameterNames; \
		}
	#define DEFINE_RHIMETHOD_GLOBALFLUSH(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE Type Name ParameterTypesAndNames \
		{ \
			Flush(); \
			ReturnStatement Name##_Internal ParameterNames; \
		}
	#include "RHIMethods.h"
	#undef DEFINE_RHIMETHOD
	#undef DEFINE_RHIMETHOD_CMDLIST
	#undef DEFINE_RHIMETHOD_GLOBAL
	#undef DEFINE_RHIMETHOD_GLOBALFLUSH
};

#if 0
class RHI_API FRHICommandListBypass
{
	friend class FRHICommandListExecutor;
	FRHICommandListBypass()
	{
	}
public:
#define DEFINE_RHIMETHOD_CMDLIST(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation) \
	FORCEINLINE Type Name ParameterTypesAndNames \
	{ \
		ReturnStatement Name##_Internal ParameterNames; \
	}
#define DEFINE_RHIMETHOD(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE Type Name ParameterTypesAndNames \
	{ \
		ReturnStatement Name##_Internal ParameterNames; \
	}
#define DEFINE_RHIMETHOD_GLOBAL(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE Type Name ParameterTypesAndNames \
	{ \
		ReturnStatement RHI##Name ParameterNames; \
	}
#define DEFINE_RHIMETHOD_GLOBALFLUSH(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE Type Name ParameterTypesAndNames \
	{ \
		ReturnStatement Name##_Internal ParameterNames; \
	}
#include "RHIMethods.h"
#undef DEFINE_RHIMETHOD
#undef DEFINE_RHIMETHOD_CMDLIST
#undef DEFINE_RHIMETHOD_GLOBAL
#undef DEFINE_RHIMETHOD_GLOBALFLUSH
};
#endif

class RHI_API FRHICommandListExecutor
{
public:
	enum
	{
		DefaultBypass = 1
	};
	FRHICommandListExecutor()
		: bLatchedBypass(!!DefaultBypass)
	{
	}
	static inline FRHICommandListImmediate& GetImmediateCommandList();
	static inline FRHICommandListImmediate& GetRecursiveRHICommandList(); // Only for use by RHI implementations

	void ExecuteList(FRHICommandList& CmdList);

	FORCEINLINE void Verify()
	{
		check(!CommandListImmediate.HasCommands());
	}
	FORCEINLINE bool Bypass()
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		return bLatchedBypass;
#else
		return !!DefaultBypass;
#endif
	}

private:
	bool bLatchedBypass;
	FRHICommandListImmediate CommandListImmediate;

	template <bool bOnlyTestMemAccess>
	void ExecuteList(FRHICommandList& CmdList);
};
extern RHI_API FRHICommandListExecutor GRHICommandList;

FORCEINLINE bool FRHICommandList::Bypass()
{
	return GRHICommandList.Bypass();
}

FORCEINLINE FRHICommandListImmediate& FRHICommandListExecutor::GetImmediateCommandList()
{
	return GRHICommandList.CommandListImmediate;
}
FORCEINLINE FRHICommandListImmediate& FRHICommandListExecutor::GetRecursiveRHICommandList()
{
	check(!GRHICommandList.CommandListImmediate.bExecuting); // queued RHI commands must not be implemented by calling other queued RHI commands
	GRHICommandList.CommandListImmediate.Flush(); // if for some reason this is called non-recursively, this flushes everything to get us in a reasonable state for the recursive, hazardous calls
	return GRHICommandList.CommandListImmediate;
}

#define DEFINE_RHIMETHOD_CMDLIST(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation)
#define DEFINE_RHIMETHOD(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation)
#define DEFINE_RHIMETHOD_GLOBAL(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation)
#define DEFINE_RHIMETHOD_GLOBALFLUSH(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE Type RHI##Name ParameterTypesAndNames \
	{ \
		ReturnStatement FRHICommandListExecutor::GetImmediateCommandList().Name ParameterNames; \
	}
#include "RHIMethods.h"
#undef DEFINE_RHIMETHOD
#undef DEFINE_RHIMETHOD_CMDLIST
#undef DEFINE_RHIMETHOD_GLOBAL
#undef DEFINE_RHIMETHOD_GLOBALFLUSH


DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Command List Execute"),STAT_RHICmdListExecuteTime,STATGROUP_RHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Command List Enqueue"),STAT_RHICmdListEnqueueTime,STATGROUP_RHI, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("RHI Command List memory"),STAT_RHICmdListMemory,STATGROUP_RHI,RHI_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("RHI Command List count"),STAT_RHICmdListCount,STATGROUP_RHI,RHI_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Counter TEMP"), STAT_RHICounterTEMP,STATGROUP_RHI,RHI_API);

#include "RHICommandList.inl"
