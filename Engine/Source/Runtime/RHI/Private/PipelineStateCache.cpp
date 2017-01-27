// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
PipelineStateCache.cpp: Pipeline state cache implementation.
=============================================================================*/

#include "PipelineStateCache.h"
#include "Misc/ScopeLock.h"

extern RHI_API FRHIComputePipelineState* ExecuteSetComputePipelineState(FComputePipelineState* ComputePipelineState);
extern RHI_API FRHIGraphicsPipelineState* ExecuteSetGraphicsPipelineState(FGraphicsPipelineState* GraphicsPipelineState);

static TAutoConsoleVariable<int32> GCVarAsyncPipelineCompile(
	TEXT("r.AsyncPipelineCompile"),
	1,
	TEXT("0 to Create PSOs at the moment they are requested\n")\
	TEXT("1 to Create Pipeline State Objects asynchronously(default)"),
	ECVF_ReadOnly | ECVF_RenderThreadSafe
);

static inline uint32 GetTypeHash(const FBoundShaderStateInput& Input)
{
	return GetTypeHash(Input.VertexDeclarationRHI) ^
		GetTypeHash(Input.VertexShaderRHI) ^
		GetTypeHash(Input.PixelShaderRHI) ^
		GetTypeHash(Input.HullShaderRHI) ^
		GetTypeHash(Input.DomainShaderRHI) ^
		GetTypeHash(Input.GeometryShaderRHI);
}

static inline uint32 GetTypeHash(const FGraphicsPipelineStateInitializer& Initializer)
{
	//#todo-rco: Hash!
	return (GetTypeHash(Initializer.BoundShaderState) | (Initializer.NumSamples << 28)) ^ ((uint32)Initializer.PrimitiveType << 24) ^ GetTypeHash(Initializer.BlendState)
		^ Initializer.RenderTargetsEnabled ^ GetTypeHash(Initializer.RasterizerState) ^ GetTypeHash(Initializer.DepthStencilState);
/*
	uint32							RenderTargetsEnabled;
	TRenderTargetFormats			RenderTargetFormats;
	TRenderTargetFlags				RenderTargetFlags;
	TRenderTargetLoadActions		RenderTargetLoadActions;
	TRenderTargetStoreActions		RenderTargetStoreActions;
	EPixelFormat					DepthStencilTargetFormat;
	uint32							DepthStencilTargetFlag;
	ERenderTargetLoadAction			DepthStencilTargetLoadAction;
	ERenderTargetStoreAction		DepthStencilTargetStoreAction;
*/
}


static FCriticalSection GComputeLock;
static TMap<FRHIComputeShader*, FComputePipelineState*> GComputePipelines;
static FCriticalSection GGraphicsLock;
static TMap <FGraphicsPipelineStateInitializer, FGraphicsPipelineState*> GGraphicsPipelines;

class FPipelineState
{
public:
	virtual bool IsCompute() const = 0;
	FGraphEventRef CompletionEvent;
};

class FComputePipelineState : public FPipelineState
{
public:
	FComputePipelineState(FRHIComputeShader* InComputeShader)
		: ComputeShader(InComputeShader)
	{
	}

	virtual bool IsCompute() const
	{
		return true;
	}

//protected:
	FRHIComputeShader* ComputeShader;
	TRefCountPtr<FRHIComputePipelineState> RHIPipeline;
};


class FGraphicsPipelineState : public FPipelineState
{
public:
	FGraphicsPipelineState(const FGraphicsPipelineStateInitializer& InInitializer)
		: Initializer(InInitializer)
	{
	}

	virtual bool IsCompute() const
	{
		return false;
	}

	//protected:
	FGraphicsPipelineStateInitializer Initializer;
	TRefCountPtr<FRHIGraphicsPipelineState> RHIPipeline;
};

class FCompilePipelineStateTask
{
public:
	FPipelineState* Pipeline;

