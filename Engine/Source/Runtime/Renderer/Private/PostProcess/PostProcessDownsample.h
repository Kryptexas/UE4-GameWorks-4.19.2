// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessDownsample.h: Post processing down sample implementation.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"
#include "PostProcessing.h"

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: Color input
// ePId_Input1: optional depth input
class FRCPassPostProcessDownsample : public TRenderingCompositePassBase<2, 1>
{
public:
	// constructor
	// @param InDebugName we store the pointer so don't release this string
	// @param Quality 0:1 sample, 1:4 samples with blurring
	// @param InRectSource used to select rectangle source, either viewrect (typical case) or UIBlur if pass is a component of UIBlur effect.
	FRCPassPostProcessDownsample(EPixelFormat InOverrideFormat = PF_Unknown,
			uint32 InQuality = 1,
			EPostProcessRectSource::Type InRectSource = EPostProcessRectSource::GBS_ViewRect,
			const TCHAR *InDebugName = TEXT("Downsample"));

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() OVERRIDE { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:
	template <uint32 Method> static void SetShader(const FRenderingCompositePassContext& Context);

	EPixelFormat OverrideFormat;
	// explained in constructor
	uint32 Quality;
	// must be a valid pointer
	const TCHAR* DebugName;
	EPostProcessRectSource::Type RectSource;
	bool IsDepthInputAvailable() const;
};
