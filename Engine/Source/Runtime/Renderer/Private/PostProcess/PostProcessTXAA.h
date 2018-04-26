// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessTXAA.h: Post process MotionBlur implementation.
=============================================================================*/

#pragma once

#if WITH_TXAA

#include "RenderingCompositionGraph.h"

// ePId_Input0: Full Res Scene color (point)
// ePId_Input1: Previous frame's output (bilinear)
// ePId_Input2: Velocity (point)
// ePID_Input3: Depth (point)
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessTXAA : public TRenderingCompositePassBase<4, 1>
{
public:
	FRCPassPostProcessTXAA(
		const FTemporalAAHistory& InInputHistory,
		FTemporalAAHistory* OutOutputHistory);

    // interface FRenderingCompositePass ---------
    virtual void Process(FRenderingCompositePassContext& Context) override;
    virtual void Release() override { delete this; }
    virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

private:
	const FTemporalAAHistory& InputHistory;
	FTemporalAAHistory* OutputHistory;
};

// ePId_Input0: Velocity (point)
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessComputeMotionVector : public TRenderingCompositePassBase<1, 1>
{
public:
    // interface FRenderingCompositePass ---------
    virtual void Process(FRenderingCompositePassContext& Context) override;
    virtual void Release() override { delete this; }
    virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;
};


#endif