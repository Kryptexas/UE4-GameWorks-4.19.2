// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHICommandListCommandExecutes.inl: RHI Command List execute functions.
=============================================================================*/

#if !defined(INTERNAL_DECORATOR)
	#define INTERNAL_DECORATOR(Method) Method##_Internal
#endif

// set this one to get a stat for each RHI command 
#define RHI_STATS 0

#if RHI_STATS
	DECLARE_STATS_GROUP(TEXT("RHICommands"),STATGROUP_RHI_COMMANDS, STATCAT_Advanced);
	#define RHISTAT(Method)	DECLARE_SCOPE_CYCLE_COUNTER(TEXT(#Method), STAT_RHI##Method, STATGROUP_RHI_COMMANDS)
#else
	#define RHISTAT(Method)
#endif

void FRHICommandSetRasterizerState::Execute()
{
	RHISTAT(SetRasterizerState);
	INTERNAL_DECORATOR(SetRasterizerState)(State);
}

void FRHICommandSetDepthStencilState::Execute()
{
	RHISTAT(SetDepthStencilState);
	INTERNAL_DECORATOR(SetDepthStencilState)(State, StencilRef);
}

template <typename TShaderRHIParamRef>
void FRHICommandSetShaderParameter<TShaderRHIParamRef>::Execute()
{
	RHISTAT(SetShaderParameter);
	INTERNAL_DECORATOR(SetShaderParameter)(Shader, BufferIndex, BaseIndex, NumBytes, NewValue); 
}
template struct FRHICommandSetShaderParameter<FVertexShaderRHIParamRef>;
template struct FRHICommandSetShaderParameter<FHullShaderRHIParamRef>;
template struct FRHICommandSetShaderParameter<FDomainShaderRHIParamRef>;
template struct FRHICommandSetShaderParameter<FGeometryShaderRHIParamRef>;
template struct FRHICommandSetShaderParameter<FPixelShaderRHIParamRef>;
template struct FRHICommandSetShaderParameter<FComputeShaderRHIParamRef>;

template <typename TShaderRHIParamRef>
void FRHICommandSetShaderUniformBuffer<TShaderRHIParamRef>::Execute()
{
	RHISTAT(SetShaderUniformBuffer);
	INTERNAL_DECORATOR(SetShaderUniformBuffer)(Shader, BaseIndex, UniformBuffer);
}
template struct FRHICommandSetShaderUniformBuffer<FVertexShaderRHIParamRef>;
template struct FRHICommandSetShaderUniformBuffer<FHullShaderRHIParamRef>;
template struct FRHICommandSetShaderUniformBuffer<FDomainShaderRHIParamRef>;
template struct FRHICommandSetShaderUniformBuffer<FGeometryShaderRHIParamRef>;
template struct FRHICommandSetShaderUniformBuffer<FPixelShaderRHIParamRef>;
template struct FRHICommandSetShaderUniformBuffer<FComputeShaderRHIParamRef>;

template <typename TShaderRHIParamRef>
void FRHICommandSetShaderTexture<TShaderRHIParamRef>::Execute()
{
	RHISTAT(SetShaderTexture);
	INTERNAL_DECORATOR(SetShaderTexture)(Shader, TextureIndex, Texture);
}
template struct FRHICommandSetShaderTexture<FVertexShaderRHIParamRef>;
template struct FRHICommandSetShaderTexture<FHullShaderRHIParamRef>;
template struct FRHICommandSetShaderTexture<FDomainShaderRHIParamRef>;
template struct FRHICommandSetShaderTexture<FGeometryShaderRHIParamRef>;
template struct FRHICommandSetShaderTexture<FPixelShaderRHIParamRef>;
template struct FRHICommandSetShaderTexture<FComputeShaderRHIParamRef>;

template <typename TShaderRHIParamRef>
void FRHICommandSetShaderResourceViewParameter<TShaderRHIParamRef>::Execute()
{
	RHISTAT(SetShaderResourceViewParameter);
	INTERNAL_DECORATOR(SetShaderResourceViewParameter)(Shader, SamplerIndex, SRV);
}
template struct FRHICommandSetShaderResourceViewParameter<FVertexShaderRHIParamRef>;
template struct FRHICommandSetShaderResourceViewParameter<FHullShaderRHIParamRef>;
template struct FRHICommandSetShaderResourceViewParameter<FDomainShaderRHIParamRef>;
template struct FRHICommandSetShaderResourceViewParameter<FGeometryShaderRHIParamRef>;
template struct FRHICommandSetShaderResourceViewParameter<FPixelShaderRHIParamRef>;
template struct FRHICommandSetShaderResourceViewParameter<FComputeShaderRHIParamRef>;

