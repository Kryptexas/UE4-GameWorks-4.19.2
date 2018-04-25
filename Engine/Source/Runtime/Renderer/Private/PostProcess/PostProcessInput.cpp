// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessAA.cpp: Post processing input implementation.
=============================================================================*/

#include "PostProcess/PostProcessInput.h"

FRCPassPostProcessInput::FRCPassPostProcessInput(const TRefCountPtr<IPooledRenderTarget>& InData)
	: Data(InData)
{
	check(Data);
}

void FRCPassPostProcessInput::Process(FRenderingCompositePassContext& Context)
{
	PassOutputs[0].PooledRenderTarget = Data;
}

FPooledRenderTargetDesc FRCPassPostProcessInput::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	check(Data);

	FPooledRenderTargetDesc Ret = Data->GetDesc();

	return Ret;
}

void FRCPassPostProcessInputSingleUse::Process(FRenderingCompositePassContext& Context)
{
	PassOutputs[0].PooledRenderTarget = Data;

	// to get the RT freed up earlier than when the graph is cleaned up
	Data.SafeRelease();
}
