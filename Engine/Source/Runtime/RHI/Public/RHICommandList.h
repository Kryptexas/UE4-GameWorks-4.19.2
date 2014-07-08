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
	ERCT_EndDrawPrimitiveUP,
	ERCT_EndDrawIndexedPrimitiveUP,
	ERCT_BuildLocalBoundShaderState,
	ERCT_SetLocalBoundShaderState,
	ERCT_SetGlobalBoundShaderState,
	ERCT_SetComputeShader,
	ERCT_DispatchComputeShader,
	ERCT_DispatchIndirectComputeShader,
	ERCT_AutomaticCacheFlushAfterComputeShader,
	ERCT_FlushComputeShaderCache,
	ERCT_DrawPrimitiveIndirect,
	ERCT_DrawIndexedIndirect,
	ERCT_DrawIndexedPrimitiveIndirect,
	ERCT_EnableDepthBoundsTest,

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
	FORCEINLINE_DEBUGGABLE void SetShader(TShaderRHIParamRef InShader)
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
	FORCEINLINE_DEBUGGABLE void Set(FRHICommandList* List, uint32 InSize, const void* InData)
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
	FORCEINLINE_DEBUGGABLE void Set(FRasterizerStateRHIParamRef InState)
	{
		Type = ERCT_SetRasterizerState;
		State = InState;
	}
};

struct FRHICommandSetDepthStencilState : public FRHICommand
{
	FDepthStencilStateRHIParamRef State;
	uint32 StencilRef;
	FORCEINLINE_DEBUGGABLE void Set(FDepthStencilStateRHIParamRef InState, uint32 InStencilRef)
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
	FORCEINLINE_DEBUGGABLE void Set(TShaderRHIParamRef InShader, uint32 InBufferIndex, uint32 InBaseIndex, uint32 InNumBytes, const void* InNewValue)
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
	FORCEINLINE_DEBUGGABLE void Set(TShaderRHIParamRef InShader, uint32 InBaseIndex, FUniformBufferRHIParamRef InUniformBuffer)
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
	FORCEINLINE_DEBUGGABLE void Set(TShaderRHIParamRef InShader, uint32 InTextureIndex, FTextureRHIParamRef InTexture)
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
	FORCEINLINE_DEBUGGABLE void Set(TShaderRHIParamRef InShader, uint32 InSamplerIndex, FShaderResourceViewRHIParamRef InSRV)
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
	FORCEINLINE_DEBUGGABLE void Set(TShaderRHIParamRef InShader, uint32 InUAVIndex, FUnorderedAccessViewRHIParamRef InUAV)
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
	FORCEINLINE_DEBUGGABLE void Set(TShaderRHIParamRef InShader, uint32 InUAVIndex, FUnorderedAccessViewRHIParamRef InUAV, uint32 InInitialCount)
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
	FORCEINLINE_DEBUGGABLE void Set(TShaderRHIParamRef InShader, uint32 InSamplerIndex, FSamplerStateRHIParamRef InSampler)
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
	FORCEINLINE_DEBUGGABLE void Set(uint32 InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances)
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
	FORCEINLINE_DEBUGGABLE void Set(FIndexBufferRHIParamRef InIndexBuffer, uint32 InPrimitiveType, int32 InBaseVertexIndex, uint32 InMinIndex, uint32 InNumVertices, uint32 InStartIndex, uint32 InNumPrimitives, uint32 InNumInstances)
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
	FORCEINLINE_DEBUGGABLE void Set(FBoundShaderStateRHIParamRef InBoundShaderState)
	{
		Type = ERCT_SetBoundShaderState;
		BoundShaderState = InBoundShaderState;
	}
};

struct FRHICommandSetBlendState : public FRHICommand
{
	FBlendStateRHIParamRef State;
	FLinearColor BlendFactor;
	FORCEINLINE_DEBUGGABLE void Set(FBlendStateRHIParamRef InState, const FLinearColor& InBlendFactor)
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
	FORCEINLINE_DEBUGGABLE void Set(uint32 InStreamIndex, FVertexBufferRHIParamRef InVertexBuffer, uint32 InStride, uint32 InOffset)
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
	FORCEINLINE_DEBUGGABLE void Set(uint32 InMinX, uint32 InMinY, float InMinZ, uint32 InMaxX, uint32 InMaxY, float InMaxZ)
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
	FORCEINLINE_DEBUGGABLE void Set(bool InbEnable, uint32 InMinX, uint32 InMinY, uint32 InMaxX, uint32 InMaxY)
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

