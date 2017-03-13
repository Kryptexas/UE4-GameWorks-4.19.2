// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
PipelineStateCache.h: Pipeline state cache definition.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"

class FComputePipelineState;
class FGraphicsPipelineState;

extern RHI_API FComputePipelineState* GetOrCreateComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader);
extern RHI_API FGraphicsPipelineState* GetOrCreateGraphicsPipelineState(FRHICommandList& RHICmdList, const FGraphicsPipelineStateInitializer& Initializer);
