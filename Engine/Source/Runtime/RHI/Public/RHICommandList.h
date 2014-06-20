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

class RHI_API FRHICommandList
{
private:
	static FRHICommandList NullRHICommandList;

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

		FORCEINLINE bool IsNull() const
		{
			return (this == &NullRHICommandList.MemManager);
		}

		FORCEINLINE uint8* Alloc(SIZE_T InSize)
		{
			checkSlow(!IsNull());
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

	static FORCEINLINE FRHICommandList& GetNullRef()
	{
		return NullRHICommandList;
	}

	FRHICommandList()
	{
		Reset();
	}

	template <typename TShaderRHIParamRef>
	inline void SetShaderUniformBuffer(TShaderRHIParamRef Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBufferRHI);

	template <typename TShaderRHIParamRef>
	inline void SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV);

	template <typename TShaderRHIParamRef>
	inline void SetShaderParameter(TShaderRHIParamRef Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* Value, bool bValueInStack = false);

	template <typename TShaderRHIParamRef>
	inline void SetShaderTexture(TShaderRHIParamRef Shader, uint32 TextureIndex, FTextureRHIParamRef Texture);

	template <typename TShaderRHIParamRef>
	inline void SetShaderSampler(TShaderRHIParamRef Shader, uint32 SamplerIndex, FSamplerStateRHIParamRef State);

