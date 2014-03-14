// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessUpscale.h: Post processing Upscale implementation.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: SceneColor (bilinear)
// ePId_Input1: SceneColor (point)
class FRCPassPostProcessUpscale : public TRenderingCompositePassBase<2, 1>
{
public:
	// constructor
	// @param InUpscaleMethod - value denoting Upscale method to use:
	//				0: Nearest
	//				1: Bilinear
	//				2: 4 tap Bilinear (with radius adjustment)
	//				3: Directional blur with unsharp mask upsample.
	FRCPassPostProcessUpscale(uint32 InUpscaleMethod);

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() OVERRIDE { delete this; }
	FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
private:
	template <uint32 Method> static void SetShader(const FRenderingCompositePassContext& Context);
	uint32 UpscaleMethod;
};