	FCompilePipelineStateTask(FPipelineState* InPipeline)
		: Pipeline(InPipeline)
	{
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		if (Pipeline->IsCompute())
		{
			FComputePipelineState* ComputePipeline = static_cast<FComputePipelineState*>(Pipeline);
			ComputePipeline->RHIPipeline = RHICreateComputePipelineState(ComputePipeline->ComputeShader);
		}
		else
		{
			FGraphicsPipelineState* GfxPipeline = static_cast<FGraphicsPipelineState*>(Pipeline);
			GfxPipeline->RHIPipeline = RHICreateGraphicsPipelineState(GfxPipeline->Initializer);
		}
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCompilePipelineStateTask, STATGROUP_TaskGraphTasks);
	}

	ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}
};

FComputePipelineState* GetOrCreateComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader)
{
	SCOPE_CYCLE_COUNTER(STAT_GetOrCreatePSO);

	{
		// Should be hitting this case more often once the cache is hot
		FScopeLock ScopeLock(&GComputeLock);

		FComputePipelineState** Found = GComputePipelines.Find(ComputeShader);
		if (Found)
		{
			RHICmdList.QueueAsyncPipelineStateCompile((*Found)->CompletionEvent);
			return *Found;
		}

		FComputePipelineState* PipelineState = new FComputePipelineState(ComputeShader);

		if (GCVarAsyncPipelineCompile.GetValueOnAnyThread() && !RHICmdList.Bypass())
		{
			PipelineState->CompletionEvent = TGraphTask<FCompilePipelineStateTask>::CreateTask().ConstructAndDispatchWhenReady(PipelineState);
			RHICmdList.QueueAsyncPipelineStateCompile(PipelineState->CompletionEvent);
		}
		else
		{
			PipelineState->RHIPipeline = RHICreateComputePipelineState(PipelineState->ComputeShader);
		}

		GComputePipelines.Add(ComputeShader, PipelineState);

		return PipelineState;
	}

	return nullptr;
}

FRHIComputePipelineState* ExecuteSetComputePipelineState(FComputePipelineState* ComputePipelineState)
{
	ensure(ComputePipelineState->RHIPipeline);
	ComputePipelineState->CompletionEvent = nullptr;
	return ComputePipelineState->RHIPipeline;
}

FGraphicsPipelineState* GetOrCreateGraphicsPipelineState(FRHICommandList& RHICmdList, const FGraphicsPipelineStateInitializer& Initializer)
{
	SCOPE_CYCLE_COUNTER(STAT_GetOrCreatePSO);

	{
		// Should be hitting this case more often once the cache is hot
		FScopeLock ScopeLock(&GGraphicsLock);

		FGraphicsPipelineState** Found = GGraphicsPipelines.Find(Initializer);
		if (Found)
		{
			RHICmdList.QueueAsyncPipelineStateCompile((*Found)->CompletionEvent);
			return *Found;
		}

		FGraphicsPipelineState* PipelineState = new FGraphicsPipelineState(Initializer);

		if (GCVarAsyncPipelineCompile.GetValueOnAnyThread() && !RHICmdList.Bypass())
		{
			PipelineState->CompletionEvent = TGraphTask<FCompilePipelineStateTask>::CreateTask().ConstructAndDispatchWhenReady(PipelineState);
			RHICmdList.QueueAsyncPipelineStateCompile(PipelineState->CompletionEvent);
		}
		else
		{
			PipelineState->RHIPipeline = RHICreateGraphicsPipelineState(Initializer);
		}

		GGraphicsPipelines.Add(Initializer, PipelineState);

		return PipelineState;
	}

	return nullptr;
}

FRHIGraphicsPipelineState* ExecuteSetGraphicsPipelineState(FGraphicsPipelineState* GraphicsPipelineState)
{
	ensure(GraphicsPipelineState->RHIPipeline);
	GraphicsPipelineState->CompletionEvent = nullptr;
	return GraphicsPipelineState->RHIPipeline;
}
