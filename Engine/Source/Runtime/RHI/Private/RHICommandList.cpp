// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHI.cpp: Render Hardware Interface implementation.
=============================================================================*/

#include "RHI.h"
#include "RHICommandList.h"

DEFINE_STAT(STAT_RHICmdListExecuteTime);
DEFINE_STAT(STAT_RHICmdListEnqueueTime);
DEFINE_STAT(STAT_RHICmdListMemory);
DEFINE_STAT(STAT_RHICmdListCount);
DEFINE_STAT(STAT_RHICounterTEMP);

static int32 GRHICmd = 0;
static FAutoConsoleVariableRef CVarRHICmd(
	TEXT("r.RHICmd"),
	GRHICmd,
	TEXT("0: Disables the RHI Command List mode, running regular RHI calls (default)\n")
	TEXT("1: Enables the RHI Command List mode")
	);

static int32 GRHISkip = 0;
static FAutoConsoleVariableRef CVarRHICmdSkip(
	TEXT("r.RHISkip"),
	GRHISkip,
	TEXT("0: No effect (default)\n")
	TEXT("1: Skips executing RHI Command List")
	);

static int32 GRHICmdExec = 0;
static FAutoConsoleVariableRef CVarRHICmdExec(
	TEXT("r.RHIExec"),
	GRHICmdExec,
	TEXT("0: Executes RHI Command List using RHI calls (default)\n")
	TEXT("1: Executes RHI Command List using platform specific APIs")
	);

static int32 GRHICmdMem = 0;
static FAutoConsoleVariableRef CVarRHICmdMem(
	TEXT("r.RHIMem"),
	GRHICmdMem,
	TEXT("0: No effect(default)\n")
	TEXT("1: Processes the RHI Command List but doesn't execute commands")
	);

RHI_API FRHICommandListExecutor GRHICommandList;
RHI_API FRHICommandList FRHICommandList::NullRHICommandList;

static_assert(sizeof(FRHICommand) == sizeof(FRHICommandNopEndOfPage), "These should match for filling in the end of a page when a new allocation won't fit");

//@todo-rco: Temp hack!
static FRHICommandList GCommandList;


FRHICommandListExecutor::FRHICommandListExecutor() :
	bLock(false)
{
}

