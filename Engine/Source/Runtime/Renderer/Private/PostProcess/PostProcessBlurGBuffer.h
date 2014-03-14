// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessBlurGBuffer.h: Post processing tone mapping implementation, can add bloom.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"

// no input
// derives from TRenderingCompositePassBase<InputCount, OutputCount>
class FRCPassPostProcessBlurGBuffer: public TRenderingCompositePassBase<0, 1>
{
public:	
	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() OVERRIDE { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

	static void SetShader(const FRenderingCompositePassContext& Context, uint32 BlurType);
};
