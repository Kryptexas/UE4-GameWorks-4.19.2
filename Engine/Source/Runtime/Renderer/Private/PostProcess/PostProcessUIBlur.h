// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessUIBlur.h: Post processing UIBlur implementation.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: fullres scene color 
// ePId_Input1: low resolution blurry scene color in LDR
class FRCPassPostProcessUIBlur : public TRenderingCompositePassBase<2, 1>
{
public:
	FRCPassPostProcessUIBlur(FRenderingCompositeOutput& InBlurOutputRef)
		: BlurOutputRef(InBlurOutputRef)
	{
	}

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() { delete this; }
	virtual bool FrameBufferBlendingWithInput0() const { return true; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:
	FRenderingCompositeOutput& BlurOutputRef;
};

