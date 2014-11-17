// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CompositionLighting.h: The center for all deferred lighting activities.
=============================================================================*/

#pragma once

#include "PostProcess/RenderingCompositionGraph.h"

/**
 * The center for all screen space processing activities (e.g. G-buffer manipulation, lighting).
 */
class FCompositionLighting
{
public:
	void ProcessBeforeBasePass(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);

	void ProcessAfterBasePass(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);

	void ProcessLighting(FRHICommandListImmediate& RHICmdList, const FViewInfo& View);
};

/** The global used for deferred lighting. */
extern FCompositionLighting GCompositionLighting;
