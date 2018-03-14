// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessTemporalAA.h: Post process MotionBlur implementation.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "RenderingCompositionGraph.h"


// ePId_Input0: Reflections (point)
// ePId_Input1: Previous frame's output (bilinear)
// ePId_Input2: Velocity (point)
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessSSRTemporalAA : public TRenderingCompositePassBase<3, 1>
{
public:
	FRCPassPostProcessSSRTemporalAA(const FTemporalAAHistory& InInputHistory, FTemporalAAHistory* OutOutputHistory)
		: InputHistory(InInputHistory), OutputHistory(OutOutputHistory)
	{ }

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

private:
	const FTemporalAAHistory& InputHistory;
	FTemporalAAHistory* OutputHistory;
};

// ePId_Input0: Half Res DOF input (point)
// ePId_Input1: Previous frame's output (bilinear)
// ePId_Input2: Velocity (point)
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessDOFTemporalAA : public TRenderingCompositePassBase<3, 1>
{
public:
	FRCPassPostProcessDOFTemporalAA(const FTemporalAAHistory& InInputHistory, FTemporalAAHistory* OutOutputHistory, bool bInIsComputePass)
		: InputHistory(InInputHistory), OutputHistory(OutOutputHistory)
	{
		bIsComputePass = bInIsComputePass;
		bPreferAsyncCompute = false;
		bPreferAsyncCompute &= (GNumActiveGPUsForRendering == 1); // Can't handle multi-frame updates on async pipe
	}

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

	virtual FComputeFenceRHIParamRef GetComputePassEndFence() const override { return AsyncEndFence; }

private:
	template <typename TRHICmdList>
	void DispatchCS(TRHICmdList& RHICmdList, FRenderingCompositePassContext& Context, const FIntRect& DestRect, FUnorderedAccessViewRHIParamRef DestUAV, FTextureRHIParamRef EyeAdaptationTex, int32 ScaleFactor);

	FComputeFenceRHIRef AsyncEndFence;

	const FTemporalAAHistory& InputHistory;
	FTemporalAAHistory* OutputHistory;
};

// ePId_Input0: Half Res DOF input (point)
// ePId_Input1: Previous frame's output (bilinear)
// ePId_Input2: Velocity (point)
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessDOFTemporalAANear : public TRenderingCompositePassBase<3, 1>
{
public:
	FRCPassPostProcessDOFTemporalAANear(const FTemporalAAHistory& InInputHistory, FTemporalAAHistory* OutOutputHistory)
		: InputHistory(InInputHistory), OutputHistory(OutOutputHistory)
	{ }

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

private:
	const FTemporalAAHistory& InputHistory;
	FTemporalAAHistory* OutputHistory;
};


// ePId_Input0: Half Res light shaft input (point)
// ePId_Input1: Previous frame's output (bilinear)
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessLightShaftTemporalAA : public TRenderingCompositePassBase<2, 1>
{
public:
	FRCPassPostProcessLightShaftTemporalAA(const FTemporalAAHistory& InInputHistory)
		: InputHistory(InInputHistory)
	{ }

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

private:
	const FTemporalAAHistory& InputHistory;
};

// ePId_Input0: Full Res Scene color (point)
// ePId_Input1: Previous frame's output (bilinear)
// ePId_Input2: Velocity (point)
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessTemporalAA : public TRenderingCompositePassBase<3, 1>
{
public:
	FRCPassPostProcessTemporalAA(
		const class FPostprocessContext& Context,
		const FTemporalAAHistory& InInputHistory,
		FTemporalAAHistory* OutOutputHistory,
		bool bInIsComputePass);

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

	virtual FComputeFenceRHIParamRef GetComputePassEndFence() const override { return AsyncEndFence; }

private:
	FIntPoint OutputExtent;

	template <typename TRHICmdList>
	void DispatchCS(TRHICmdList& RHICmdList, FRenderingCompositePassContext& Context, const FIntRect& DestRect, FUnorderedAccessViewRHIParamRef DestUAV, const bool bUseFast, const bool bUseDither, FTextureRHIParamRef EyeAdaptationTex);

	FComputeFenceRHIRef AsyncEndFence;

	const FTemporalAAHistory& InputHistory;
	FTemporalAAHistory* OutputHistory;
};