	inline void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV);
	inline void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32 UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32 InitialCount);

	inline void SetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState);

	inline void SetRasterizerState(FRasterizerStateRHIParamRef State);

	inline void SetBlendState(FBlendStateRHIParamRef State, const FLinearColor& BlendFactor = FLinearColor::White);

	inline void DrawPrimitive(uint32 PrimitiveType, uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances);
	inline void DrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32 PrimitiveType, int32 BaseVertexIndex, uint32 MinIndex, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances);
	inline void SetStreamSource(uint32 StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint32 Offset);

	// need cmd list implementations

	FORCEINLINE void SetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ)
	{
		CheckIsNull();
		RHISetViewport(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
	}

	FORCEINLINE void SetDepthStencilState(FDepthStencilStateRHIParamRef NewStateRHI, uint32 StencilRef = 0)
	{
		CheckIsNull();
		RHISetDepthStencilState(NewStateRHI, StencilRef);
	}

	FORCEINLINE void BeginDrawPrimitiveUP(uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData)
	{
		CheckIsNull();
		RHIBeginDrawPrimitiveUP(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, OutVertexData);
	}
	
	FORCEINLINE void EndDrawPrimitiveUP()
	{
		CheckIsNull();
		RHIEndDrawPrimitiveUP();
	}

	FORCEINLINE void CopyToResolveTarget(FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI, bool bKeepOriginalSurface, const FResolveParams& ResolveParams)
	{
		CheckIsNull();
		RHICopyToResolveTarget(SourceTextureRHI, DestTextureRHI, bKeepOriginalSurface, ResolveParams);
	}

	FORCEINLINE void BeginDrawIndexedPrimitiveUP(uint32 PrimitiveType, uint32 NumPrimitives, uint32 NumVertices, uint32 VertexDataStride, void*& OutVertexData, uint32 MinVertexIndex, uint32 NumIndices, uint32 IndexDataStride, void*& OutIndexData)
	{
		CheckIsNull();
		RHIBeginDrawIndexedPrimitiveUP(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, OutVertexData, MinVertexIndex, NumIndices, IndexDataStride, OutIndexData);
	}

	FORCEINLINE void EndDrawIndexedPrimitiveUP()
	{
		CheckIsNull();
		RHIEndDrawIndexedPrimitiveUP();
	}

	FORCEINLINE void Clear(bool bClearColor, const FLinearColor& Color, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntRect ExcludeRect)
	{
		CheckIsNull();
		RHIClear(bClearColor, Color, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
	}

	FORCEINLINE void ClearMRT(bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntRect ExcludeRect)
	{
		CheckIsNull();
		RHIClearMRT(bClearColor, NumClearColors, ClearColorArray, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
	}

	FORCEINLINE void SetRenderTargets(uint32 NewNumSimultaneousRenderTargets, const FRHIRenderTargetView* NewRenderTargetsRHI, FTextureRHIParamRef NewDepthStencilTargetRHI, uint32 NewNumUAVs, const FUnorderedAccessViewRHIParamRef* UAVs)
	{
		CheckIsNull();
		RHISetRenderTargets(NewNumSimultaneousRenderTargets, NewRenderTargetsRHI, NewDepthStencilTargetRHI, NewNumUAVs, UAVs);
	}

	FORCEINLINE void EnableDepthBoundsTest(bool bEnable, float MinDepth, float MaxDepth)
	{
		CheckIsNull();
		RHIEnableDepthBoundsTest(bEnable, MinDepth, MaxDepth);
	}

	FORCEINLINE void SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
	{
		CheckIsNull();
		RHISetScissorRect(bEnable, MinX, MinY, MaxX, MaxY);
	}

	FORCEINLINE void ClearUAV(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const uint32* Values)
	{
		CheckIsNull();
		RHIClearUAV(UnorderedAccessViewRHI, Values);
	}

	FORCEINLINE void SetComputeShader(FComputeShaderRHIParamRef ComputeShaderRHI)
	{
		CheckIsNull();
		RHISetComputeShader(ComputeShaderRHI);
	}

	FORCEINLINE void DrawPrimitiveIndirect(uint32 PrimitiveType, FVertexBufferRHIParamRef ArgumentBufferRHI, uint32 ArgumentOffset)
	{
		CheckIsNull();
		RHIDrawPrimitiveIndirect(PrimitiveType, ArgumentBufferRHI, ArgumentOffset);
	}

	FORCEINLINE bool IsNull() const
	{
		return MemManager.IsNull();
	}

	FORCEINLINE void CheckIsNull() const
	{
		check(IsNull());
	}

	const SIZE_T GetUsedMemory() const;

protected:
	enum EState
	{
		Ready,
		Executing,
		Kicked,
	};
	EState State;
	uint32 NumCommands;

	FORCEINLINE uint8* Alloc(SIZE_T InSize)
	{
		return MemManager.Alloc(InSize);
	}

	FORCEINLINE bool CanAddCommand() const
	{
		return (State == Ready);
	}

	template <typename TCmd>
	FORCEINLINE TCmd* AddCommand(uint32 ExtraData = 0)
	{
		checkSlow(!IsNull());
		check(CanAddCommand());
		TCmd* Cmd = (TCmd*)Alloc(sizeof(TCmd) + ExtraData);
		//::OutputDebugStringW(*FString::Printf(TEXT("Add %d: @ 0x%p, %d bytes\n"), NumCommands, (void*)Cmd, (sizeof(TCmd) + ExtraData)));
		++NumCommands;
		return Cmd;
	}

	void Reset()
	{
		State = Kicked;
		NumCommands = 0;
		MemManager.Reset();
	}

	FORCEINLINE bool Bypass()
	{
		return IsNull();
	}

	friend class FRHICommandListExecutor;
	friend class FRHICommandListIterator;

	// Used here as we really want Alloc() to only work for this class, nobody else should use it
	friend struct FRHICommandNopBlob;
};

class RHI_API FRHICommandListExecutor
{
public:
	FRHICommandListExecutor();
	FRHICommandList& CreateList();
	void ExecuteAndFreeList(FRHICommandList& CmdList);

	void Verify();

private:
	FCriticalSection CriticalSection;
	bool bLock;

	template <bool bOnlyTestMemAccess>
	void ExecuteList(FRHICommandList& CmdList);
};
extern RHI_API FRHICommandListExecutor GRHICommandList;

DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Command List Execute"),STAT_RHICmdListExecuteTime,STATGROUP_RHI, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Command List Enqueue"),STAT_RHICmdListEnqueueTime,STATGROUP_RHI, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("RHI Command List memory"),STAT_RHICmdListMemory,STATGROUP_RHI,RHI_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("RHI Command List count"),STAT_RHICmdListCount,STATGROUP_RHI,RHI_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("RHI Counter TEMP"), STAT_RHICounterTEMP,STATGROUP_RHI,RHI_API);

#include "RHICommandList.inl"
