// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
IFlexFluidSurfaceRendering.h: Flex fluid surface rendering definitions.
=============================================================================*/

#pragma once

#include "ShaderBaseClasses.h"
#include "ScenePrivate.h"

class IFlexFluidSurfaceRenderer
{
public:

	/**
	* Iterates through DynamicMeshElements picking out all corresponding fluid surface proxies and storing them
	* for later stages. Then selecting all visibe particle system mesh elements corresponding to thouse surface proxies.
	* Allocates render textures per fluid surface.
	*/
	virtual void UpdateProxiesAndResources(FRHICommandList& RHICmdList, TArray<FMeshBatchAndRelevance, SceneRenderingAllocator>& DynamicMeshElements, FSceneRenderTargets& SceneContext) = 0;

	/**
	* Clears the textures, renders particles for depth and thickness, smoothes the depth texture.
	*/
	virtual void RenderParticles(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views) = 0;

	/**
	* Renders opaque surfaces.
	*/
	virtual void RenderBasePass(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views) = 0;

	/**
	* Figures out whether FFlexFluidSurfaceRenderer::RenderDepth needs to be called for a given scene proxy.
	* Returns true for fluid surfaces and particle systems corresponding to fluid surfaces.
	*/
	virtual bool IsDepthMaskingRequired(const FPrimitiveSceneProxy* SceneProxy) = 0;

	/**
	* Render Depth for fluid surfaces used for masking the static pre-shadows,
	* skips any particle systems.
	*/
	virtual void RenderDepth(FRHICommandList& RHICmdList, FPrimitiveSceneProxy* SceneProxy, const FViewInfo& View) = 0;

	/**
	* Clears the temporarily stored surface proxies.
	*/
	virtual void Cleanup() = 0;
};

extern RENDERER_API class IFlexFluidSurfaceRenderer* GFlexFluidSurfaceRenderer;