	FORCEINLINE_DEBUGGABLE void Set(
		uint32 InNewNumSimultaneousRenderTargets,
		const FRHIRenderTargetView* InNewRenderTargetsRHI,
		FTextureRHIParamRef InNewDepthStencilTargetRHI,
		uint32 InNewNumUAVs,
		const FUnorderedAccessViewRHIParamRef* InUAVs
		)

	{
		Type = ERCT_SetRenderTargets;
		check(InNewNumSimultaneousRenderTargets <= MaxSimultaneousRenderTargets && InNewNumUAVs <= MaxSimultaneousUAVs);
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

struct FRHICommandEndDrawPrimitiveUP : public FRHICommand
{
	uint32 PrimitiveType;
	uint32 NumPrimitives;
	uint32 NumVertices;
	uint32 VertexDataStride;
	void* OutVertexData;

	FORCEINLINE_DEBUGGABLE void Set(uint32 InPrimitiveType, uint32 InNumPrimitives, uint32 InNumVertices, uint32 InVertexDataStride, void* InOutVertexData)
	{
		Type = ERCT_EndDrawPrimitiveUP;
		PrimitiveType = InPrimitiveType;
		NumPrimitives = InNumPrimitives;
		NumVertices = InNumVertices;
		VertexDataStride = InVertexDataStride;
		OutVertexData = InOutVertexData;
	}
};


struct FRHICommandEndDrawIndexedPrimitiveUP : public FRHICommand
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

	FORCEINLINE_DEBUGGABLE void Set(uint32 InPrimitiveType, uint32 InNumPrimitives, uint32 InNumVertices, uint32 InVertexDataStride, void* InOutVertexData, uint32 InMinVertexIndex, uint32 InNumIndices, uint32 InIndexDataStride, void* InOutIndexData)
	{
		Type = ERCT_EndDrawIndexedPrimitiveUP;
		PrimitiveType = InPrimitiveType;
		NumPrimitives = InNumPrimitives;
		NumVertices = InNumVertices;
		VertexDataStride = InVertexDataStride;
		OutVertexData = InOutVertexData;
		MinVertexIndex = InMinVertexIndex;
		NumIndices = InNumIndices;
		IndexDataStride = InIndexDataStride;
		OutIndexData = InOutIndexData;
	}
};

struct FRHICommandSetComputeShader : public FRHICommand
{
	FComputeShaderRHIParamRef ComputeShader;
	FORCEINLINE_DEBUGGABLE void Set(FComputeShaderRHIParamRef InComputeShader)
	{
		Type = ERCT_SetComputeShader;
		ComputeShader = InComputeShader;
	}
};

struct FRHICommandDispatchComputeShader : public FRHICommand
{
	uint32 ThreadGroupCountX;
	uint32 ThreadGroupCountY;
	uint32 ThreadGroupCountZ;
	FORCEINLINE_DEBUGGABLE void Set(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
	{
		Type = ERCT_DispatchComputeShader;
		ThreadGroupCountX = InThreadGroupCountX;
		ThreadGroupCountY = InThreadGroupCountY;
		ThreadGroupCountZ = InThreadGroupCountZ;
	}
};

struct FRHICommandDispatchIndirectComputeShader : public FRHICommand
{
	FVertexBufferRHIParamRef ArgumentBuffer;
	uint32 ArgumentOffset;
	FORCEINLINE_DEBUGGABLE void Set(FVertexBufferRHIParamRef InArgumentBuffer, uint32 InArgumentOffset)
	{
		Type = ERCT_DispatchIndirectComputeShader;
		ArgumentBuffer = InArgumentBuffer;
		ArgumentOffset = InArgumentOffset;

	}
};

struct FRHICommandAutomaticCacheFlushAfterComputeShader : public FRHICommand
{
	bool bEnable;
	FORCEINLINE_DEBUGGABLE void Set(bool InbEnable)
	{
		Type = ERCT_AutomaticCacheFlushAfterComputeShader;
		bEnable = InbEnable;
	}
};

struct FRHICommandFlushComputeShaderCache : public FRHICommand
{
	FORCEINLINE_DEBUGGABLE void Set()
	{
		Type = ERCT_FlushComputeShaderCache;
	}
};

struct FRHICommandDrawPrimitiveIndirect : public FRHICommand
{
	uint32 PrimitiveType;
	FVertexBufferRHIParamRef ArgumentBuffer;
	uint32 ArgumentOffset;
	FORCEINLINE_DEBUGGABLE void Set(uint32 InPrimitiveType, FVertexBufferRHIParamRef InArgumentBuffer, uint32 InArgumentOffset)
	{
		Type = ERCT_DrawPrimitiveIndirect;
		PrimitiveType = InPrimitiveType;
		ArgumentBuffer = InArgumentBuffer;
		ArgumentOffset = InArgumentOffset;

	}
};

struct FRHICommandDrawIndexedIndirect : public FRHICommand
{
	FIndexBufferRHIParamRef IndexBufferRHI;
	uint32 PrimitiveType;
	FStructuredBufferRHIParamRef ArgumentsBufferRHI;
	uint32 DrawArgumentsIndex;
	uint32 NumInstances;

