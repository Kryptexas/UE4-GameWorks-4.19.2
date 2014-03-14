// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessDeferredDecals.h: Deferred Decals implementation.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"

// ePId_Input0: SceneColor
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessDeferredDecals : public TRenderingCompositePassBase<1, 1>
{
public:
	// @param InRenderStage 0:before BasePass, 1:before lighting, 2:AfterLighting 
	FRCPassPostProcessDeferredDecals(uint32 InRenderStage);

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() OVERRIDE { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:
	// 0:before BasePass, 1:before lighting, (later we could add "after lighting" and multiply)
	uint32 RenderStage;
};

bool IsDBufferEnabled();