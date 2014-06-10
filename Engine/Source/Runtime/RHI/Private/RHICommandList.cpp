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
RHI_API FRHICommandList FRHICommandList::NullRHICommandList(0);


//@todo-rco: Temp hack!
static FRHICommandList GCommandList;


FRHICommandListExecutor::FRHICommandListExecutor() :
	bLock(false)
{
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

	auto* CmdPtr = CmdList.Memory;
	auto* CmdTail = CmdList.GetTail();
	INC_MEMORY_STAT_BY(STAT_RHICmdListMemory, (CmdTail - CmdPtr));

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

			uint32 NumCommands = 0;
			while (CmdPtr < CmdTail)
			{
				auto* Cmd = (FRHICommand*)CmdPtr;
				switch (Cmd->Type)
				{
				case ERCT_NopBlob:
					{
						// Nop
						auto* RHICmd = (FRHICommandNopBlob*)Cmd;
						CmdPtr += sizeof(FRHICommandNopBlob) + RHICmd->Size;
					}
					break;
				case ERCT_SetRasterizerState:
					{
						auto* RHICmd = (FRHICommandSetRasterizerState*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							RHISetRasterizerState(RHICmd->State);
						}
						CmdPtr += sizeof(FRHICommandSetRasterizerState);
						RHICmd->State->Release();
					}
					break;
				case ERCT_SetShaderParameter:
					{
						auto* RHICmd = (FRHICommandSetShaderParameter*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							switch (RHICmd->ShaderFrequency)
							{
							case SF_Vertex: RHISetShaderParameter((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
							case SF_Hull: RHISetShaderParameter((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
							case SF_Domain: RHISetShaderParameter((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
							case SF_Geometry: RHISetShaderParameter((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
							case SF_Pixel: RHISetShaderParameter((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
							case SF_Compute: RHISetShaderParameter((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->BufferIndex, RHICmd->BaseIndex, RHICmd->NumBytes, RHICmd->NewValue); break;
							default: check(0); break;
							}
						}
						CmdPtr += sizeof(FRHICommandSetShaderParameter);
					}
					break;
				case ERCT_SetShaderUniformBuffer:
					{
						auto* RHICmd = (FRHICommandSetShaderUniformBuffer*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							switch (RHICmd->ShaderFrequency)
							{
							case SF_Vertex: RHISetShaderUniformBuffer((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
							case SF_Hull: RHISetShaderUniformBuffer((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
							case SF_Domain: RHISetShaderUniformBuffer((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
							case SF_Geometry: RHISetShaderUniformBuffer((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
							case SF_Pixel: RHISetShaderUniformBuffer((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
							case SF_Compute: RHISetShaderUniformBuffer((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->UniformBuffer); break;
							default: check(0); break;
							}
						}
						CmdPtr += sizeof(FRHICommandSetShaderUniformBuffer);
						RHICmd->Shader->Release(); 
						RHICmd->UniformBuffer->Release();
					}
					break;
				case ERCT_SetShaderSampler:
					{
						auto* RHICmd = (FRHICommandSetShaderSampler*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							switch (RHICmd->ShaderFrequency)
							{
							case SF_Vertex: RHISetShaderSampler((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
							case SF_Hull: RHISetShaderSampler((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
							case SF_Domain: RHISetShaderSampler((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
							case SF_Geometry: RHISetShaderSampler((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
							case SF_Pixel: RHISetShaderSampler((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
							case SF_Compute: RHISetShaderSampler((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->SamplerIndex, RHICmd->Sampler); break;
							default: check(0); break;
							}
						}
						CmdPtr += sizeof(FRHICommandSetShaderSampler);
						RHICmd->Shader->Release(); 
						RHICmd->Sampler->Release();
					}
					break;
				case ERCT_SetShaderTexture:
					{
						auto* RHICmd = (FRHICommandSetShaderTexture*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							switch (RHICmd->ShaderFrequency)
							{
							case SF_Vertex: RHISetShaderTexture((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
							case SF_Hull: RHISetShaderTexture((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
							case SF_Domain: RHISetShaderTexture((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
							case SF_Geometry: RHISetShaderTexture((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
							case SF_Pixel: RHISetShaderTexture((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
							case SF_Compute: RHISetShaderTexture((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->TextureIndex, RHICmd->Texture); break;
							default: check(0); break;
							}
						}
						CmdPtr += sizeof(FRHICommandSetShaderTexture);
						RHICmd->Shader->Release(); 
						RHICmd->Texture->Release();
					}
					break;
				case ERCT_DrawPrimitive:
					{
						auto* RHICmd = (FRHICommandDrawPrimitive*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							RHIDrawPrimitive(RHICmd->PrimitiveType, RHICmd->BaseVertexIndex, RHICmd->NumPrimitives, RHICmd->NumInstances);
						}
						CmdPtr += sizeof(FRHICommandDrawPrimitive);
					}
					break;
				case ERCT_DrawIndexedPrimitive:
					{
						auto* RHICmd = (FRHICommandDrawIndexedPrimitive*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							RHIDrawIndexedPrimitive(RHICmd->IndexBuffer, RHICmd->PrimitiveType, RHICmd->BaseVertexIndex, RHICmd->MinIndex, RHICmd->NumVertices, RHICmd->StartIndex, RHICmd->NumPrimitives, RHICmd->NumInstances);
						}
						CmdPtr += sizeof(FRHICommandDrawIndexedPrimitive);
						RHICmd->IndexBuffer->Release();
					}
					break;
				case ERCT_SetBoundShaderState:
					{
						auto* RHICmd = (FRHICommandSetBoundShaderState*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							RHISetBoundShaderState(RHICmd->BoundShaderState);
						}
						CmdPtr += sizeof(FRHICommandSetBoundShaderState);
						RHICmd->BoundShaderState->Release();
					}
					break;
				case ERCT_SetBlendState:
					{
						auto* RHICmd = (FRHICommandSetBlendState*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							RHISetBlendState(RHICmd->State, RHICmd->BlendFactor);
						}
						CmdPtr += sizeof(FRHICommandSetBlendState);
						RHICmd->State->Release();
					}
					break;
				case ERCT_SetStreamSource:
					{
						auto* RHICmd = (FRHICommandSetStreamSource*)Cmd;
						if (!bOnlySkipAroundMem)
						{
							RHISetStreamSource(RHICmd->StreamIndex, RHICmd->VertexBuffer, RHICmd->Stride, RHICmd->Offset);
						}
						CmdPtr += sizeof(FRHICommandSetStreamSource);
						RHICmd->VertexBuffer->Release();
					}
					break;
				default:
					check(0);
				}

				CmdPtr = Align(CmdPtr, FRHICommandList::Alignment);
				++NumCommands;
			}

			check(CmdList.NumCommands == NumCommands);
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
