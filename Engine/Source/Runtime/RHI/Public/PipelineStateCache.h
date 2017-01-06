// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
PipelineStateCache.h: Pipeline state cache definition.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"

extern RHI_API TRefCountPtr<FRHIComputePipelineState> GetComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader);
