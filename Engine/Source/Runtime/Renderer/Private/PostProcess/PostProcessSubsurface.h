// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessSubsurface.h: Screenspace subsurface scattering implementation
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"


// ePId_Input0: SceneColor
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
// uses some GBuffer attributes
// alpha is unused
class FRCPassPostProcessSubsurfaceSetup : public TRenderingCompositePassBase<1, 1>
{
public:
	// @param bInHalfRes if the output shoudl be half resolution to scene color
	FRCPassPostProcessSubsurfaceSetup(bool bInVisualize, bool bInHalfRes = false);

	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
	virtual void Release() override { delete this; }

	bool bVisualize;
	// if the output shoudl be half resolution to scene color
	bool bHalfRes;
};


// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: SceneColor (horizontal blur) or the pass before (vertical blur)
// modifies SceneColor, uses some GBuffer attributes
class FRCPassPostProcessSubsurface : public TRenderingCompositePassBase<1, 1>
{
public:
	// constructor
	// @param InDirection 0:horizontal/1:vertical
	// @param InRadius in pixels in the full res image
	FRCPassPostProcessSubsurface(uint32 InDirection, float InRadius, bool bInHalfRes = false);

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:
	// in pixels in the full res image
	float Radius;
	// 0:horizontal/1:vertical
	uint32 Direction;
	// if the output should be half resolution to scene color
	bool bHalfRes;
};


// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: output from FRCPassPostProcessSubsurface
// ePId_Input1: SceneColor before Screen Space Subsurface input
// modifies SceneColor, uses some GBuffer attributes
class FRCPassPostProcessSubsurfaceRecombine : public TRenderingCompositePassBase<2, 1>
{
public:
	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;
};