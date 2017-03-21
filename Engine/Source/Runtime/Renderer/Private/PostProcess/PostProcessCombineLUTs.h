// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessCombineLUTs.h: Post processing tone mapping implementation, can add bloom.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "ShaderParameters.h"
#include "PostProcess/RenderingCompositionGraph.h"

class UTexture;
class FFinalPostProcessSettings;

bool UseVolumeTextureLUT(EShaderPlatform Platform);

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
class FRCPassPostProcessCombineLUTs : public TRenderingCompositePassBase<0, 1>
{
public:
	// interface FRenderingCompositePass ---------
	FRCPassPostProcessCombineLUTs(EShaderPlatform InShaderPlatform, bool bInAllocateOutput)
	: ShaderPlatform(InShaderPlatform)
	, bAllocateOutput(bInAllocateOutput)
	{
		
	}

	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

	/** */
	uint32 GenerateFinalTable(const FFinalPostProcessSettings& Settings, FTexture* OutTextures[], float OutWeights[], uint32 MaxCount) const;
	/** @return 0xffffffff if not found */
	uint32 FindIndex(const FFinalPostProcessSettings& Settings, UTexture* Tex) const;

private:

	EShaderPlatform ShaderPlatform;
	bool bAllocateOutput;
};



/*-----------------------------------------------------------------------------
FColorRemapShaderParameters
-----------------------------------------------------------------------------*/

/** Encapsulates the color remap parameters. */
class FColorRemapShaderParameters
{
public:
	FColorRemapShaderParameters() {}

	FColorRemapShaderParameters(const FShaderParameterMap& ParameterMap);

	void Set(FRHICommandList& RHICmdList, const FPixelShaderRHIParamRef ShaderRHI);

	friend FArchive& operator<<(FArchive& Ar,FColorRemapShaderParameters& P);

	FShaderParameter MappingPolynomial;
};
