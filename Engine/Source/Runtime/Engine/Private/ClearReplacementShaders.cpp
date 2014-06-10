// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "ClearReplacementShaders.h"
#include "ShaderParameterUtils.h"

IMPLEMENT_SHADER_TYPE(,FClearReplacementVS,TEXT("ClearReplacementShaders"),TEXT("ClearVS"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FClearReplacementPS,TEXT("ClearReplacementShaders"),TEXT("ClearPS"),SF_Pixel);

IMPLEMENT_SHADER_TYPE(,FClearTexture2DReplacementCS,TEXT("ClearReplacementShaders"),TEXT("ClearTexture2DCS"),SF_Compute);

IMPLEMENT_SHADER_TYPE(,FClearBufferReplacementCS,TEXT("ClearReplacementShaders"),TEXT("ClearBufferCS"),SF_Compute);

void FClearTexture2DReplacementCS::SetParameters( FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef TextureRW, FLinearColor Value )
{
	RHICmdList.CheckIsNull();
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
	SetShaderValue(RHICmdList, ComputeShaderRHI, ClearColor, Value);
	RHISetUAVParameter(ComputeShaderRHI, ClearTextureRW.GetBaseIndex(), TextureRW);
}

void FClearBufferReplacementCS::SetParameters( FRHICommandList& RHICmdList, FUnorderedAccessViewRHIParamRef BufferRW, uint32 Dword )
{
	RHICmdList.CheckIsNull();
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
	SetShaderValue(RHICmdList, ComputeShaderRHI, ClearDword, Dword);
	RHISetUAVParameter(ComputeShaderRHI, ClearBufferRW.GetBaseIndex(), BufferRW);
}
