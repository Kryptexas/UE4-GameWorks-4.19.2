// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlexFluidSurfaceRendering.h: Flex fluid surface rendering definitions.
=============================================================================*/

#pragma once

#include "GameWorks/IFlexFluidSurfaceRendering.h"

class FFlexFluidSurfaceRenderer : public IFlexFluidSurfaceRenderer
{
public:
	static FFlexFluidSurfaceRenderer& get()
	{
		static FFlexFluidSurfaceRenderer instance;
		return instance;
	}

	/**
	 * Iterates through DynamicMeshElements picking out all corresponding fluid surface proxies and storing them 
	 * for later stages. Then selecting all visibe particle system mesh elements corresponding to thouse surface proxies.
	 * Allocates render textures per fluid surface.
	 */
	virtual void UpdateProxiesAndResources(FRHICommandList& RHICmdList, TArray<FMeshBatchAndRelevance, SceneRenderingAllocator>& DynamicMeshElements, FSceneRenderTargets& SceneContext) override;

	/**
	 * Clears the textures, renders particles for depth and thickness, smoothes the depth texture.
	 */
	virtual void RenderParticles(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views) override;

	/**
	 * Renders opaque surfaces.
	 */
	virtual void RenderBasePass(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views) override;

	/**
	 * Figures out whether FFlexFluidSurfaceRenderer::RenderDepth needs to be called for a given scene proxy.
	 * Returns true for fluid surfaces and particle systems corresponding to fluid surfaces.
	 */
	virtual bool IsDepthMaskingRequired(const FPrimitiveSceneProxy* SceneProxy) override;

	/**
	 * Render Depth for fluid surfaces used for masking the static pre-shadows, 
	 * skips any particle systems.
	 */
	virtual void RenderDepth(FRHICommandList& RHICmdList, FPrimitiveSceneProxy* SceneProxy, const FViewInfo& View) override;

	/**
	 * Clears the temporarily stored surface proxies.
	 */
	virtual void Cleanup() override;

	virtual ~FFlexFluidSurfaceRenderer() {}

private:

	TArray<class FFlexFluidSurfaceSceneProxy*> SurfaceSceneProxies;
};
