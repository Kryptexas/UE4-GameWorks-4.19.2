// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "RHI.h"
#include "RHICommandList.h"

DEFINE_STAT(STAT_ImmedCmdListExecuteTime);
DEFINE_STAT(STAT_ImmedCmdListMemory);
DEFINE_STAT(STAT_ImmedCmdListCount);

DEFINE_STAT(STAT_NonImmedCmdListExecuteTime);
DEFINE_STAT(STAT_NonImmedCmdListMemory);
DEFINE_STAT(STAT_NonImmedCmdListCount);

static TAutoConsoleVariable<int32> CVarRHICmdBypass(
	TEXT("r.RHICmdBypass"),
	FRHICommandListExecutor::DefaultBypass,
	TEXT("Whether to bypass the rhi command list and send the rhi commands immediately.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static int32 GRHICmdExec = 0;
static FAutoConsoleVariableRef CVarRHICmdExec(
	TEXT("r.RHIExec"),
	GRHICmdExec,
	TEXT("0: Executes RHI Command List using RHI calls (default)\n")
	TEXT("1: Executes RHI Command List using platform specific APIs")
	);


RHI_API FRHICommandListExecutor GRHICommandList;
RHI_API TLockFreeFixedSizeAllocator<FRHICommandList::FMemManager::FPage::PageSize> FRHICommandList::FMemManager::FPage::TheAllocator;
RHI_API void(*SetGlobalBoundShaderState_InternalPtr)(FGlobalBoundShaderState& BoundShaderState);


//RHI_API FGlobalRHI GRHI;

static_assert(sizeof(FRHICommand) == sizeof(FRHICommandNopEndOfPage), "These should match for filling in the end of a page when a new allocation won't fit");

void FRHICommandListExecutor::ExecuteListGenericRHI(FRHICommandList& CmdList)
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
				SetRasterizerState_Internal(RHICmd->State);
			}
			break;
		case ERCT_SetShaderParameter:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderParameter>();
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
			break;
		case ERCT_SetShaderUniformBuffer:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderUniformBuffer>();
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
			break;
		case ERCT_SetShaderSampler:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderSampler>();
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
			break;
		case ERCT_SetShaderTexture:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderTexture>();
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
			break;
		case ERCT_SetShaderResourceViewParameter:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetShaderResourceViewParameter>();
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
			break;
		case ERCT_SetUAVParameter:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetUAVParameter>();
				switch (RHICmd->ShaderFrequency)
				{
				case SF_Compute:	SetUAVParameter_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->UAVIndex, RHICmd->UAV); break;
				default: check(0); break;
				}
			}
			break;
		case ERCT_SetUAVParameter_IntialCount:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetUAVParameter_IntialCount>();
				switch (RHICmd->ShaderFrequency)
				{
				case SF_Compute:	SetUAVParameter_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->UAVIndex, RHICmd->UAV, RHICmd->InitialCount); break;
				default: check(0); break;
				}
			}
			break;
		case ERCT_DrawPrimitive:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawPrimitive>();
				DrawPrimitive_Internal(RHICmd->PrimitiveType, RHICmd->BaseVertexIndex, RHICmd->NumPrimitives, RHICmd->NumInstances);
			}
			break;
		case ERCT_DrawIndexedPrimitive:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawIndexedPrimitive>();
				DrawIndexedPrimitive_Internal(RHICmd->IndexBuffer, RHICmd->PrimitiveType, RHICmd->BaseVertexIndex, RHICmd->MinIndex, RHICmd->NumVertices, RHICmd->StartIndex, RHICmd->NumPrimitives, RHICmd->NumInstances);
			}
			break;
		case ERCT_SetBoundShaderState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetBoundShaderState>();
				SetBoundShaderState_Internal(RHICmd->BoundShaderState);
			}
			break;
		case ERCT_SetBlendState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetBlendState>();
				SetBlendState_Internal(RHICmd->State, RHICmd->BlendFactor);
			}
			break;
		case ERCT_SetStreamSource:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetStreamSource>();
				SetStreamSource_Internal(RHICmd->StreamIndex, RHICmd->VertexBuffer, RHICmd->Stride, RHICmd->Offset);
			}
			break;
		case ERCT_SetDepthStencilState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetDepthStencilState>();
				SetDepthStencilState_Internal(RHICmd->State, RHICmd->StencilRef);
			}
			break;
		case ERCT_SetViewport:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetViewport>();
				SetViewport_Internal(RHICmd->MinX, RHICmd->MinY, RHICmd->MinZ, RHICmd->MaxX, RHICmd->MaxY, RHICmd->MaxZ);
			}
			break;
		case ERCT_SetScissorRect:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetScissorRect>();
				SetScissorRect_Internal(RHICmd->bEnable, RHICmd->MinX, RHICmd->MinY, RHICmd->MaxX, RHICmd->MaxY);
			}
			break;
		case ERCT_SetRenderTargets:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetRenderTargets>();
				SetRenderTargets_Internal(
					RHICmd->NewNumSimultaneousRenderTargets,
					RHICmd->NewRenderTargetsRHI,
					RHICmd->NewDepthStencilTargetRHI,
					RHICmd->NewNumUAVs,
					RHICmd->UAVs);
			}
			break;
		case ERCT_EndDrawPrimitiveUP:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandEndDrawPrimitiveUP>();
				void* Buffer = NULL;
				BeginDrawPrimitiveUP_Internal(RHICmd->PrimitiveType, RHICmd->NumPrimitives, RHICmd->NumVertices, RHICmd->VertexDataStride, Buffer);
				FMemory::Memcpy(Buffer, RHICmd->OutVertexData, RHICmd->NumVertices * RHICmd->VertexDataStride);
				EndDrawPrimitiveUP_Internal();
				FMemory::Free(RHICmd->OutVertexData);
			}
			break;
		case ERCT_EndDrawIndexedPrimitiveUP:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandEndDrawIndexedPrimitiveUP>();
				void* VertexBuffer = NULL;
				void* IndexBuffer = NULL;
				BeginDrawIndexedPrimitiveUP_Internal(
					RHICmd->PrimitiveType,
					RHICmd->NumPrimitives,
					RHICmd->NumVertices,
					RHICmd->VertexDataStride,
					VertexBuffer,
					RHICmd->MinVertexIndex,
					RHICmd->NumIndices,
					RHICmd->IndexDataStride,
					IndexBuffer);
				FMemory::Memcpy(VertexBuffer, RHICmd->OutVertexData, RHICmd->NumVertices * RHICmd->VertexDataStride);
				FMemory::Memcpy(IndexBuffer, RHICmd->OutIndexData, RHICmd->NumIndices * RHICmd->IndexDataStride);
				EndDrawIndexedPrimitiveUP_Internal();
				FMemory::Free(RHICmd->OutVertexData);
				FMemory::Free(RHICmd->OutIndexData);
			}
			break;
		case ERCT_BuildLocalBoundShaderState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandBuildLocalBoundShaderState>();
				check(RHICmd->WorkArea.CheckCmdList == &CmdList && RHICmd->WorkArea.UID == CmdList.UID // this BSS was not built for this particular commandlist
					&& !IsValidRef(RHICmd->WorkArea.BSS)); // should not already have been created
				if (RHICmd->WorkArea.UseCount)
				{
					RHICmd->WorkArea.BSS = CreateBoundShaderState_Internal(RHICmd->WorkArea.Args.VertexDeclarationRHI, RHICmd->WorkArea.Args.VertexShaderRHI, RHICmd->WorkArea.Args.HullShaderRHI, RHICmd->WorkArea.Args.DomainShaderRHI, RHICmd->WorkArea.Args.PixelShaderRHI, RHICmd->WorkArea.Args.GeometryShaderRHI);
				}
			}
			break;
		case ERCT_SetLocalBoundShaderState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetLocalBoundShaderState>();
				check(RHICmd->LocalBoundShaderState.WorkArea->CheckCmdList == &CmdList && RHICmd->LocalBoundShaderState.WorkArea->UID == CmdList.UID // this BSS was not built for this particular commandlist
					&& RHICmd->LocalBoundShaderState.WorkArea->UseCount > 0 && IsValidRef(RHICmd->LocalBoundShaderState.WorkArea->BSS)); // this should have been created and should have uses outstanding

				SetBoundShaderState_Internal(RHICmd->LocalBoundShaderState.WorkArea->BSS);

				if (--RHICmd->LocalBoundShaderState.WorkArea->UseCount == 0)
				{
					RHICmd->LocalBoundShaderState.WorkArea->BSS = nullptr; // this also releases the ref, which wouldn't otherwise happen because RHICommands are unsafe and don't do constructors or destructors
				}
			}
			break;
		case ERCT_SetGlobalBoundShaderState:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetGlobalBoundShaderState>();
				SetGlobalBoundShaderState_InternalPtr(RHICmd->GlobalBoundShaderState);
			}
			break;

		case ERCT_SetComputeShader:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetComputeShader>();
				SetComputeShader_Internal(RHICmd->ComputeShader);
			}
			break;
		case ERCT_DispatchComputeShader:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDispatchComputeShader>();
				DispatchComputeShader_Internal(RHICmd->ThreadGroupCountX, RHICmd->ThreadGroupCountY, RHICmd->ThreadGroupCountZ);
			}
			break;
		case ERCT_DispatchIndirectComputeShader:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDispatchIndirectComputeShader>();
				DispatchIndirectComputeShader_Internal(RHICmd->ArgumentBuffer, RHICmd->ArgumentOffset);
			}
			break;
		case ERCT_AutomaticCacheFlushAfterComputeShader:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandAutomaticCacheFlushAfterComputeShader>();
				AutomaticCacheFlushAfterComputeShader_Internal(RHICmd->bEnable);
			}
			break;
		case ERCT_FlushComputeShaderCache:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandFlushComputeShaderCache>();
				FlushComputeShaderCache_Internal();
			}
			break;
		case ERCT_DrawPrimitiveIndirect:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawPrimitiveIndirect>();
				DrawPrimitiveIndirect_Internal(RHICmd->PrimitiveType, RHICmd->ArgumentBuffer, RHICmd->ArgumentOffset);
			}
			break;
		case ERCT_DrawIndexedIndirect:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawIndexedIndirect>();
				DrawIndexedIndirect_Internal(RHICmd->IndexBufferRHI, RHICmd->PrimitiveType, RHICmd->ArgumentsBufferRHI, RHICmd->DrawArgumentsIndex, RHICmd->NumInstances);
			}
			break;
		case ERCT_DrawIndexedPrimitiveIndirect:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandDrawIndexedPrimitiveIndirect>();
				DrawIndexedPrimitiveIndirect_Internal(RHICmd->PrimitiveType, RHICmd->IndexBuffer, RHICmd->ArgumentsBuffer, RHICmd->ArgumentOffset);
			}
			break;
		case ERCT_EnableDepthBoundsTest:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandEnableDepthBoundsTest>();
				EnableDepthBoundsTest_Internal(RHICmd->bEnable, RHICmd->MinDepth, RHICmd->MaxDepth);
			}
			break;
		case ERCT_ClearUAV:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandClearUAV>();
				ClearUAV_Internal(RHICmd->UnorderedAccessViewRHI, RHICmd->Values);
			}
			break;
		case ERCT_CopyToResolveTarget:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandCopyToResolveTarget>();
				CopyToResolveTarget_Internal(RHICmd->SourceTexture, RHICmd->DestTexture, RHICmd->bKeepOriginalSurface, RHICmd->ResolveParams);
			}
			break;
		case ERCT_Clear:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandClear>();
				Clear_Internal(RHICmd->bClearColor, RHICmd->Color, RHICmd->bClearDepth, RHICmd->Depth, RHICmd->bClearStencil, RHICmd->Stencil, RHICmd->ExcludeRect);
			}
			break;
		case ERCT_ClearMRT:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandClearMRT>();
				ClearMRT_Internal(RHICmd->bClearColor, RHICmd->NumClearColors, RHICmd->ColorArray, RHICmd->bClearDepth, RHICmd->Depth, RHICmd->bClearStencil, RHICmd->Stencil, RHICmd->ExcludeRect);
			}
			break;
		case ERCT_BuildLocalUniformBuffer:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandBuildLocalUniformBuffer>();
				check(RHICmd->WorkArea.CheckCmdList == &CmdList && RHICmd->WorkArea.UID == CmdList.UID // this uniform buffer was not built for this particular commandlist
					&& !IsValidRef(RHICmd->WorkArea.UniformBuffer) && RHICmd->WorkArea.Layout && RHICmd->WorkArea.Contents); // should not already have been created
				if (RHICmd->WorkArea.UseCount)
				{
					RHICmd->WorkArea.UniformBuffer = RHICreateUniformBuffer(RHICmd->WorkArea.Contents, *RHICmd->WorkArea.Layout, UniformBuffer_SingleFrame);
				}
				FMemory::Free(RHICmd->WorkArea.Contents);
				RHICmd->WorkArea.Contents = nullptr;
			}
			break;
		case ERCT_SetLocalUniformBuffer:
			{
				auto* RHICmd = Iter.NextCommand<FRHICommandSetLocalUniformBuffer>();
				check(RHICmd->LocalUniformBuffer.WorkArea->CheckCmdList == &CmdList && RHICmd->LocalUniformBuffer.WorkArea->UID == CmdList.UID // this BSS was not built for this particular commandlist
					&& RHICmd->LocalUniformBuffer.WorkArea->UseCount > 0 && IsValidRef(RHICmd->LocalUniformBuffer.WorkArea->UniformBuffer)); // this should have been created and should have uses outstanding
				switch (RHICmd->ShaderFrequency)
				{
					case SF_Vertex:		SetShaderUniformBuffer_Internal((FVertexShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->LocalUniformBuffer.WorkArea->UniformBuffer); break;
					case SF_Hull:		SetShaderUniformBuffer_Internal((FHullShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->LocalUniformBuffer.WorkArea->UniformBuffer); break;
					case SF_Domain:		SetShaderUniformBuffer_Internal((FDomainShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->LocalUniformBuffer.WorkArea->UniformBuffer); break;
					case SF_Geometry:	SetShaderUniformBuffer_Internal((FGeometryShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->LocalUniformBuffer.WorkArea->UniformBuffer); break;
					case SF_Pixel:		SetShaderUniformBuffer_Internal((FPixelShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->LocalUniformBuffer.WorkArea->UniformBuffer); break;
					case SF_Compute:	SetShaderUniformBuffer_Internal((FComputeShaderRHIParamRef)RHICmd->Shader, RHICmd->BaseIndex, RHICmd->LocalUniformBuffer.WorkArea->UniformBuffer); break;
					default: check(0); break;
				}
				if (--RHICmd->LocalUniformBuffer.WorkArea->UseCount == 0)
				{
					RHICmd->LocalUniformBuffer.WorkArea->UniformBuffer = nullptr; // this also releases the ref, which wouldn't otherwise happen because RHICommands are unsafe and don't do constructors or destructors
					RHICmd->LocalUniformBuffer.WorkArea->Layout = nullptr;
				}
			}
			break;

		default:
			checkf(0, TEXT("Unknown RHI Command %d!"), Cmd->Type);
		}
	}

	Iter.CheckNumCommands();
}

void FRHICommandListExecutor::ExecuteInner(FRHICommandList& CmdList)
{
	static IConsoleVariable* RHIExecCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHIExec"));
	CmdList.bExecuting = true;
	if (RHIExecCVar && RHIExecCVar->GetInt() != 0)
	{
		RHIExecuteCommandList(&CmdList);
	}
	else
	{
		ExecuteListGenericRHI(CmdList);
	}
	CmdList.Reset();
}

void FRHICommandListExecutor::ExecuteList(FRHICommandList& CmdList)
{
	check(IsInRenderingThread() && &CmdList != &GetImmediateCommandList());

	if (!GetImmediateCommandList().bExecuting) // don't flush if this is a recursive call and we are already executing the immediate command list
	{
		GetImmediateCommandList().Flush();
	}

	INC_MEMORY_STAT_BY(STAT_NonImmedCmdListMemory, CmdList.GetUsedMemory());
	INC_DWORD_STAT_BY(STAT_NonImmedCmdListCount, CmdList.NumCommands);

	SCOPE_CYCLE_COUNTER(STAT_NonImmedCmdListExecuteTime);
	ExecuteInner(CmdList);
}

void FRHICommandListExecutor::ExecuteList(FRHICommandListImmediate& CmdList)
{
	check(IsInRenderingThread() && &CmdList == &GetImmediateCommandList());

	INC_MEMORY_STAT_BY(STAT_ImmedCmdListMemory, CmdList.GetUsedMemory());
	INC_DWORD_STAT_BY(STAT_ImmedCmdListCount, CmdList.NumCommands);

	{
		SCOPE_CYCLE_COUNTER(STAT_ImmedCmdListExecuteTime);
		ExecuteInner(CmdList);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (GRHICommandList.OutstandingCmdListCount.GetValue() == 1)
	{
		LatchBypass();
	}
#endif
}


void FRHICommandListExecutor::LatchBypass()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	check(GRHICommandList.OutstandingCmdListCount.GetValue() == 1 && !GRHICommandList.GetImmediateCommandList().HasCommands());
	bool NewBypass = (CVarRHICmdBypass.GetValueOnRenderThread() >= 1);
	bLatchedBypass = NewBypass;
#endif
}

void FRHICommandListExecutor::CheckNoOutstandingCmdLists()
{
	check(GRHICommandList.OutstandingCmdListCount.GetValue() == 1); // else we are attempting to delete resources while there is still a live cmdlist (other than the immediate cmd list) somewhere.
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

FRHICommandList::FRHICommandList()
{
	GRHICommandList.OutstandingCmdListCount.Increment();
	Reset();
}

FRHICommandList::~FRHICommandList()
{
	Flush();
	GRHICommandList.OutstandingCmdListCount.Decrement();
}

const SIZE_T FRHICommandList::GetUsedMemory() const
{
	return MemManager.GetUsedMemory();
}

void FRHICommandList::Reset()
{
	bExecuting = false;
	NumCommands = 0;
	MemManager.Reset();
	UID = GRHICommandList.UIDCounter.Increment();
}


