// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessAA.h: Post processing input implementation.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "PostProcess/RenderingCompositionGraph.h"

// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessInput : public TRenderingCompositePassBase<0, 1>
{
public:
	// constructor
	FRCPassPostProcessInput(const TRefCountPtr<IPooledRenderTarget>& InData);

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

protected:

	TRefCountPtr<IPooledRenderTarget> Data;
};

//
class FRCPassPostProcessInputSingleUse : public FRCPassPostProcessInput
{
public:
	// constructor
	FRCPassPostProcessInputSingleUse(TRefCountPtr<IPooledRenderTarget>& InData)
		: FRCPassPostProcessInput(InData)
	{
	}

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
};
