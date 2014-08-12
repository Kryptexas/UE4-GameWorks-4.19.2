// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessMaterial.h: Post processing Material
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: SceneColor
// ePId_Input1: SeparateTranslucency
class FRCPassPostProcessMaterial : public TRenderingCompositePassBase<2,1>
{
public:
	// constructor
	FRCPassPostProcessMaterial(UMaterialInterface* InMaterialInterface, EPixelFormat OutputFormatIN = PF_Unknown);

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:
	// PF_Unknown for default behavior.
	EPixelFormat OutputFormat;
	void SetShader(const FRenderingCompositePassContext& Context);

	UMaterialInterface* MaterialInterface;
};