template <typename TShaderRHIParamRef>
void FRHICommandSetUAVParameter<TShaderRHIParamRef>::Execute()
{
	RHISTAT(SetUAVParameter);
	INTERNAL_DECORATOR(SetUAVParameter)(Shader, UAVIndex, UAV);
}
template struct FRHICommandSetUAVParameter<FComputeShaderRHIParamRef>;

template <typename TShaderRHIParamRef>
void FRHICommandSetUAVParameter_IntialCount<TShaderRHIParamRef>::Execute()
{
	RHISTAT(SetUAVParameter);
	INTERNAL_DECORATOR(SetUAVParameter)(Shader, UAVIndex, UAV, InitialCount);
}
template struct FRHICommandSetUAVParameter_IntialCount<FComputeShaderRHIParamRef>;

template <typename TShaderRHIParamRef>
void FRHICommandSetShaderSampler<TShaderRHIParamRef>::Execute()
{
	RHISTAT(SetShaderSampler);
	INTERNAL_DECORATOR(SetShaderSampler)(Shader, SamplerIndex, Sampler);
}
template struct FRHICommandSetShaderSampler<FVertexShaderRHIParamRef>;
template struct FRHICommandSetShaderSampler<FHullShaderRHIParamRef>;
template struct FRHICommandSetShaderSampler<FDomainShaderRHIParamRef>;
template struct FRHICommandSetShaderSampler<FGeometryShaderRHIParamRef>;
template struct FRHICommandSetShaderSampler<FPixelShaderRHIParamRef>;
template struct FRHICommandSetShaderSampler<FComputeShaderRHIParamRef>;

