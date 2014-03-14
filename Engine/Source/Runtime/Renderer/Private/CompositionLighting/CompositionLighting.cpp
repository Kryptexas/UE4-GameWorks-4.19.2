// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CompositionLighting.cpp: The center for all deferred lighting activities.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "CompositionLighting.h"
#include "PostProcessing.h"					// FPostprocessContext
#include "PostProcessInput.h"
#include "PostProcessAmbient.h"
#include "PostProcessLpvIndirect.h"
#include "PostProcessAmbientOcclusion.h"
#include "PostProcessDeferredDecals.h"
#include "BatchedElements.h"
#include "ScreenRendering.h"
#include "PostProcessPassThrough.h"
#include "ScreenSpaceReflections.h"
#include "PostProcessWeightedSampleSum.h"
#include "PostProcessTemporalAA.h"

/** The global center for all deferred lighting activities. */
FCompositionLighting GCompositionLighting;

// -------------------------------------------------------

static bool IsAmbientCubemapPassRequired(FPostprocessContext& Context)
{
	FScene* Scene = (FScene*)Context.View.Family->Scene;

	return Context.View.FinalPostProcessSettings.ContributingCubemaps.Num() != 0 && !IsSimpleDynamicLightingEnabled();
}

static bool IsLpvIndirectPassRequired(FPostprocessContext& Context)
{
	FScene* Scene = (FScene*)Context.View.Family->Scene;

	const FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

	if(ViewState)
	{
		FLightPropagationVolume* LightPropagationVolume = ViewState->GetLightPropagationVolume();

		if(LightPropagationVolume && Context.View.FinalPostProcessSettings.LPVIntensity > 0.01f)
		{
			return true; 
		}
	}

	return false;
}

static void AddPostProcessingLpvIndirect(FPostprocessContext& Context, FRenderingCompositePass* SSAO )
{
	FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new FRCPassPostProcessLpvIndirect());
	Pass->SetInput(ePId_Input0, Context.FinalOutput);
	Pass->SetInput(ePId_Input1, SSAO );

	Context.FinalOutput = FRenderingCompositeOutputRef(Pass);
}

static bool IsReflectionEnvironmentActive(FPostprocessContext& Context)
{
	FScene* Scene = (FScene*)Context.View.Family->Scene;

	// LPV & Screenspace Reflections : Reflection Environment active if either LPV (assumed true if this was called), Reflection Captures or SSR active

	bool IsReflectingEnvironment = Context.View.Family->EngineShowFlags.ReflectionEnvironment;
	bool HasReflectionCaptures = (Scene->ReflectionSceneData.RegisteredReflectionCaptures.Num() > 0);
	bool HasSSR = Context.View.Family->EngineShowFlags.ScreenSpaceReflections;

	return (GRHIFeatureLevel == ERHIFeatureLevel::SM5 && IsReflectingEnvironment && (HasReflectionCaptures || HasSSR) && !IsSimpleDynamicLightingEnabled() );
}

static bool IsBasePassAmbientOcclusionRequired(FPostprocessContext& Context)
{
	// the BaseAO pass is only worth with some AO
	return Context.View.FinalPostProcessSettings.AmbientOcclusionStaticFraction >= 1 / 100.0f && !IsSimpleDynamicLightingEnabled();
}

static bool IsAmbientOcclusionPassRequired(FPostprocessContext& Context)
{
	if(IsLpvIndirectPassRequired(Context) )
	{
		return true;
	}

	return
		Context.View.FinalPostProcessSettings.AmbientOcclusionIntensity > 0 
		&& Context.View.FinalPostProcessSettings.AmbientOcclusionRadius >= 0.1f 
		&& (IsBasePassAmbientOcclusionRequired(Context) || IsAmbientCubemapPassRequired(Context) || IsReflectionEnvironmentActive(Context) || Context.View.Family->EngineShowFlags.VisualizeBuffer )
		&& !IsSimpleDynamicLightingEnabled();
}

static void AddPostProcessingAmbientCubemap(FPostprocessContext& Context, FRenderingCompositeOutputRef AmbientOcclusion)
{
	FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAmbient());
	Pass->SetInput(ePId_Input0, Context.FinalOutput);
	Pass->SetInput(ePId_Input1, AmbientOcclusion);

	Context.FinalOutput = FRenderingCompositeOutputRef(Pass);
}

