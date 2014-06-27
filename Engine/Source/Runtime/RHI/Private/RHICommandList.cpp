// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "RHI.h"
#include "RHICommandList.h"
//#include "../../RenderCore/Public/RenderingThread.h"

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

static TAutoConsoleVariable<int32> CVarRHICmdBypass(
	TEXT("r.RHICmdBypass"),
	FRHICommandListExecutor::DefaultBypass,
	TEXT("Whether to bypass the rhi command list and send the rhi commands immediately.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

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
//RHI_API FGlobalRHI GRHI;

static_assert(sizeof(FRHICommand) == sizeof(FRHICommandNopEndOfPage), "These should match for filling in the end of a page when a new allocation won't fit");

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
					SetRasterizerState_Internal(RHICmd->State);
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
					case SF_Vertex:		SetShaderParameter_Internal((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Hull:		SetShaderParameter_Internal((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Domain:		SetShaderParameter_Internal((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Geometry:	SetShaderParameter_Internal((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Pixel:		SetShaderParameter_Internal((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
					case SF_Compute:	SetShaderParameter_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
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
					case SF_Vertex:		SetShaderUniformBuffer_Internal((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Hull:		SetShaderUniformBuffer_Internal((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Domain:		SetShaderUniformBuffer_Internal((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Geometry:	SetShaderUniformBuffer_Internal((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Pixel:		SetShaderUniformBuffer_Internal((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
					case SF_Compute:	SetShaderUniformBuffer_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
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
					case SF_Vertex:		SetShaderSampler_Internal((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Hull:		SetShaderSampler_Internal((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Domain:		SetShaderSampler_Internal((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Geometry:	SetShaderSampler_Internal((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Pixel:		SetShaderSampler_Internal((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
					case SF_Compute:	SetShaderSampler_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
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
					case SF_Vertex:		SetShaderTexture_Internal((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Hull:		SetShaderTexture_Internal((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Domain:		SetShaderTexture_Internal((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Geometry:	SetShaderTexture_Internal((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Pixel:		SetShaderTexture_Internal((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					case SF_Compute:	SetShaderTexture_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
					default: check(0); break;
					}
				}
				RHICmd->Shader->Release(); 
				RHICmd->Texture->Release();
			}
			break;
		case ERCT_SetShaderResourceViewParameter:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderResourceViewParameter>();
				if (!bOnlyTestMemAccess)
				{
					switch (RHICmd->ShaderFrequency)
					{
					case SF_Vertex:		SetShaderResourceViewParameter_Internal((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->SRV); break;
					case SF_Hull:		SetShaderResourceViewParameter_Internal((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->SRV); break;
					case SF_Domain:		SetShaderResourceViewParameter_Internal((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->SRV); break;
					case SF_Geometry:	SetShaderResourceViewParameter_Internal((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->SRV); break;
					case SF_Pixel:		SetShaderResourceViewParameter_Internal((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->SRV); break;
					case SF_Compute:	SetShaderResourceViewParameter_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->SRV); break;
					default: check(0); break;
					}
				}
				RHICmd->Shader->Release();
				RHICmd->SRV->Release();
			}
			break;
		case ERCT_SetUAVParameter:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetUAVParameter>();
				if (!bOnlyTestMemAccess)
				{
					switch (RHICmd->ShaderFrequency)
					{
					case SF_Compute:	SetUAVParameter_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->UAVIndex, RHICmd->UAV); break;
					default: check(0); break;
					}
				}
				RHICmd->Shader->Release();
				RHICmd->UAV->Release();
			}
			break;
		case ERCT_SetUAVParameter_IntialCount:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetUAVParameter_IntialCount>();
				if (!bOnlyTestMemAccess)
				{
					switch (RHICmd->ShaderFrequency)
					{
					case SF_Compute:	SetUAVParameter_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->UAVIndex, RHICmd->UAV, RHICmd->InitialCount); break;
					default: check(0); break;
					}
				}
				RHICmd->Shader->Release();
				RHICmd->UAV->Release();
			}
			break;
		case ERCT_DrawPrimitive:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawPrimitive>();
				if (!bOnlyTestMemAccess)
				{
					DrawPrimitive_Internal(RHICmd->PrimitiveType, RHICmd->BaseVertexIndex, RHICmd->NumPrimitives, RHICmd->NumInstances);
				}
			}
			break;
		case ERCT_DrawIndexedPrimitive:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawIndexedPrimitive>();
				if (!bOnlyTestMemAccess)
				{
					DrawIndexedPrimitive_Internal(RHICmd->IndexBuffer, RHICmd->PrimitiveType, RHICmd->BaseVertexIndex, RHICmd->MinIndex, RHICmd->NumVertices, RHICmd->StartIndex, RHICmd->NumPrimitives, RHICmd->NumInstances);
				}
				RHICmd->IndexBuffer->Release();
			}
			break;
		case ERCT_SetBoundShaderState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetBoundShaderState>();
				if (!bOnlyTestMemAccess)
				{
					SetBoundShaderState_Internal(RHICmd->BoundShaderState);
				}
				RHICmd->BoundShaderState->Release();
			}
			break;
		case ERCT_SetBlendState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetBlendState>();
				if (!bOnlyTestMemAccess)
				{
					SetBlendState_Internal(RHICmd->State, RHICmd->BlendFactor);
				}
				RHICmd->State->Release();
			}
			break;
		case ERCT_SetStreamSource:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetStreamSource>();
				if (!bOnlyTestMemAccess)
				{
					SetStreamSource_Internal(RHICmd->StreamIndex, RHICmd->VertexBuffer, RHICmd->Stride, RHICmd->Offset);
				}
				RHICmd->VertexBuffer->Release();
			}
			break;
		case ERCT_SetDepthStencilState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetDepthStencilState>();
				if (!bOnlyTestMemAccess)
				{
					SetDepthStencilState_Internal(RHICmd->State, RHICmd->StencilRef);
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


void FRHICommandListExecutor::ExecuteList(FRHICommandList& CmdList)
{
	CmdList.bExecuting = true;
	SCOPE_CYCLE_COUNTER(STAT_RHICmdListExecuteTime);

	check(IsInRenderingThread());

	INC_MEMORY_STAT_BY(STAT_RHICmdListMemory, CmdList.GetUsedMemory());

	static IConsoleVariable* RHICmdListCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHISkip"));
	bool bSkipRHICmdList = (RHICmdListCVar && RHICmdListCVar->GetInt() != 0);

	if (!bSkipRHICmdList)
	{
		if (&CmdList != &GetImmediateCommandList())
		{
			GetImmediateCommandList().Flush();
		}
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
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (&CmdList == &GetImmediateCommandList())
	{
		bLatchedBypass = (CVarRHICmdBypass.GetValueOnRenderThread() >= 1);
	}
#endif
	CmdList.Reset();
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

	FirstPage = LastPage = nullptr;
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
