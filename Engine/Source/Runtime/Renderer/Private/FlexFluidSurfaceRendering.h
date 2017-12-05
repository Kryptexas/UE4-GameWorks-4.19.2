// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlexFluidSurfaceRendering.h: Flex fluid surface rendering definitions.
=============================================================================*/

#pragma once

#include "ShaderBaseClasses.h"
#include "ScenePrivate.h"

class FFlexFluidSurfaceRenderer
{
public:

	/**
	 * Texture initialization and particle depth rendering and smoothing.
	 */
	void PreRenderOpaque(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views);

	/**
	 * Render thickness after scene depth buffer is available.
	 */
	void PostRenderOpaque(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views);
};

extern FFlexFluidSurfaceRenderer GFlexFluidSurfaceRenderer;