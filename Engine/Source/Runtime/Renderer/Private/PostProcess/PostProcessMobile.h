// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessMobile.h: Mobile uber post processing.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"

class FRCPassPostProcessBloomSetupES2 : public TRenderingCompositePassBase<1, 1>
{
public:
	FRCPassPostProcessBloomSetupES2(FIntPoint InPrePostSourceViewportSize, bool bInUsedFramebufferFetch) : PrePostSourceViewportSize(InPrePostSourceViewportSize), bUsedFramebufferFetch(bInUsedFramebufferFetch) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() OVERRIDE { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	bool bUsedFramebufferFetch;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessBloomSetupSmallES2 : public TRenderingCompositePassBase<1, 1>
{
public:
	FRCPassPostProcessBloomSetupSmallES2(FIntPoint InPrePostSourceViewportSize, bool bInUsedFramebufferFetch) : PrePostSourceViewportSize(InPrePostSourceViewportSize), bUsedFramebufferFetch(bInUsedFramebufferFetch) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() OVERRIDE { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	bool bUsedFramebufferFetch;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessDofNearES2 : public TRenderingCompositePassBase<1, 1>
{
public:
	FRCPassPostProcessDofNearES2(FIntPoint InPrePostSourceViewportSize) : PrePostSourceViewportSize(InPrePostSourceViewportSize) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessDofDownES2 : public TRenderingCompositePassBase<2, 1>
{
public:
	FRCPassPostProcessDofDownES2(FIntPoint InPrePostSourceViewportSize, bool bInUsedFramebufferFetch) : PrePostSourceViewportSize(InPrePostSourceViewportSize), bUsedFramebufferFetch(bInUsedFramebufferFetch) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	bool bUsedFramebufferFetch;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessDofBlurES2 : public TRenderingCompositePassBase<2, 1>
{
public:
	FRCPassPostProcessDofBlurES2(FIntPoint InPrePostSourceViewportSize) : PrePostSourceViewportSize(InPrePostSourceViewportSize) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
};

class FRCPassPostProcessBloomDownES2 : public TRenderingCompositePassBase<1, 1>
{
public:
	FRCPassPostProcessBloomDownES2(FIntPoint InPrePostSourceViewportSize, float InScale) : PrePostSourceViewportSize(InPrePostSourceViewportSize), Scale(InScale) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	float Scale;
};

class FRCPassPostProcessBloomUpES2 : public TRenderingCompositePassBase<2, 1>
{
public:
	FRCPassPostProcessBloomUpES2(FIntPoint InPrePostSourceViewportSize, FVector2D InScaleAB, FVector4& InTintA, FVector4& InTintB) : PrePostSourceViewportSize(InPrePostSourceViewportSize), ScaleAB(InScaleAB), TintA(InTintA), TintB(InTintB) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	FVector2D ScaleAB;
	FVector4 TintA;
	FVector4 TintB;
};

class FRCPassPostProcessSunMaskES2 : public TRenderingCompositePassBase<1, 1>
{
public:
	FRCPassPostProcessSunMaskES2(FIntPoint InPrePostSourceViewportSize, bool bInOnChip) : PrePostSourceViewportSize(InPrePostSourceViewportSize), bOnChip(bInOnChip) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	bool bOnChip;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessSunAlphaES2 : public TRenderingCompositePassBase<1, 1>
{
public:
	FRCPassPostProcessSunAlphaES2(FIntPoint InPrePostSourceViewportSize) : PrePostSourceViewportSize(InPrePostSourceViewportSize) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessSunBlurES2 : public TRenderingCompositePassBase<1, 1>
{
public:
	FRCPassPostProcessSunBlurES2(FIntPoint InPrePostSourceViewportSize) : PrePostSourceViewportSize(InPrePostSourceViewportSize) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
};

class FRCPassPostProcessSunMergeES2 : public TRenderingCompositePassBase<3, 1>
{
public:
	FRCPassPostProcessSunMergeES2(FIntPoint InPrePostSourceViewportSize) : PrePostSourceViewportSize(InPrePostSourceViewportSize) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessSunMergeSmallES2 : public TRenderingCompositePassBase<2, 1>
{
public:
	FRCPassPostProcessSunMergeSmallES2(FIntPoint InPrePostSourceViewportSize) : PrePostSourceViewportSize(InPrePostSourceViewportSize) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessSunAvgES2 : public TRenderingCompositePassBase<2, 1>
{
public:
	FRCPassPostProcessSunAvgES2(FIntPoint InPrePostSourceViewportSize) : PrePostSourceViewportSize(InPrePostSourceViewportSize) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	void SetShader(const FRenderingCompositePassContext& Context);
};

class FRCPassPostProcessAaES2 : public TRenderingCompositePassBase<2, 1>
{
public:
	FRCPassPostProcessAaES2(FIntPoint InPrePostSourceViewportSize) : PrePostSourceViewportSize(InPrePostSourceViewportSize) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() { delete this; }
private:
	FIntPoint PrePostSourceViewportSize;
	void SetShader(const FRenderingCompositePassContext& Context);
};