	FORCEINLINE_DEBUGGABLE void Set(FIndexBufferRHIParamRef InIndexBufferRHI, uint32 InPrimitiveType, FStructuredBufferRHIParamRef InArgumentsBufferRHI, uint32 InDrawArgumentsIndex, uint32 InNumInstances)
	{
		Type = ERCT_DrawIndexedIndirect;
		IndexBufferRHI = InIndexBufferRHI;
		PrimitiveType = InPrimitiveType;
		ArgumentsBufferRHI = InArgumentsBufferRHI;
		DrawArgumentsIndex = InDrawArgumentsIndex;
		NumInstances = InNumInstances;
	}
};

struct FRHICommandDrawIndexedPrimitiveIndirect : public FRHICommand
{
	uint32 PrimitiveType;
	FIndexBufferRHIParamRef IndexBuffer;
	FVertexBufferRHIParamRef ArgumentsBuffer;
	uint32 ArgumentOffset;

	FORCEINLINE_DEBUGGABLE void Set(uint32 InPrimitiveType, FIndexBufferRHIParamRef InIndexBuffer, FVertexBufferRHIParamRef InArgumentsBuffer, uint32 InArgumentOffset)
	{
		Type = ERCT_DrawIndexedPrimitiveIndirect;
		PrimitiveType = InPrimitiveType;
		IndexBuffer = InIndexBuffer;
		ArgumentsBuffer = InArgumentsBuffer;
		ArgumentOffset = InArgumentOffset;
	}
};

struct FRHICommandEnableDepthBoundsTest : public FRHICommand
{
	bool bEnable;
	float MinDepth;
	float MaxDepth;

