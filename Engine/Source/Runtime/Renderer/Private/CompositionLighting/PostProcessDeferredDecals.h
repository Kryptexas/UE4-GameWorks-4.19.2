// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessDeferredDecals.h: Deferred Decals implementation.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"


// Actual values are used in the shader so do not change
enum EDecalRenderStage
{
	// for DBuffer decals (get proper baked lighting)
	DRS_BeforeBasePass = 0,
	// for volumetrics to update the depth buffer
	DRS_AfterBasePass = 1,
	// for normal decals not modifying the depth buffer
	DRS_BeforeLighting = 2,

	// later we could add "after lighting" and multiply
};

// ePId_Input0: SceneColor (not needed for DBuffer decals)
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessDeferredDecals : public TRenderingCompositePassBase<1, 1>
{
public:
	// One instance for each render stage
	FRCPassPostProcessDeferredDecals(EDecalRenderStage InDecalRenderStage);

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

private:
	// see EDecalRenderStage
	EDecalRenderStage DecalRenderStage;
};

bool IsDBufferEnabled();