static FRenderingCompositeOutputRef AddPostProcessingAmbientOcclusion(FPostprocessContext& Context)
{
	uint32 AmbientOcclusionLevels;
	{
		static const auto ICVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AmbientOcclusionLevels"));
		AmbientOcclusionLevels = FMath::Clamp(ICVar->GetValueOnRenderThread(), 0, 4);
	}

	if(AmbientOcclusionLevels <= 0)
	{
		// off
		return FRenderingCompositeOutputRef();
	}

	FRenderingCompositePass* AmbientOcclusionInMip1 = 0;
	FRenderingCompositePass* AmbientOcclusionInMip2 = 0;
	FRenderingCompositePass* AmbientOcclusionInMip3 = 0;
	FRenderingCompositePass* AmbientOcclusionPassMip1 = 0; 
	FRenderingCompositePass* AmbientOcclusionPassMip2 = 0; 
	FRenderingCompositePass* AmbientOcclusionPassMip3 = 0; 

	// generate input in half, quarter, .. resolution

	AmbientOcclusionInMip1 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAmbientOcclusionSetup());
	AmbientOcclusionInMip1->SetInput(ePId_Input0, Context.SceneDepth);

	if(AmbientOcclusionLevels >= 3)
	{
		AmbientOcclusionInMip2 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAmbientOcclusionSetup());
		AmbientOcclusionInMip2->SetInput(ePId_Input1, FRenderingCompositeOutputRef(AmbientOcclusionInMip1, ePId_Output0));
	}

	if(AmbientOcclusionLevels >= 4)
	{
		AmbientOcclusionInMip3 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAmbientOcclusionSetup());
		AmbientOcclusionInMip3->SetInput(ePId_Input1, FRenderingCompositeOutputRef(AmbientOcclusionInMip2, ePId_Output0));
	}

	// upsample from lower resolution

	if(AmbientOcclusionLevels >= 4)
	{
		AmbientOcclusionPassMip3 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAmbientOcclusion());
		AmbientOcclusionPassMip3->SetInput(ePId_Input0, AmbientOcclusionInMip3);
		AmbientOcclusionPassMip3->SetInput(ePId_Input1, AmbientOcclusionInMip3);
	}

	if(AmbientOcclusionLevels >= 3)
	{
		AmbientOcclusionPassMip2 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAmbientOcclusion());
		AmbientOcclusionPassMip2->SetInput(ePId_Input0, AmbientOcclusionInMip2);
		AmbientOcclusionPassMip2->SetInput(ePId_Input1, AmbientOcclusionInMip2);
		AmbientOcclusionPassMip2->SetInput(ePId_Input2, AmbientOcclusionPassMip3);
	}

	if(AmbientOcclusionLevels >= 2)
	{
		AmbientOcclusionPassMip1 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAmbientOcclusion());
		AmbientOcclusionPassMip1->SetInput(ePId_Input0, AmbientOcclusionInMip1);
		AmbientOcclusionPassMip1->SetInput(ePId_Input1, AmbientOcclusionInMip1);
		AmbientOcclusionPassMip1->SetInput(ePId_Input2, AmbientOcclusionPassMip2);
	}

	// finally full resolution

	FRenderingCompositePass* AmbientOcclusionPassMip0 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAmbientOcclusion(false));
	AmbientOcclusionPassMip0->SetInput(ePId_Input0, Context.GBufferA);
	AmbientOcclusionPassMip0->SetInput(ePId_Input1, AmbientOcclusionInMip1);
	AmbientOcclusionPassMip0->SetInput(ePId_Input2, AmbientOcclusionPassMip1);

	// to make sure this pass is processed as well (before), needed to make process decals before computing AO
	AmbientOcclusionInMip1->AddDependency(Context.FinalOutput);

	Context.FinalOutput = FRenderingCompositeOutputRef(AmbientOcclusionPassMip0);

	GSceneRenderTargets.bScreenSpaceAOIsValid = true;

	if(IsBasePassAmbientOcclusionRequired(Context))
	{
		FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBasePassAO());
		Pass->AddDependency(Context.FinalOutput);

		Context.FinalOutput = FRenderingCompositeOutputRef(Pass);
	}

	return FRenderingCompositeOutputRef(AmbientOcclusionPassMip0);
}

static void AddDeferredDecalsBeforeBasePass(FPostprocessContext& Context)
{
	FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDeferredDecals(0));
	Pass->SetInput(ePId_Input0, Context.FinalOutput);

	Context.FinalOutput = FRenderingCompositeOutputRef(Pass);
}

static void AddDeferredDecalsBeforeLighting(FPostprocessContext& Context)
{
	FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDeferredDecals(1));
	Pass->SetInput(ePId_Input0, Context.FinalOutput);

	Context.FinalOutput = FRenderingCompositeOutputRef(Pass);
}