void FRHICommandDrawPrimitive::Execute()
{
	RHISTAT(DrawPrimitive);
	INTERNAL_DECORATOR(DrawPrimitive)(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
}

void FRHICommandDrawIndexedPrimitive::Execute()
{
	RHISTAT(DrawIndexedPrimitive);
	INTERNAL_DECORATOR(DrawIndexedPrimitive)(IndexBuffer, PrimitiveType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, NumPrimitives, NumInstances);
}

void FRHICommandSetBoundShaderState::Execute()
{
	RHISTAT(SetBoundShaderState);
	INTERNAL_DECORATOR(SetBoundShaderState)(BoundShaderState);
}

void FRHICommandSetBlendState::Execute()
{
	RHISTAT(SetBlendState);
	INTERNAL_DECORATOR(SetBlendState)(State, BlendFactor);
}

void FRHICommandSetStreamSource::Execute()
{
	RHISTAT(SetStreamSource);
	INTERNAL_DECORATOR(SetStreamSource)(StreamIndex, VertexBuffer, Stride, Offset);
}

void FRHICommandSetViewport::Execute()
{
	RHISTAT(SetViewport);
	INTERNAL_DECORATOR(SetViewport)(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
}

void FRHICommandSetScissorRect::Execute()
{
	RHISTAT(SetScissorRect);
	INTERNAL_DECORATOR(SetScissorRect)(bEnable, MinX, MinY, MaxX, MaxY);
}

void FRHICommandSetRenderTargets::Execute()
{
	RHISTAT(SetRenderTargets);
	INTERNAL_DECORATOR(SetRenderTargets)(
		NewNumSimultaneousRenderTargets,
		NewRenderTargetsRHI,
		NewDepthStencilTargetRHI,
		NewNumUAVs,
		UAVs);
}

void FRHICommandEndDrawPrimitiveUP::Execute()
{
	RHISTAT(EndDrawPrimitiveUP);
	void* Buffer = NULL;
	INTERNAL_DECORATOR(BeginDrawPrimitiveUP)(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, Buffer);
	FMemory::Memcpy(Buffer, OutVertexData, NumVertices * VertexDataStride);
	INTERNAL_DECORATOR(EndDrawPrimitiveUP)();
}

void FRHICommandEndDrawIndexedPrimitiveUP::Execute()
{
	RHISTAT(EndDrawIndexedPrimitiveUP);
	void* VertexBuffer = nullptr;
	void* IndexBuffer = nullptr;
	INTERNAL_DECORATOR(BeginDrawIndexedPrimitiveUP)(
		PrimitiveType,
		NumPrimitives,
		NumVertices,
		VertexDataStride,
		VertexBuffer,
		MinVertexIndex,
		NumIndices,
		IndexDataStride,
		IndexBuffer);
	FMemory::Memcpy(VertexBuffer, OutVertexData, NumVertices * VertexDataStride);
	FMemory::Memcpy(IndexBuffer, OutIndexData, NumIndices * IndexDataStride);
	INTERNAL_DECORATOR(EndDrawIndexedPrimitiveUP)();
}

void FRHICommandSetComputeShader::Execute()
{
	RHISTAT(SetComputeShader);
	INTERNAL_DECORATOR(SetComputeShader)(ComputeShader);
}

void FRHICommandDispatchComputeShader::Execute()
{
	RHISTAT(DispatchComputeShader);
	INTERNAL_DECORATOR(DispatchComputeShader)(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void FRHICommandDispatchIndirectComputeShader::Execute()
{
	RHISTAT(DispatchIndirectComputeShader);
	INTERNAL_DECORATOR(DispatchIndirectComputeShader)(ArgumentBuffer, ArgumentOffset);
}

void FRHICommandAutomaticCacheFlushAfterComputeShader::Execute()
{
	RHISTAT(AutomaticCacheFlushAfterComputeShader);
	INTERNAL_DECORATOR(AutomaticCacheFlushAfterComputeShader)(bEnable);
}

void FRHICommandFlushComputeShaderCache::Execute()
{
	RHISTAT(FlushComputeShaderCache);
	INTERNAL_DECORATOR(FlushComputeShaderCache)();
}

void FRHICommandDrawPrimitiveIndirect::Execute()
{
	RHISTAT(DrawPrimitiveIndirect);
	INTERNAL_DECORATOR(DrawPrimitiveIndirect)(PrimitiveType, ArgumentBuffer, ArgumentOffset);
}

void FRHICommandDrawIndexedIndirect::Execute()
{
	RHISTAT(DrawIndexedIndirect);
	INTERNAL_DECORATOR(DrawIndexedIndirect)(IndexBufferRHI, PrimitiveType, ArgumentsBufferRHI, DrawArgumentsIndex, NumInstances);
}

void FRHICommandDrawIndexedPrimitiveIndirect::Execute()
{
	RHISTAT(DrawIndexedPrimitiveIndirect);
	INTERNAL_DECORATOR(DrawIndexedPrimitiveIndirect)(PrimitiveType, IndexBuffer, ArgumentsBuffer, ArgumentOffset);
}

void FRHICommandEnableDepthBoundsTest::Execute()
{
	RHISTAT(EnableDepthBoundsTest);
	INTERNAL_DECORATOR(EnableDepthBoundsTest)(bEnable, MinDepth, MaxDepth);
}

void FRHICommandClearUAV::Execute()
{
	RHISTAT(ClearUAV);
	INTERNAL_DECORATOR(ClearUAV)(UnorderedAccessViewRHI, Values);
}

void FRHICommandCopyToResolveTarget::Execute()
{
	RHISTAT(CopyToResolveTarget);
	INTERNAL_DECORATOR(CopyToResolveTarget)(SourceTexture, DestTexture, bKeepOriginalSurface, ResolveParams);
}

void FRHICommandClear::Execute()
{
	RHISTAT(Clear);
	INTERNAL_DECORATOR(Clear)(bClearColor, Color, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
}

void FRHICommandClearMRT::Execute()
{
	RHISTAT(ClearMRT);
	INTERNAL_DECORATOR(ClearMRT)(bClearColor, NumClearColors, ColorArray, bClearDepth, Depth, bClearStencil, Stencil, ExcludeRect);
}

void FRHICommandBuildLocalBoundShaderState::Execute()
{
	RHISTAT(BuildLocalBoundShaderState);
	check(!IsValidRef(WorkArea.ComputedBSS->BSS)); // should not already have been created
	if (WorkArea.ComputedBSS->UseCount)
	{
		WorkArea.ComputedBSS->BSS = 	
			INTERNAL_DECORATOR(CreateBoundShaderState)(WorkArea.Args.VertexDeclarationRHI, WorkArea.Args.VertexShaderRHI, WorkArea.Args.HullShaderRHI, WorkArea.Args.DomainShaderRHI, WorkArea.Args.PixelShaderRHI, WorkArea.Args.GeometryShaderRHI);
	}
}

void FRHICommandSetLocalBoundShaderState::Execute()
{
	RHISTAT(SetLocalBoundShaderState);
	check(LocalBoundShaderState.WorkArea->ComputedBSS->UseCount > 0 && IsValidRef(LocalBoundShaderState.WorkArea->ComputedBSS->BSS)); // this should have been created and should have uses outstanding

	INTERNAL_DECORATOR(SetBoundShaderState)(LocalBoundShaderState.WorkArea->ComputedBSS->BSS);

	if (--LocalBoundShaderState.WorkArea->ComputedBSS->UseCount == 0)
	{
		LocalBoundShaderState.WorkArea->ComputedBSS->~FComputedBSS();
	}
}

void FRHICommandBuildLocalUniformBuffer::Execute()
{
	RHISTAT(BuildLocalUniformBuffer);
	check(!IsValidRef(WorkArea.ComputedUniformBuffer->UniformBuffer) && WorkArea.Layout && WorkArea.Contents); // should not already have been created
	if (WorkArea.ComputedUniformBuffer->UseCount)
	{
#if PLATFORM_SUPPORTS_RHI_THREAD
		WorkArea.ComputedUniformBuffer->UniformBuffer = RHICreateUniformBuffer(WorkArea.Contents, *WorkArea.Layout, UniformBuffer_SingleFrame); 
#else
		WorkArea.ComputedUniformBuffer->UniformBuffer = INTERNAL_DECORATOR(CreateUniformBuffer)(WorkArea.Contents, *WorkArea.Layout, UniformBuffer_SingleFrame);
#endif
	}
	WorkArea.Layout = nullptr;
	WorkArea.Contents = nullptr;
}

template <typename TShaderRHIParamRef>
void FRHICommandSetLocalUniformBuffer<TShaderRHIParamRef>::Execute()
{
	RHISTAT(SetLocalUniformBuffer);
	check(LocalUniformBuffer.WorkArea->ComputedUniformBuffer->UseCount > 0 && IsValidRef(LocalUniformBuffer.WorkArea->ComputedUniformBuffer->UniformBuffer)); // this should have been created and should have uses outstanding
	INTERNAL_DECORATOR(SetShaderUniformBuffer)(Shader, BaseIndex, LocalUniformBuffer.WorkArea->ComputedUniformBuffer->UniformBuffer);
	if (--LocalUniformBuffer.WorkArea->ComputedUniformBuffer->UseCount == 0)
	{
		LocalUniformBuffer.WorkArea->ComputedUniformBuffer->~FComputedUniformBuffer();
	}
}
template struct FRHICommandSetLocalUniformBuffer<FVertexShaderRHIParamRef>;
template struct FRHICommandSetLocalUniformBuffer<FHullShaderRHIParamRef>;
template struct FRHICommandSetLocalUniformBuffer<FDomainShaderRHIParamRef>;
template struct FRHICommandSetLocalUniformBuffer<FGeometryShaderRHIParamRef>;
template struct FRHICommandSetLocalUniformBuffer<FPixelShaderRHIParamRef>;
template struct FRHICommandSetLocalUniformBuffer<FComputeShaderRHIParamRef>;

void FRHICommandBeginRenderQuery::Execute()
{
	RHISTAT(BeginRenderQuery);
	INTERNAL_DECORATOR(BeginRenderQuery)(RenderQuery);
}

void FRHICommandEndRenderQuery::Execute()
{
	RHISTAT(EndRenderQuery);
	INTERNAL_DECORATOR(EndRenderQuery)(RenderQuery);
}

#if !PLATFORM_SUPPORTS_RHI_THREAD
void FRHICommandResetRenderQuery::Execute()
{
	RHISTAT(ResetRenderQuery);
	INTERNAL_DECORATOR(ResetRenderQuery)(RenderQuery);
}
#endif

void FRHICommandBeginScene::Execute()
{
	RHISTAT(BeginScene);
	INTERNAL_DECORATOR(BeginScene)();
}

void FRHICommandEndScene::Execute()
{
	RHISTAT(EndScene);
	INTERNAL_DECORATOR(EndScene)();
}

void FRHICommandUpdateVertexBuffer::Execute()
{
	RHISTAT(UpdateVertexBuffer);
	void* Data = INTERNAL_DECORATOR(LockVertexBuffer)(VertexBuffer, 0, BufferSize, RLM_WriteOnly);
	FMemory::Memcpy(Data, Buffer, BufferSize);
	INTERNAL_DECORATOR(UnlockVertexBuffer)(VertexBuffer);
	FMemory::Free((void*)Buffer);
}

void FRHICommandBeginFrame::Execute()
{
	RHISTAT(BeginFrame);
	INTERNAL_DECORATOR(BeginFrame)();
}

void FRHICommandEndFrame::Execute()
{
	RHISTAT(EndFrame);
	INTERNAL_DECORATOR(EndFrame)();
}

#if PLATFORM_SUPPORTS_RHI_THREAD
void FRHICommandBeginDrawingViewport::Execute()
{
	RHISTAT(BeginDrawingViewport);
	INTERNAL_DECORATOR(BeginDrawingViewport)(Viewport, RenderTargetRHI);
}

void FRHICommandEndDrawingViewport::Execute()
{
	RHISTAT(EndDrawingViewport);
	INTERNAL_DECORATOR(EndDrawingViewport)(Viewport, bPresent, bLockToVsync);
}
#endif

void FRHICommandPushEvent::Execute()
{
	RHISTAT(PushEvent);
	INTERNAL_DECORATOR(PushEvent)(Name);
}

void FRHICommandPopEvent::Execute()
{
	RHISTAT(PopEvent);
	INTERNAL_DECORATOR(PopEvent)();
}