	FORCEINLINE_DEBUGGABLE void Set(bool InbEnable, float InMinDepth, float InMaxDepth)
	{
		Type = ERCT_EnableDepthBoundsTest;
		bEnable = InbEnable;
		MinDepth = InMinDepth;
		MaxDepth = InMaxDepth;
	}
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


struct FLocalBoundShaderStateWorkArea
{
	FBoundShaderStateInput Args;
	FBoundShaderStateRHIRef BSS;
	int32 UseCount;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	class FRHICommandList* CheckCmdList;
	int32 UID;
#endif
};

struct FLocalBoundShaderState
{
	FLocalBoundShaderStateWorkArea* WorkArea;
	FBoundShaderStateRHIRef BypassBSS; // this is only used in the case of Bypass, should eventually be deleted
	FLocalBoundShaderState()
		: WorkArea(nullptr)
	{
	}
};

struct FRHICommandBuildLocalBoundShaderState : public FRHICommand
{
	FLocalBoundShaderStateWorkArea WorkArea;
	FORCEINLINE_DEBUGGABLE void Set(
		class FRHICommandList* CheckCmdList,
		int32 UID,
		FVertexDeclarationRHIParamRef VertexDeclarationRHI,
		FVertexShaderRHIParamRef VertexShaderRHI,
		FHullShaderRHIParamRef HullShaderRHI,
		FDomainShaderRHIParamRef DomainShaderRHI,
		FPixelShaderRHIParamRef PixelShaderRHI,
		FGeometryShaderRHIParamRef GeometryShaderRHI
		)
	{
		Type = ERCT_BuildLocalBoundShaderState;
		WorkArea.Args.VertexDeclarationRHI = VertexDeclarationRHI;
		WorkArea.Args.VertexShaderRHI = VertexShaderRHI;
		WorkArea.Args.HullShaderRHI = HullShaderRHI;
		WorkArea.Args.DomainShaderRHI = DomainShaderRHI;
		WorkArea.Args.PixelShaderRHI = PixelShaderRHI;
		WorkArea.Args.GeometryShaderRHI = GeometryShaderRHI;

		// these RHI commands are bad, no constructor! we construct this in place
		new ((void*)&WorkArea.BSS) FBoundShaderStateRHIRef();
		WorkArea.UseCount = 0;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		WorkArea.CheckCmdList = CheckCmdList;
		WorkArea.UID = UID;
#endif
	}
};

struct FRHICommandSetLocalBoundShaderState : public FRHICommand
{
	FLocalBoundShaderState LocalBoundShaderState;
	FORCEINLINE_DEBUGGABLE void Set(class FRHICommandList* CheckCmdList, int32 UID, FLocalBoundShaderState& InLocalBoundShaderState)
	{
		Type = ERCT_SetLocalBoundShaderState;
		LocalBoundShaderState.WorkArea = InLocalBoundShaderState.WorkArea;
		check(CheckCmdList == LocalBoundShaderState.WorkArea->CheckCmdList && UID == LocalBoundShaderState.WorkArea->UID); // this BSS was not built for this particular commandlist
		LocalBoundShaderState.WorkArea->UseCount++;
	}
};


class FShader;

struct FGlobalBoundShaderStateArgs
{
	FVertexDeclarationRHIParamRef VertexDeclarationRHI;
	FShader* VertexShader;
	FShader* PixelShader;
	FShader* GeometryShader;
};

class FGlobalBoundShaderStateResource;
template<class ResourceType> class TGlobalResource;


struct FGlobalBoundShaderStateWorkArea
{
	FGlobalBoundShaderStateArgs Args;
	TGlobalResource<FGlobalBoundShaderStateResource>* BSS; //ideally this would be part of this memory block and not a separate allocation...that is doable, if a little tedious. The point is we need to delay the construction until we get back to the render thread.

	FGlobalBoundShaderStateWorkArea()
		: BSS(nullptr)
	{
	}
};

typedef FGlobalBoundShaderStateWorkArea* FGlobalBoundShaderState;

struct FRHICommandSetGlobalBoundShaderState : public FRHICommand
{
	FGlobalBoundShaderState GlobalBoundShaderState;
	FORCEINLINE_DEBUGGABLE void Set(FGlobalBoundShaderState InGlobalBoundShaderState)
	{
		Type = ERCT_SetGlobalBoundShaderState;
		GlobalBoundShaderState = InGlobalBoundShaderState;
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

			FORCEINLINE_DEBUGGABLE SIZE_T MemUsed() const
			{
				return Current - Head;
			}

			FORCEINLINE_DEBUGGABLE SIZE_T MemAvailable() const
			{
				return Tail - Current;
			}

			FORCEINLINE_DEBUGGABLE bool CanFitAllocation(SIZE_T AlignedSize) const
			{
				return (AlignedSize + Current <= Tail);
			}

			FORCEINLINE_DEBUGGABLE uint8* Alloc(SIZE_T AlignedSize)
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

		FORCEINLINE_DEBUGGABLE uint8* Alloc(SIZE_T InSize)
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

	FDrawUpData DrawUPData;

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
			auto* Cmd = AddCommand<FRHICommandBuildLocalBoundShaderState>();
			Cmd->Set(this, this->UID, VertexDeclarationRHI, VertexShaderRHI, HullShaderRHI, DomainShaderRHI, PixelShaderRHI, GeometryShaderRHI);
			Result.WorkArea = &Cmd->WorkArea;

			if (VertexDeclarationRHI)
			{
				VertexDeclarationRHI->AddRef();
			}
			if (VertexShaderRHI)
			{
				VertexShaderRHI->AddRef();
			}
			if (HullShaderRHI)
			{
				HullShaderRHI->AddRef();
			}
			if (DomainShaderRHI)
			{
				DomainShaderRHI->AddRef();
			}
			if (PixelShaderRHI)
			{
				PixelShaderRHI->AddRef();
			}
			if (GeometryShaderRHI)
			{
				GeometryShaderRHI->AddRef();
			}
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
		auto* Cmd = AddCommand<FRHICommandSetLocalBoundShaderState>();
		Cmd->Set(this, this->UID, LocalBoundShaderState);
	}

