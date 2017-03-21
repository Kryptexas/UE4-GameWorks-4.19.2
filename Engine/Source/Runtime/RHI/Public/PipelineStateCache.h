// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
PipelineStateCache.h: Pipeline state cache definition.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "EnumClassFlags.h"

enum class EApplyRendertargetOption : int
{
	DoNothing  = 0,
	ForceApply = 1 << 0,
	CheckApply = 1 << 1,
};

ENUM_CLASS_FLAGS(EApplyRendertargetOption);

extern RHI_API void SetComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader);
extern RHI_API void SetGraphicsPipelineState(FRHICommandList& RHICmdList, const FGraphicsPipelineStateInitializer& Initializer, EApplyRendertargetOption ApplyFlags = EApplyRendertargetOption::CheckApply);