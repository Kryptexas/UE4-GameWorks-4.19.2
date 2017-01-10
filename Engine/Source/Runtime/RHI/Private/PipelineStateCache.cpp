// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
PipelineStateCache.cpp: Pipeline state cache implementation.
=============================================================================*/

#include "PipelineStateCache.h"
#include "Misc/ScopeLock.h"

static FCriticalSection GComputeLock;
static TMap<FRHIComputeShader*, TRefCountPtr<FRHIComputePipelineState>> GComputePipelines;

TRefCountPtr<FRHIComputePipelineState> GetComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader)
{
	FScopeLock ScopeLock(&GComputeLock);

	TRefCountPtr<FRHIComputePipelineState>* Found = GComputePipelines.Find(ComputeShader);
	if (Found)
	{
		return *Found;
	}

	FRHIComputePipelineState* PipelineState = RHICreateComputePipelineState(ComputeShader);
	GComputePipelines.Add(ComputeShader, PipelineState);
	return PipelineState;
}
