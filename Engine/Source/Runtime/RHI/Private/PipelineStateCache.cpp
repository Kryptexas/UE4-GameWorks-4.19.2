// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
PipelineStateCache.cpp: Pipeline state cache implementation.
=============================================================================*/

#include "PipelineStateCache.h"
#include "Misc/ScopeRWLock.h"

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


static FRWLock GComputeLock;
static TMap<FRHIComputeShader*, FComputePipelineState*> GComputePipelines;
static FRWLock GGraphicsLock;
static TMap <FGraphicsPipelineStateInitializer, FGraphicsPipelineState*> GGraphicsPipelines;

class FPipelineState
{
public:
	virtual ~FPipelineState() { }
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

static bool IsAsyncCompilationAllowed(FRHICommandList& RHICmdList)
{
	return GCVarAsyncPipelineCompile.GetValueOnAnyThread() && !RHICmdList.Bypass() && IsRunningRHIInSeparateThread();
}

FComputePipelineState* GetAndOrCreateComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader)
{
	SCOPE_CYCLE_COUNTER(STAT_GetOrCreatePSO);

	{
		// Should be hitting this case more often once the cache is hot
		FRWScopeLock ScopeLock(GComputeLock, SLT_ReadOnly);

		FComputePipelineState** Found = GComputePipelines.Find(ComputeShader);
		if (Found)
		{
			if (IsAsyncCompilationAllowed(RHICmdList))
			{
				FGraphEventRef& CompletionEvent = (*Found)->CompletionEvent;
				RHICmdList.QueueAsyncPipelineStateCompile(CompletionEvent);
			}
			return *Found;
		}
		
		ScopeLock.RaiseLockToWrite();
		Found = GComputePipelines.Find(ComputeShader);
		if (Found)
		{
			if (IsAsyncCompilationAllowed(RHICmdList))
			{
				FGraphEventRef& CompletionEvent = (*Found)->CompletionEvent;
				RHICmdList.QueueAsyncPipelineStateCompile(CompletionEvent);
			}
			return *Found;
		}
		else
		{
			FComputePipelineState* PipelineState = new FComputePipelineState(ComputeShader);

			if (IsAsyncCompilationAllowed(RHICmdList))
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
	}

	return nullptr;
}

FRHIComputePipelineState* ExecuteSetComputePipelineState(FComputePipelineState* ComputePipelineState)
{
	ensure(ComputePipelineState->RHIPipeline);
	ComputePipelineState->CompletionEvent = nullptr;
	return ComputePipelineState->RHIPipeline;
}

FGraphicsPipelineState* GetAndOrCreateGraphicsPipelineState(FRHICommandList& RHICmdList, const FGraphicsPipelineStateInitializer& OriginalInitializer, EApplyRendertargetOption ApplyFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_GetOrCreatePSO);
	FGraphicsPipelineStateInitializer NewInitializer;
	const FGraphicsPipelineStateInitializer* Initializer = &OriginalInitializer;

	if (!!(ApplyFlags & EApplyRendertargetOption::ForceApply))
	{
		// Copy original initializer first, then apply the render targets
		NewInitializer = OriginalInitializer;
		RHICmdList.ApplyCachedRenderTargets(NewInitializer);
		Initializer = &NewInitializer;
	}
	
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	else if(!!(ApplyFlags & EApplyRendertargetOption::CheckApply))
	{
		// Catch cases where the state does not match
		NewInitializer = OriginalInitializer;
		RHICmdList.ApplyCachedRenderTargets(NewInitializer);

		int AnyFailed = 0;
		AnyFailed |= (NewInitializer.RenderTargetsEnabled != OriginalInitializer.RenderTargetsEnabled)			<< 0;

		if (AnyFailed == 0)
		{
			for (int i = 0; i < (int)NewInitializer.RenderTargetsEnabled; i++)
			{
				AnyFailed |= (NewInitializer.RenderTargetFormats[i] != OriginalInitializer.RenderTargetFormats[i])				<< 1;
				AnyFailed |= (NewInitializer.RenderTargetFlags[i] != OriginalInitializer.RenderTargetFlags[i])					<< 2;
				AnyFailed |= (NewInitializer.RenderTargetLoadActions[i] != OriginalInitializer.RenderTargetLoadActions[i])		<< 3;
				AnyFailed |= (NewInitializer.RenderTargetStoreActions[i] != OriginalInitializer.RenderTargetStoreActions[i])	<< 4;

				if (AnyFailed)
				{
					AnyFailed |= i << 24;
					break;
				}
			}
		}

		AnyFailed |= (NewInitializer.DepthStencilTargetFormat != OriginalInitializer.DepthStencilTargetFormat)	<< 5;
		AnyFailed |= (NewInitializer.DepthStencilTargetFlag != OriginalInitializer.DepthStencilTargetFlag)		<< 6;
		AnyFailed |= (NewInitializer.DepthTargetLoadAction != OriginalInitializer.DepthTargetLoadAction)		<< 7;
		AnyFailed |= (NewInitializer.DepthTargetStoreAction != OriginalInitializer.DepthTargetStoreAction)		<< 8;
		AnyFailed |= (NewInitializer.StencilTargetLoadAction != OriginalInitializer.StencilTargetLoadAction)	<< 9;
		AnyFailed |= (NewInitializer.StencilTargetStoreAction != OriginalInitializer.StencilTargetStoreAction)	<< 10;

		ensureMsgf(AnyFailed == 0, TEXT("GetAndOrCreateGraphicsPipelineState RenderTarget check failed with: %i !"), AnyFailed);
		Initializer = (AnyFailed != 0) ? &NewInitializer : &OriginalInitializer;
	}
#endif

	{
		// Should be hitting this case more often once the cache is hot
		FRWScopeLock ScopeLock(GGraphicsLock, SLT_ReadOnly);

		FGraphicsPipelineState** Found = GGraphicsPipelines.Find(*Initializer);
		if (Found)
		{
			if (IsAsyncCompilationAllowed(RHICmdList))
			{
				FGraphEventRef& CompletionEvent = (*Found)->CompletionEvent;
				RHICmdList.QueueAsyncPipelineStateCompile(CompletionEvent);
			}
			return *Found;
		}

		ScopeLock.RaiseLockToWrite();
		Found = GGraphicsPipelines.Find(*Initializer);
		if (Found)
		{
			if (IsAsyncCompilationAllowed(RHICmdList))
			{
				FGraphEventRef& CompletionEvent = (*Found)->CompletionEvent;
				RHICmdList.QueueAsyncPipelineStateCompile(CompletionEvent);
			}
			return *Found;
		}
		else
		{
			FGraphicsPipelineState* PipelineState = new FGraphicsPipelineState(*Initializer);

			if (IsAsyncCompilationAllowed(RHICmdList))
			{
				PipelineState->CompletionEvent = TGraphTask<FCompilePipelineStateTask>::CreateTask().ConstructAndDispatchWhenReady(PipelineState);
				RHICmdList.QueueAsyncPipelineStateCompile(PipelineState->CompletionEvent);
			}
			else
			{
				PipelineState->RHIPipeline = RHICreateGraphicsPipelineState(*Initializer);
			}

			GGraphicsPipelines.Add(*Initializer, PipelineState);
			return PipelineState;
		}
	}

	return nullptr;
}

FRHIGraphicsPipelineState* ExecuteSetGraphicsPipelineState(FGraphicsPipelineState* GraphicsPipelineState)
{
	ensure(GraphicsPipelineState->RHIPipeline);
	GraphicsPipelineState->CompletionEvent = nullptr;
	return GraphicsPipelineState->RHIPipeline;
}

void SetComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader)
{
	RHICmdList.SetComputePipelineState(GetAndOrCreateComputePipelineState(RHICmdList, ComputeShader));
}

void SetGraphicsPipelineState(FRHICommandList& RHICmdList, const FGraphicsPipelineStateInitializer& Initializer, EApplyRendertargetOption ApplyFlags)
{
	RHICmdList.SetGraphicsPipelineState(GetAndOrCreateGraphicsPipelineState(RHICmdList, Initializer, ApplyFlags));
}
