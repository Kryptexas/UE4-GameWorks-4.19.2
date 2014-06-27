// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ForwardShadingRenderer.cpp: Scene rendering code for the ES2 feature level.
=============================================================================*/

#include "RendererPrivate.h"
#include "Engine.h"
#include "ScenePrivate.h"
#include "FXSystem.h"
#include "PostProcessing.h"
#include "SceneFilterRendering.h"
#include "PostProcessMobile.h"


FForwardShadingSceneRenderer::FForwardShadingSceneRenderer(const FSceneViewFamily* InViewFamily,FHitProxyConsumer* HitProxyConsumer)
	:	FSceneRenderer(InViewFamily, HitProxyConsumer)
{
}

/**
 * Initialize scene's views.
 * Check visibility, sort translucent items, etc.
 */
void FForwardShadingSceneRenderer::InitViews(FRHICommandListImmediate& RHICmdList)
{
	SCOPED_DRAW_EVENT(InitViews, DEC_SCENE_ITEMS);

	SCOPE_CYCLE_COUNTER(STAT_InitViewsTime);

	PreVisibilityFrameSetup(RHICmdList);
	ComputeViewVisibility(RHICmdList);
	PostVisibilityFrameSetup();

	OnStartFrame();
}

/** 
* Renders the view family. 
*/
void FForwardShadingSceneRenderer::Render(FRHICommandListImmediate& RHICmdList)
{
	if(!ViewFamily.EngineShowFlags.Rendering)
	{
		return;
	}

	auto FeatureLevel = ViewFamily.Scene->GetFeatureLevel();

	// Initialize global system textures (pass-through if already initialized).
	GSystemTextures.InitializeTextures(RHICmdList, FeatureLevel);

	// Allocate the maximum scene render target space for the current view family.
	GSceneRenderTargets.Allocate(ViewFamily);

	// Find the visible primitives.
	InitViews(RHICmdList);
	
	// Notify the FX system that the scene is about to be rendered.
	if (Scene->FXSystem)
	{
		Scene->FXSystem->PreRender(RHICmdList);
	}

	GRenderTargetPool.VisualizeTexture.OnStartFrame(Views[0]);

	// Dynamic vertex and index buffers need to be committed before rendering.
	FGlobalDynamicVertexBuffer::Get().Commit();
	FGlobalDynamicIndexBuffer::Get().Commit();

	const bool bGammaSpace = !IsMobileHDR();
	if( bGammaSpace )
	{
		SetRenderTarget(RHICmdList, ViewFamily.RenderTarget->GetRenderTargetTexture(), GSceneRenderTargets.GetSceneDepthTexture());
	}
	else
	{
		// Begin rendering to scene color
		GSceneRenderTargets.BeginRenderingSceneColor(RHICmdList, false);
	}

	// Clear color and depth buffer
	// Note, this is a reversed Z depth surface, so 0.0f is the far plane.
	RHICmdList.Clear(true, FLinearColor::Black, true, 0.0f, true, 0, FIntRect());

	RenderForwardShadingBasePass(RHICmdList);

	// Make a copy of the scene depth if the current hardware doesn't support reading and writing to the same depth buffer
	GSceneRenderTargets.ResolveSceneDepthToAuxiliaryTexture(RHICmdList);

	// Notify the FX system that opaque primitives have been rendered.
	if (Scene->FXSystem)
	{
		Scene->FXSystem->PostRenderOpaque(RHICmdList);
	}

	// Draw translucency.
	if (ViewFamily.EngineShowFlags.Translucency)
	{
		SCOPE_CYCLE_COUNTER(STAT_TranslucencyDrawTime);

		if (ViewFamily.EngineShowFlags.Refraction)
		{
			// to apply refraction effect by distorting the scene color
			RenderDistortion(RHICmdList);
		}
		RenderTranslucency(RHICmdList);
	}

	if( !bGammaSpace )
	{
		// This might eventually be a problem with multiple views.
		// Using only view 0 to check to do on-chip transform of alpha.
		const FViewInfo& View = Views[0];

		bool bOnChipSunMask = 
			GSupportsRenderTargetFormat_PF_FloatRGBA && 
			GSupportsShaderFramebufferFetch &&
			ViewFamily.EngineShowFlags.PostProcessing &&
			((View.bLightShaftUse) || (View.FinalPostProcessSettings.DepthOfFieldScale > 0.0));

		// Convert alpha from depth to circle of confusion with sunshaft intensity.
		// This is done before resolve on hardware with framebuffer fetch.
		if(bOnChipSunMask)
		{
			// This will break when PrePostSourceViewportSize is not full size.
			FIntPoint PrePostSourceViewportSize = GSceneRenderTargets.GetBufferSizeXY();

			FMemMark Mark(FMemStack::Get());
			FRenderingCompositePassContext CompositeContext(RHICmdList, View);

			FRenderingCompositePass* PostProcessSunMask = CompositeContext.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSunMaskES2(PrePostSourceViewportSize, true));
			CompositeContext.Root->AddDependency(FRenderingCompositeOutputRef(PostProcessSunMask));
			CompositeContext.Process(TEXT("OnChipAlphaTransform"));
		}

		// Resolve the scene color for post processing.
		GSceneRenderTargets.ResolveSceneColor(RHICmdList, FResolveRect(0, 0, ViewFamily.FamilySizeX, ViewFamily.FamilySizeY));

		// Drop depth and stencil before post processing to avoid export.
		RHICmdList.DiscardRenderTargets(true, true, 0);

		// Finish rendering for each view.
		{
			SCOPED_DRAW_EVENT(FinishRendering, DEC_SCENE_ITEMS);
			SCOPE_CYCLE_COUNTER(STAT_FinishRenderViewTargetTime);
			for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{	
				SCOPED_CONDITIONAL_DRAW_EVENTF(EventView, Views.Num() > 1, DEC_SCENE_ITEMS, TEXT("View%d"), ViewIndex);
				GPostProcessing.ProcessES2(RHICmdList, Views[ViewIndex], bOnChipSunMask);
			}
		}
	}

	RenderFinish(RHICmdList);
}
