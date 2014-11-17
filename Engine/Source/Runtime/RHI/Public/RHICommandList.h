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
	inline void SetShaderUniformBuffer(TShaderRHIParamRef Shader, uint32 BaseIndex, FUniformBufferRHIParamRef UniformBufferRHI);

	template <typename TShaderRHIParamRef>
	inline void SetShaderParameter(TShaderRHIParamRef Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* Value, bool bValueInStack = false);

	template <typename TShaderRHIParamRef>
	inline void SetShaderTexture(TShaderRHIParamRef Shader, uint32 TextureIndex, FTextureRHIParamRef Texture);

	template <typename TShaderRHIParamRef>
	inline void SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32 SamplerIndex, FShaderResourceViewRHIParamRef SRV);

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
	inline void SetDepthStencilState(FDepthStencilStateRHIParamRef NewStateRHI, uint32 StencilRef = 0);

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