	FORCEINLINE_DEBUGGABLE void BuildAndSetGlobalBoundShaderState(
		void(*InSetGlobalBoundShaderState_InternalPtr)(FGlobalBoundShaderState BoundShaderState),
		FGlobalBoundShaderState& GlobalBoundShaderState,
		FVertexDeclarationRHIParamRef VertexDeclarationRHI,
		FShader* VertexShader,
		FShader* PixelShader,
		FShader* GeometryShader
		)
	{
		SetGlobalBoundShaderState_InternalPtr = InSetGlobalBoundShaderState_InternalPtr; // total hack to work around module issues
		if (!GlobalBoundShaderState)
		{
			FGlobalBoundShaderStateWorkArea* NewGlobalBoundShaderState = new FGlobalBoundShaderStateWorkArea();
			NewGlobalBoundShaderState->Args.VertexDeclarationRHI = VertexDeclarationRHI;
			NewGlobalBoundShaderState->Args.VertexShader = VertexShader;
			NewGlobalBoundShaderState->Args.PixelShader = PixelShader;
			NewGlobalBoundShaderState->Args.GeometryShader = GeometryShader;
			FPlatformMisc::MemoryBarrier();

			FGlobalBoundShaderStateWorkArea* OldGlobalBoundShaderState = (FGlobalBoundShaderStateWorkArea*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)&GlobalBoundShaderState, NewGlobalBoundShaderState, nullptr);

			if (OldGlobalBoundShaderState != nullptr)
			{
				//we lost
				delete NewGlobalBoundShaderState;
				check(OldGlobalBoundShaderState == GlobalBoundShaderState);
			}
			else
			{
				// we won

				// this is not even slightly threadsafe, a "loser" might destroy the resource before we get here, and besides this is global; we are never going to destory these anyway.

				if (VertexDeclarationRHI)
				{
					VertexDeclarationRHI->AddRef();
				}
			}
		}
		check(
			VertexDeclarationRHI == GlobalBoundShaderState->Args.VertexDeclarationRHI &&
			VertexShader == GlobalBoundShaderState->Args.VertexShader &&
			PixelShader == GlobalBoundShaderState->Args.PixelShader &&
			GeometryShader == GlobalBoundShaderState->Args.GeometryShader
			);
		if (Bypass())
		{
			InSetGlobalBoundShaderState_InternalPtr(GlobalBoundShaderState);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetGlobalBoundShaderState>();
		Cmd->Set(GlobalBoundShaderState);
	}