template <bool bOnlyTestMemAccess>
void FRHICommandListExecutor::ExecuteList(FRHICommandList& CmdList)
{
	FRHICommandListIterator Iter(CmdList);
	while (Iter.HasCommandsLeft())
	{
		auto* Cmd = *Iter;
		switch (Cmd->Type)
		{
		case ERCT_NopBlob:
			{
				// Nop
				auto* RHICmd = Iter.NextCommand<FRHICommandNopBlob>();
				(void)RHICmd;	// Unused
			}
			break;
		case ERCT_NopEndOfPage:
			{
				// Nop
				auto* RHICmd = Iter.NextCommand<FRHICommandNopEndOfPage>();
				(void)RHICmd;	// Unused
			}
			break;
		case ERCT_SetRasterizerState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetRasterizerState>();
				if (!bOnlyTestMemAccess)
				{
					RHISetRasterizerState(RHICmd->State);
				}
				RHICmd->State->Release();
			}
			break;
		case ERCT_SetShaderParameter:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderParameter>();
				if (!bOnlyTestMemAccess)
				{
					switch (RHICmd->ShaderFrequency)
					{
					case SF_Vertex:		RHISetShaderParameter((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Hull:		RHISetShaderParameter((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Domain:		RHISetShaderParameter((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Geometry:	RHISetShaderParameter((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Pixel:		RHISetShaderParameter((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Compute:	RHISetShaderParameter((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					default: check(0); break;
					}
				}
			}
			break;
		case ERCT_SetShaderUniformBuffer:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderUniformBuffer>();
				if (!bOnlyTestMemAccess)
				{
					switch (RHICmd->ShaderFrequency)
					{
					case SF_Vertex:		RHISetShaderUniformBuffer((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Hull:		RHISetShaderUniformBuffer((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Domain:		RHISetShaderUniformBuffer((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Geometry:	RHISetShaderUniformBuffer((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Pixel:		RHISetShaderUniformBuffer((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Compute:	RHISetShaderUniformBuffer((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					default: check(0); break;
					}
				}
				RHICmd->Shader->Release(); 
				RHICmd->UniformBuffer->Release();
			}
			break;
		case ERCT_SetShaderSampler:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderSampler>();
				if (!bOnlyTestMemAccess)
				{
					switch (RHICmd->ShaderFrequency)
					{
					case SF_Vertex:		RHISetShaderSampler((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Hull:		RHISetShaderSampler((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Domain:		RHISetShaderSampler((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Geometry:	RHISetShaderSampler((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Pixel:		RHISetShaderSampler((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Compute:	RHISetShaderSampler((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					default: check(0); break;
					}
				}
				RHICmd->Shader->Release(); 
				RHICmd->Sampler->Release();
			}
			break;
		case ERCT_SetShaderTexture:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderTexture>();
				if (!bOnlyTestMemAccess)
				{
					switch (RHICmd->ShaderFrequency)
					{
					case SF_Vertex:		RHISetShaderTexture((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Hull:		RHISetShaderTexture((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Domain:		RHISetShaderTexture((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Geometry:	RHISetShaderTexture((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Pixel:		RHISetShaderTexture((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Compute:	RHISetShaderTexture((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					default: check(0); break;
					}
				}
				RHICmd->Shader->Release(); 
				RHICmd->Texture->Release();
			}
			break;
		case ERCT_DrawPrimitive:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawPrimitive>();
				if (!bOnlyTestMemAccess)
				{
					RHIDrawPrimitive(RHICmd->PrimitiveType, RHICmd->BaseVertexIndex, RHICmd->NumPrimitives, RHICmd->NumInstances);
				}
			}
			break;
		case ERCT_DrawIndexedPrimitive:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawIndexedPrimitive>();
				if (!bOnlyTestMemAccess)
				{
					RHIDrawIndexedPrimitive(RHICmd->IndexBuffer, RHICmd->PrimitiveType, RHICmd->BaseVertexIndex, RHICmd->MinIndex, RHICmd->NumVertices, RHICmd->StartIndex, RHICmd->NumPrimitives, RHICmd->NumInstances);
				}
				RHICmd->IndexBuffer->Release();
			}
			break;
		case ERCT_SetBoundShaderState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetBoundShaderState>();
				if (!bOnlyTestMemAccess)
				{
					RHISetBoundShaderState(RHICmd->BoundShaderState);
				}
				RHICmd->BoundShaderState->Release();
			}
			break;
		case ERCT_SetBlendState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetBlendState>();
				if (!bOnlyTestMemAccess)
				{
					RHISetBlendState(RHICmd->State, RHICmd->BlendFactor);
				}
				RHICmd->State->Release();
			}
			break;
		case ERCT_SetStreamSource:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetStreamSource>();
				if (!bOnlyTestMemAccess)
				{
					RHISetStreamSource(RHICmd->StreamIndex, RHICmd->VertexBuffer, RHICmd->Stride, RHICmd->Offset);
				}
				RHICmd->VertexBuffer->Release();
			}
			break;
		case ERCT_SetDepthStencilState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetDepthStencilState>();
				if (!bOnlyTestMemAccess)
				{
					RHISetDepthStencilState(RHICmd->State, RHICmd->StencilRef);
				}
				RHICmd->State->Release();
			}
			break;
		default:
			checkf(0, TEXT("Unknown RHI Command %d!"), Cmd->Type);
		}
	}

	Iter.CheckNumCommands();
}


void FRHICommandListExecutor::ExecuteAndFreeList(FRHICommandList& CmdList)
{
	if (CmdList.IsNull())
	{
		// HACK!
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_RHICmdListExecuteTime);

	check(CmdList.CanAddCommand());

	FScopeLock Lock(&CriticalSection);
	check(bLock);
	bLock = false;

	CmdList.State = FRHICommandList::Executing;

	INC_MEMORY_STAT_BY(STAT_RHICmdListMemory, CmdList.GetUsedMemory());

	static IConsoleVariable* RHICmdListCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHISkip"));
	bool bSkipRHICmdList = (RHICmdListCVar && RHICmdListCVar->GetInt() != 0);

	if (!bSkipRHICmdList)
	{
		static IConsoleVariable* RHIExecCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHIExec"));
		if (RHIExecCVar && RHIExecCVar->GetInt() != 0)
		{
			RHIExecuteCommandList(&CmdList);
		}
		else
		{
			static IConsoleVariable* RHIMemCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHIMem"));
			bool bOnlySkipAroundMem = (RHIMemCVar && RHIMemCVar->GetInt() != 0);
			if (bOnlySkipAroundMem)
			{
				ExecuteList<true>(CmdList);
			}
			else
			{
				ExecuteList<false>(CmdList);
			}
		}
	}

	INC_DWORD_STAT_BY(STAT_RHICmdListCount, CmdList.NumCommands);
	CmdList.Reset();
}

FRHICommandList& FRHICommandListExecutor::CreateList()
{
	FScopeLock Lock(&CriticalSection);
	check(!bLock);
	bLock = true;
	check(GCommandList.State == FRHICommandList::Kicked);
	check(GCommandList.NumCommands == 0);
	GCommandList.State = FRHICommandList::Ready;
	return GCommandList;
}

void FRHICommandListExecutor::Verify()
{
	check(!bLock);
}


FRHICommandList::FMemManager::FMemManager() :
	FirstPage(nullptr),
	LastPage(nullptr)
{
}

FRHICommandList::FMemManager::~FMemManager()
{
	while (FirstPage)
	{
		auto* Page = FirstPage->NextPage;
		delete FirstPage;
		FirstPage = Page;
	}

	FirstPage = LastPage = (FPage*)0x1; // we want this to not bypass the cmd list; please just crash.
}

const SIZE_T FRHICommandList::FMemManager::GetUsedMemory() const
{
	SIZE_T TotalUsedMemory = 0;
	auto* Page = FirstPage;
	do
	{
		SIZE_T UsedPageMemory = Page->MemUsed();
		if (!UsedPageMemory)
		{
			return TotalUsedMemory;
		}
		TotalUsedMemory += UsedPageMemory;
		Page = Page->NextPage;
	}
	while (Page);
	return TotalUsedMemory;
}

void FRHICommandList::FMemManager::Reset()
{
	auto* Page = FirstPage;
	while (Page)
	{
		Page->Reset();
		Page = Page->NextPage;
	}
	LastPage = FirstPage;
}

const SIZE_T FRHICommandList::GetUsedMemory() const
{
	return MemManager.GetUsedMemory();
}