void FCompositionLighting::ProcessBeforeBasePass(const FViewInfo& View)
{
	check(IsInRenderingThread());

	// so that the passes can register themselves to the graph
	{
		FMemMark Mark(FMemStack::Get());
		FRenderingCompositePassContext CompositeContext(View);

		FPostprocessContext Context(CompositeContext.Graph, View);

		// Add the passes we want to add to the graph (commenting a line means the pass is not inserted into the graph) ----------

		// decals are before AmbientOcclusion so the decal can output a normal that AO is affected by
		if(Context.View.Family->EngineShowFlags.Decals && IsDBufferEnabled()) 
		{
			AddDeferredDecalsBeforeBasePass(Context);
		}

		// The graph setup should be finished before this line ----------------------------------------

		SCOPED_DRAW_EVENT(CompositionBeforeBasePass, DEC_SCENE_ITEMS);

		Context.FinalOutput.GetOutput()->RenderTargetDesc = GSceneRenderTargets.SceneColor->GetDesc();
		Context.FinalOutput.GetOutput()->PooledRenderTarget = GSceneRenderTargets.SceneColor;

		// you can add multiple dependencies
		CompositeContext.Root->AddDependency(Context.FinalOutput);

		CompositeContext.Process(TEXT("Composition_BeforeBasePass"));
	}
}

void FCompositionLighting::ProcessAfterBasePass(const FViewInfo& View)
{
	check(IsInRenderingThread());

	// might get renamed to refracted or ...WithAO
	GSceneRenderTargets.SceneColor->SetDebugName(TEXT("SceneColor"));
	// to be able to observe results with VisualizeTexture

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.SceneColor);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.GBufferA);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.GBufferB);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.GBufferC);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.GBufferD);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.GBufferE);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.ScreenSpaceAO);
	
	// so that the passes can register themselves to the graph
	{
		FMemMark Mark(FMemStack::Get());
		FRenderingCompositePassContext CompositeContext(View);

		FPostprocessContext Context(CompositeContext.Graph, View);

		// Add the passes we want to add to the graph (commenting a line means the pass is not inserted into the graph) ----------

		// decals are before AmbientOcclusion so the decal can output a normal that AO is affected by
		if( Context.View.Family->EngineShowFlags.Decals &&
			!Context.View.Family->EngineShowFlags.VisualizeLightCulling)		// decal are distracting when looking at LightCulling
		{
			AddDeferredDecalsBeforeLighting(Context);
		}

		FRenderingCompositeOutputRef AmbientOcclusion;

		if(IsAmbientOcclusionPassRequired(Context))
		{
			AmbientOcclusion = AddPostProcessingAmbientOcclusion(Context);
		}

		if(IsAmbientCubemapPassRequired(Context))
		{
			AddPostProcessingAmbientCubemap(Context, AmbientOcclusion);
		}

		// The graph setup should be finished before this line ----------------------------------------
    
		SCOPED_DRAW_EVENT(LightCompositionTasks_PreLighting, DEC_SCENE_ITEMS);
		Context.FinalOutput.GetOutput()->RenderTargetDesc = GSceneRenderTargets.SceneColor->GetDesc();
		Context.FinalOutput.GetOutput()->PooledRenderTarget = GSceneRenderTargets.SceneColor;
    
		// you can add multiple dependencies
		CompositeContext.Root->AddDependency(Context.FinalOutput);
    
		CompositeContext.Process(TEXT("CompositionLighting_AfterBasePass"));
	}
}


void FCompositionLighting::ProcessLighting(const FViewInfo& View)
{
	check(IsInRenderingThread());

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.ReflectiveShadowMapDiffuse);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.ReflectiveShadowMapNormal);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.ReflectiveShadowMapDepth);

	{
		FMemMark Mark(FMemStack::Get());
		FRenderingCompositePassContext CompositeContext(View);

		FPostprocessContext Context(CompositeContext.Graph, View);

		FRenderingCompositeOutputRef AmbientOcclusion;

		if(IsLpvIndirectPassRequired(Context))
		{
			FRenderingCompositePass* SSAO = Context.Graph.RegisterPass(new FRCPassPostProcessInput(GSceneRenderTargets.ScreenSpaceAO));
			AddPostProcessingLpvIndirect( Context, SSAO );
		}
		// The graph setup should be finished before this line ----------------------------------------

		SCOPED_DRAW_EVENT(CompositionLighting, DEC_SCENE_ITEMS);

		Context.FinalOutput.GetOutput()->RenderTargetDesc = GSceneRenderTargets.SceneColor->GetDesc();
		Context.FinalOutput.GetOutput()->PooledRenderTarget = GSceneRenderTargets.SceneColor;

		// you can add multiple dependencies
		CompositeContext.Root->AddDependency(Context.FinalOutput);

		CompositeContext.Process(TEXT("CompositionLighting_Lighting"));
	}
}