	template <typename TShaderRHIParamRef>
	FORCEINLINE_DEBUGGABLE void SetShaderUniformBuffer(TShaderRHIParamRef Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
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
	FORCEINLINE_DEBUGGABLE void SetShaderParameter(TShaderRHIParamRef Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue, bool bValueInStack = false)
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
	FORCEINLINE_DEBUGGABLE void SetShaderTexture(TShaderRHIParamRef Shader, uint32 TextureIndex, FTextureRHIParamRef Texture)
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
	FORCEINLINE_DEBUGGABLE void SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV)
	{
		if (Bypass())
		{
			SetShaderResourceViewParameter_Internal(Shader, SamplerIndex, SRV);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetShaderResourceViewParameter>();
		Cmd->Set<TShaderRHIParamRef>(Shader, SamplerIndex, SRV);
		Shader->AddRef();
		if (SRV)
		{
			SRV->AddRef();
		}
	}

	template <typename TShaderRHIParamRef>
	FORCEINLINE_DEBUGGABLE void SetShaderSampler(TShaderRHIParamRef Shader, uint32 SamplerIndex, FSamplerStateRHIParamRef State)
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

	FORCEINLINE_DEBUGGABLE void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
	{
		if (Bypass())
		{
			SetUAVParameter_Internal(Shader, UAVIndex, UAV);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetUAVParameter>();
		Cmd->Set<FComputeShaderRHIParamRef>(Shader, UAVIndex, UAV);
		Shader->AddRef();
		if (UAV)
		{
			UAV->AddRef();
		}
	}

	FORCEINLINE_DEBUGGABLE void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount)
	{
		if (Bypass())
		{
			SetUAVParameter_Internal(Shader, UAVIndex, UAV, InitialCount);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetUAVParameter_IntialCount>();
		Cmd->Set<FComputeShaderRHIParamRef>(Shader, UAVIndex, UAV, InitialCount);
		Shader->AddRef();
		if (UAV)
		{
			UAV->AddRef();
		}
	}

	FORCEINLINE_DEBUGGABLE void SetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState)
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

	FORCEINLINE_DEBUGGABLE void SetRasterizerState(FRasterizerStateRHIParamRef State)
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

	FORCEINLINE_DEBUGGABLE void SetBlendState(FBlendStateRHIParamRef State, const FLinearColor& BlendFactor = FLinearColor::White)
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

	FORCEINLINE_DEBUGGABLE void DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		if (Bypass())
		{
			DrawPrimitive_Internal(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
			return;
		}

		auto* Cmd = AddCommand<FRHICommandDrawPrimitive>();
		Cmd->Set(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
	}

	FORCEINLINE_DEBUGGABLE void DrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 MinIndex, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
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

	FORCEINLINE_DEBUGGABLE void SetStreamSource(uint32 StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint32 Offset)
	{
		if (Bypass())
		{
			SetStreamSource_Internal(StreamIndex, VertexBuffer, Stride, Offset);
			return;
		}

		auto* Cmd = AddCommand<FRHICommandSetStreamSource>();
		Cmd->Set(StreamIndex, VertexBuffer, Stride, Offset);
		if (VertexBuffer)
		{
			VertexBuffer->AddRef();
		}
	}

	FORCEINLINE_DEBUGGABLE void SetDepthStencilState(FDepthStencilStateRHIParamRef NewStateRHI, uint32 StencilRef = 0)
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

	FORCEINLINE_DEBUGGABLE void SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ)
	{
		if (Bypass())
		{
			SetViewport_Internal(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetViewport>();
		Cmd->Set(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
	}

	FORCEINLINE_DEBUGGABLE void SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
	{
		if (Bypass())
		{
			SetScissorRect_Internal(bEnable, MinX, MinY, MaxX, MaxY);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetScissorRect>();
		Cmd->Set(bEnable,  MinX, MinY, MaxX, MaxY);
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
		auto* Cmd = AddCommand<FRHICommandSetRenderTargets>();
		Cmd->Set(
			NewNumSimultaneousRenderTargets,
			NewRenderTargetsRHI,
			NewDepthStencilTargetRHI,
			NewNumUAVs,
			UAVs);
		for (uint32 Index = 0; Index < NewNumSimultaneousRenderTargets; Index++)
		{
			if (NewRenderTargetsRHI[Index].Texture)
			{
				NewRenderTargetsRHI[Index].Texture->AddRef();
			}
		}
		if (NewDepthStencilTargetRHI)
		{
			NewDepthStencilTargetRHI->AddRef();
		}
		for (uint32 Index = 0; Index < NewNumUAVs; Index++)
		{
			if (UAVs[Index])
			{
				UAVs[Index]->AddRef();
			}
		}

	}

	void BeginDrawPrimitiveUP(uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData)
	{
		if (Bypass())
		{
			BeginDrawPrimitiveUP_Internal(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, OutVertexData);
			return;
		}
		check(!DrawUPData.OutVertexData && NumVertices * VertexDataStride > 0);

		OutVertexData = FMemory::Malloc(NumVertices * VertexDataStride);

		DrawUPData.PrimitiveType = PrimitiveType;
		DrawUPData.NumPrimitives = NumPrimitives;
		DrawUPData.NumVertices = NumVertices;
		DrawUPData.VertexDataStride = VertexDataStride;
		DrawUPData.OutVertexData = OutVertexData;
	}

	void EndDrawPrimitiveUP()
	{
		if (Bypass())
		{
			EndDrawPrimitiveUP_Internal();
			return;
		}
		check(DrawUPData.OutVertexData && DrawUPData.NumVertices);
		auto* Cmd = AddCommand<FRHICommandEndDrawPrimitiveUP>();
		Cmd->Set(DrawUPData.PrimitiveType, DrawUPData.NumPrimitives, DrawUPData.NumVertices, DrawUPData.VertexDataStride, DrawUPData.OutVertexData);

		DrawUPData.OutVertexData = nullptr;
		DrawUPData.NumVertices = 0;
	}

	void BeginDrawIndexedPrimitiveUP(uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData, uint32 MinVertexIndex, uint32 NumIndices, uint32 IndexDataStride, void*& OutIndexData)
	{
		if (Bypass())
		{
			BeginDrawIndexedPrimitiveUP_Internal(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, OutVertexData, MinVertexIndex, NumIndices, IndexDataStride, OutIndexData);
			return;
		}
		check(!DrawUPData.OutVertexData && !DrawUPData.OutIndexData && NumVertices * VertexDataStride > 0 && NumIndices * IndexDataStride > 0);

		OutVertexData = FMemory::Malloc(NumVertices * VertexDataStride);
		OutIndexData = FMemory::Malloc(NumIndices * IndexDataStride);

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
	void EndDrawIndexedPrimitiveUP()
	{
		if (Bypass())
		{
			EndDrawIndexedPrimitiveUP_Internal();
			return;
		}
		check(DrawUPData.OutVertexData && DrawUPData.OutIndexData && DrawUPData.NumIndices && DrawUPData.NumVertices);

		auto* Cmd = AddCommand<FRHICommandEndDrawIndexedPrimitiveUP>();
		Cmd->Set(DrawUPData.PrimitiveType, DrawUPData.NumPrimitives, DrawUPData.NumVertices, DrawUPData.VertexDataStride, DrawUPData.OutVertexData, DrawUPData.MinVertexIndex, DrawUPData.NumIndices, DrawUPData.IndexDataStride, DrawUPData.OutIndexData);

		DrawUPData.OutVertexData = nullptr;
		DrawUPData.OutIndexData = nullptr;
		DrawUPData.NumIndices = 0;
		DrawUPData.NumVertices = 0;
	}

	void SetComputeShader(FComputeShaderRHIParamRef ComputeShader)
	{
		if (Bypass())
		{
			SetComputeShader_Internal(ComputeShader);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandSetComputeShader>();
		Cmd->Set(ComputeShader);
		ComputeShader->AddRef();
	}

	void DispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
		if (Bypass())
		{
			DispatchComputeShader_Internal(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandDispatchComputeShader>();
		Cmd->Set(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	void DispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer, uint32 ArgumentOffset)
	{
		if (Bypass())
		{
			DispatchIndirectComputeShader_Internal(ArgumentBuffer, ArgumentOffset);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandDispatchIndirectComputeShader>();
		Cmd->Set(ArgumentBuffer, ArgumentOffset);
		ArgumentBuffer->AddRef();
	}

	void AutomaticCacheFlushAfterComputeShader(bool bEnable)
	{
		if (Bypass())
		{
			AutomaticCacheFlushAfterComputeShader_Internal(bEnable);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandAutomaticCacheFlushAfterComputeShader>();
		Cmd->Set(bEnable);
	}

	void FlushComputeShaderCache()
	{
		if (Bypass())
		{
			FlushComputeShaderCache_Internal();
			return;
		}
		auto* Cmd = AddCommand<FRHICommandFlushComputeShaderCache>();
		Cmd->Set();
	}

	void DrawPrimitiveIndirect(uint32 PrimitiveType, FVertexBufferRHIParamRef ArgumentBuffer, uint32 ArgumentOffset)
	{
		if (Bypass())
		{
			DrawPrimitiveIndirect_Internal(PrimitiveType, ArgumentBuffer, ArgumentOffset);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandDrawPrimitiveIndirect>();
		Cmd->Set(PrimitiveType, ArgumentBuffer, ArgumentOffset);
		ArgumentBuffer->AddRef();
	}

	void DrawIndexedIndirect(FIndexBufferRHIParamRef IndexBufferRHI, uint32 PrimitiveType, FStructuredBufferRHIParamRef ArgumentsBufferRHI, uint32 DrawArgumentsIndex, uint32 NumInstances)
	{
		if (Bypass())
		{
			DrawIndexedIndirect_Internal(IndexBufferRHI, PrimitiveType, ArgumentsBufferRHI, DrawArgumentsIndex, NumInstances);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandDrawIndexedIndirect>();
		Cmd->Set(IndexBufferRHI, PrimitiveType, ArgumentsBufferRHI, DrawArgumentsIndex, NumInstances);
		IndexBufferRHI->AddRef();
		ArgumentsBufferRHI->AddRef();
	}

	void DrawIndexedPrimitiveIndirect(uint32 PrimitiveType, FIndexBufferRHIParamRef IndexBuffer, FVertexBufferRHIParamRef ArgumentsBuffer, uint32 ArgumentOffset)
	{
		if (Bypass())
		{
			DrawIndexedPrimitiveIndirect_Internal(PrimitiveType, IndexBuffer, ArgumentsBuffer, ArgumentOffset);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandDrawIndexedPrimitiveIndirect>();
		Cmd->Set(PrimitiveType, IndexBuffer, ArgumentsBuffer, ArgumentOffset);
		IndexBuffer->AddRef();
		ArgumentsBuffer->AddRef();
	}

	void EnableDepthBoundsTest(bool bEnable, float MinDepth, float MaxDepth)
	{
		if (Bypass())
		{
			EnableDepthBoundsTest_Internal(bEnable, MinDepth, MaxDepth);
			return;
		}
		auto* Cmd = AddCommand<FRHICommandEnableDepthBoundsTest>();
		Cmd->Set(bEnable, MinDepth, MaxDepth);

	}



	const SIZE_T GetUsedMemory() const;

private:
	bool bExecuting;
	uint32 NumCommands;
	uint32 UID;
	static FThreadSafeCounter UIDCounter;
	void(*SetGlobalBoundShaderState_InternalPtr)(FGlobalBoundShaderState BoundShaderState);


	FORCEINLINE_DEBUGGABLE uint8* Alloc(SIZE_T InSize)
	{
		return MemManager.Alloc(InSize);
	}

	FORCEINLINE_DEBUGGABLE bool HasCommands() const
	{
		return (NumCommands > 0);
	}

	template <typename TCmd>
	FORCEINLINE_DEBUGGABLE TCmd* AddCommand(uint32 ExtraData = 0)
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
		UID = UIDCounter.Increment();
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
		FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
		{ \
			Flush(); \
			ReturnStatement Name##_Internal ParameterNames; \
		}
	#define DEFINE_RHIMETHOD_GLOBAL(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
		{ \
			ReturnStatement RHI##Name ParameterNames; \
		}
	#define DEFINE_RHIMETHOD_GLOBALFLUSH(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
		FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
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
	FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
	{ \
		ReturnStatement Name##_Internal ParameterNames; \
	}
#define DEFINE_RHIMETHOD(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
	{ \
		ReturnStatement Name##_Internal ParameterNames; \
	}
#define DEFINE_RHIMETHOD_GLOBAL(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
	{ \
		ReturnStatement RHI##Name ParameterNames; \
	}
#define DEFINE_RHIMETHOD_GLOBALFLUSH(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation) \
	FORCEINLINE_DEBUGGABLE Type Name ParameterTypesAndNames \
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
	void LatchBypass();

	FORCEINLINE_DEBUGGABLE void Verify()
	{
		check(CommandListImmediate.bExecuting || !CommandListImmediate.HasCommands());
	}
	FORCEINLINE_DEBUGGABLE bool Bypass()
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

FORCEINLINE_DEBUGGABLE bool FRHICommandList::Bypass()
{
	return GRHICommandList.Bypass();
}

FORCEINLINE_DEBUGGABLE FRHICommandListImmediate& FRHICommandListExecutor::GetImmediateCommandList()
{
	return GRHICommandList.CommandListImmediate;
}
FORCEINLINE_DEBUGGABLE FRHICommandListImmediate& FRHICommandListExecutor::GetRecursiveRHICommandList()
{
	check(!GRHICommandList.CommandListImmediate.bExecuting); // queued RHI commands must not be implemented by calling other queued RHI commands
	GRHICommandList.CommandListImmediate.Flush(); // if for some reason this is called non-recursively, this flushes everything to get us in a reasonable state for the recursive, hazardous calls
	return GRHICommandList.CommandListImmediate;
}

#define DEFINE_RHIMETHOD_CMDLIST(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation)
#define DEFINE_RHIMETHOD(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation)
#define DEFINE_RHIMETHOD_GLOBAL(Type, Name, ParameterTypesAndNames, ParameterNames, ReturnStatement, NullImplementation)
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


DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Command List Execute"),STAT_RHICmdListExecuteTime,STATGROUP_RHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Command List Enqueue"),STAT_RHICmdListEnqueueTime,STATGROUP_RHI, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("RHI Command List memory"),STAT_RHICmdListMemory,STATGROUP_RHI,RHI_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("RHI Command List count"),STAT_RHICmdListCount,STATGROUP_RHI,RHI_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Counter TEMP"), STAT_RHICounterTEMP,STATGROUP_RHI,RHI_API);

#include "RHICommandList.inl"
