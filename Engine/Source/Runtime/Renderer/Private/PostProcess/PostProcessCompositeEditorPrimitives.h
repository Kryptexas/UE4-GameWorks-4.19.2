// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "RenderingCompositionGraph.h"

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: SceneColor
class FRCPassPostProcessCompositeEditorPrimitives : public TRenderingCompositePassBase<1, 1>
{
public:
	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() OVERRIDE { delete this; }
	FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:
	/**
	 * Draws primitives that need to be composited
	 *
	 * @param View	The view to draw in
	 */
	void RenderPrimitivesToComposite(const FViewInfo& View);
};

