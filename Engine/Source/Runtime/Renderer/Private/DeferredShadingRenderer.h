// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeferredShadingRenderer.h: Scene rendering definitions.
=============================================================================*/

#pragma once

#include "DepthRendering.h"		// EDepthDrawingMode

class FLightShaftsOutput
{
public:
	bool bRendered;
	TRefCountPtr<IPooledRenderTarget> LightShaftOcclusion;

	FLightShaftsOutput() :
		bRendered(false)
	{}
};

/**
 * Scene renderer that implements a deferred shading pipeline and associated features.
 */
class FDeferredShadingSceneRenderer : public FSceneRenderer
{
public:

	/** Defines which objects we want to render in the EarlyZPass. */
	EDepthDrawingMode EarlyZPassMode;

	/** 
	 * Layout used to track translucent self shadow residency from the per-light shadow passes, 
	 * So that shadow maps can be re-used in the translucency pass where possible.
	 */
	FTextureLayout TranslucentSelfShadowLayout;
	int32 CachedTranslucentSelfShadowLightId;

	FDeferredShadingSceneRenderer(const FSceneViewFamily* InViewFamily,FHitProxyConsumer* HitProxyConsumer);

	/** Clears a view */
	void ClearView();

	/** Clears gbuffer where Z is still at the maximum value (ie no geometry rendered) */
    void ClearGBufferAtMaxZ();

	/** Clears LPVs for all views */
	void ClearLPVs();

	/** Propagates LPVs for all views */
	void PropagateLPVs();

    /** Renders the basepass for the static data of a given View. */
    bool RenderBasePassStaticData(FViewInfo& View);
	bool RenderBasePassStaticDataMasked(FViewInfo& View);
	bool RenderBasePassStaticDataDefault(FViewInfo& View);

	/** Sorts base pass draw lists front to back for improved GPU culling. */
	void SortBasePassStaticData(FVector ViewPosition);

    /** Renders the basepass for the dynamic data of a given View. */
    bool RenderBasePassDynamicData(FViewInfo& View);

    /** Renders the basepass for a given View. */
    bool RenderBasePass(FViewInfo& View);
	
	/** 
	* Renders the scene's base pass 
	* @return true if anything was rendered
	*/
	bool RenderBasePass();

	/** Finishes the view family rendering. */
    void RenderFinish();

	/** Renders the view family. */
	virtual void Render() OVERRIDE;

	/** Render the view family's hit proxies. */
	virtual void RenderHitProxies() OVERRIDE;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	void RenderVisualizeTexturePool();
#endif

	/**
	 * Whether or not to composite editor objects onto the scene as a post processing step
	 *
	 * @param View The view to test against
	 *
	 * @return true if compositing is needed
	 */
	static bool ShouldCompositeEditorPrimitives(const FViewInfo& View);

	/** bound shader state for occlusion test prims */
	static FGlobalBoundShaderState OcclusionTestBoundShaderState;

private:

	/** Creates a per object projected shadow for the given interaction. */
	void CreatePerObjectProjectedShadow(
		FLightPrimitiveInteraction* Interaction, 
		bool bCreateTranslucentObjectShadow, 
		bool bCreateInsetObjectShadow,
		const TArray<FProjectedShadowInfo*,SceneRenderingAllocator>& ViewDependentWholeSceneShadows,
		TArray<FProjectedShadowInfo*, SceneRenderingAllocator>& OutPreShadows);

	/**
	 * Creates a projected shadow for all primitives affected by a light.  
	 * @param LightSceneInfo - The light to create a shadow for.
	 */
	void CreateWholeSceneProjectedShadow(FLightSceneInfo* LightSceneInfo);

	/**
	 * Checks to see if this primitive is affected by various shadow types
	 *
	 * @param PrimitiveSceneInfoCompact The primitive to check for shadow interaction
	 * @param PreShadows The list of pre-shadows to check against
	 */
	void GatherShadowsForPrimitiveInner(const FPrimitiveSceneInfoCompact& PrimitiveSceneInfoCompact,
		const TArray<FProjectedShadowInfo*,SceneRenderingAllocator>& PreShadows,
		const TArray<FProjectedShadowInfo*,SceneRenderingAllocator>& ViewDependentWholeSceneShadows,
		bool bReflectionCaptureScene);

	/** Gathers the list of primitives used to draw various shadow types */
	void GatherShadowPrimitives(
		const TArray<FProjectedShadowInfo*,SceneRenderingAllocator>& PreShadows,
		const TArray<FProjectedShadowInfo*,SceneRenderingAllocator>& ViewDependentWholeSceneShadows,
		bool bReflectionCaptureScene);

	/** Updates the preshadow cache, allocating new preshadows that can fit and evicting old ones. */
	void UpdatePreshadowCache();

	/** Finds a matching cached preshadow, if one exists. */
	TRefCountPtr<FProjectedShadowInfo> GetCachedPreshadow(
		const FLightPrimitiveInteraction* InParentInteraction, 
		const FProjectedShadowInitializer& Initializer,
		const FBoxSphereBounds& Bounds,
		uint32 InResolutionX);

	/** Calculates projected shadow visibility. */
	void InitProjectedShadowVisibility();

	/** Returns whether a per object shadow should be created due to the light being a stationary light. */
	bool ShouldCreateObjectShadowForStationaryLight(const FLightSceneInfo* LightSceneInfo, const FPrimitiveSceneProxy* PrimitiveSceneProxy, bool bInteractionShadowMapped) const;

	/** Creates shadows for the given interaction. */
	void SetupInteractionShadows(
		FLightPrimitiveInteraction* Interaction, 
		FVisibleLightInfo& VisibleLightInfo, 
		bool bReflectionCaptureScene,
		const TArray<FProjectedShadowInfo*,SceneRenderingAllocator>& ViewDependentWholeSceneShadows,
		TArray<FProjectedShadowInfo*,SceneRenderingAllocator>& PreShadows);

	/** Finds the visible dynamic shadows for each view. */
	void InitDynamicShadows();

	/** Determines which primitives are visible for each view. */
	void InitViews(); 

	/**
	 * Renders the scene's prepass and occlusion queries.
	 * @return true if anything was rendered
	 */
	bool RenderPrePass();

	/** Issues occlusion queries. */
	void BeginOcclusionTests();

	/** Renders the scene's fogging. */
	bool RenderFog(FLightShaftsOutput LightShaftsOutput);

	/** Renders the scene's atmosphere. */
	void RenderAtmosphere(FLightShaftsOutput LightShaftsOutput);

	/** Renders reflections that can be done in a deferred pass. */
	void RenderDeferredReflections();

	/** Whether tiled deferred is supported and can be used at all. */
	bool CanUseTiledDeferred() const;

	/** Whether to use tiled deferred shading given a number of lights that support it. */
	bool ShouldUseTiledDeferred(int32 NumUnshadowedLights, int32 NumSimpleLights) const;

	/** Renders the lights in SortedLights in the range [0, NumUnshadowedLights) using tiled deferred shading. */
	void RenderTiledDeferredLighting(const TArray<FSortedLightSceneInfo, SceneRenderingAllocator>& SortedLights, int32 NumUnshadowedLights, const TArray<FSimpleLightEntry, SceneRenderingAllocator>& SimpleLights);

	/** Renders the scene's lighting. */
	void RenderLights();

	/** Renders an array of lights for the stationary light overlap viewmode. */
	void RenderLightArrayForOverlapViewmode(const TSparseArray<FLightSceneInfoCompact>& LightArray);

	/** Render stationary light overlap as complexity to scene color. */
	void RenderStationaryLightOverlap();
	
	/** Renders the scene's distortion */
	void RenderDistortion();

	/** 
	 * Renders the scene's translucency.
	 */
	void RenderTranslucency();

	/** Renders the scene's light shafts */
	FLightShaftsOutput RenderLightShaftOcclusion();

	void RenderLightShaftBloom();

	/** Makes a copy of scene color so that a material with a scene color node can read the up to date scene color. */
	void CopySceneColor(const FViewInfo& View, const FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/** Reuses an existing translucent shadow map if possible or re-renders one if necessary. */
	const FProjectedShadowInfo* PrepareTranslucentShadowMap(const FViewInfo& View, FPrimitiveSceneInfo* PrimitiveSceneInfo, bool bSeparateTranslucencyPass);

	/** Renders the velocities of movable objects for the motion blur effect. */
	void RenderVelocities(const FViewInfo& View, TRefCountPtr<IPooledRenderTarget>& VelocityRT, bool bLastFrame);

	/** Renders world-space lightmap density instead of the normal color. */
	bool RenderLightMapDensities();

	/** Updates the downsized depth buffer with the current full resolution depth buffer. */
	void UpdateDownsampledDepthSurface();

	/**
	 * Finish rendering a view, writing the contents to ViewFamily.RenderTarget.
	 * @param View - The view to process.
	*/
	void FinishRenderViewTarget(const FViewInfo* View, bool bLastView);

	/**
	  * Used by RenderLights to figure out if projected shadows need to be rendered to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @return true if anything needs to be rendered
	  */
	bool CheckForProjectedShadows( const FLightSceneInfo* LightSceneInfo ) const;

	/** Renders one pass point light shadows. */
	bool RenderOnePassPointLightShadows(const FLightSceneInfo* LightSceneInfo, bool bRenderedTranslucentObjectShadows, bool& bInjectedTranslucentVolume);

	/** Renders the projections of the given Shadows to the appropriate color render target. */
	void RenderProjections(
		const FLightSceneInfo* LightSceneInfo, 
		const TArray<FProjectedShadowInfo*,SceneRenderingAllocator>& Shadows
		);

	/** Renders the shadowmaps of translucent shadows and their projections onto opaque surfaces. */
	bool RenderTranslucentProjectedShadows( const FLightSceneInfo* LightSceneInfo );

	/** Renders reflective shadowmaps for LPVs */
	bool RenderReflectiveShadowMaps( const FLightSceneInfo* LightSceneInfo );

	/**
	  * Used by RenderLights to render projected shadows to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @return true if anything got rendered
	  */
	bool RenderProjectedShadows( const FLightSceneInfo* LightSceneInfo, bool bRenderedTranslucentObjectShadows, bool& bInjectedTranslucentVolume );

	/** Caches the depths of any preshadows that should be cached, and renders their projections. */
	bool RenderCachedPreshadows(const FLightSceneInfo* LightSceneInfo);

	/**
	  * Used by RenderLights to figure out if light functions need to be rendered to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @return true if anything got rendered
	  */
	bool CheckForLightFunction( const FLightSceneInfo* LightSceneInfo ) const;

	/**
	  * Used by RenderLights to render a light function to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @param LightIndex The light's index into FScene::Lights
	  */
	bool RenderLightFunction(const FLightSceneInfo* LightSceneInfo, bool bLightAttenuationCleared);

	/** Renders a light function indicating that whole scene shadowing being displayed is for previewing only, and will go away in game. */
	bool RenderPreviewShadowsIndicator(const FLightSceneInfo* LightSceneInfo, bool bLightAttenuationCleared);

	/** Renders a light function with the given material. */
	bool RenderLightFunctionForMaterial(const FLightSceneInfo* LightSceneInfo, const FMaterialRenderProxy* MaterialProxy, bool bLightAttenuationCleared, bool bRenderingPreviewShadowsIndicator);

	/**
	  * Used by RenderLights to render a light to the scene color buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @param LightIndex The light's index into FScene::Lights
	  * @return true if anything got rendered
	  */
	void RenderLight(const FLightSceneInfo* LightSceneInfo, bool bRenderOverlap, bool bIssueDrawEvent);

	/** Renders an array of simple lights using standard deferred shading. */
	void RenderSimpleLightsStandardDeferred(const TArray<FSimpleLightEntry, SceneRenderingAllocator>& SimpleLights);

	/** Clears the translucency lighting volumes before light accumulation. */
	void ClearTranslucentVolumeLighting();

	/** Add AmbientCubemap to the lighting volumes. */
	void InjectAmbientCubemapTranslucentVolumeLighting();

	/** Renders indirect lighting into the translucent lighting volumes. */
	void CompositeIndirectTranslucentVolumeLighting();

	/** Clears the volume texture used to accumulate per object shadows for translucency. */
	void ClearTranslucentVolumePerObjectShadowing();

	/** Accumulates the per object shadow's contribution for translucency. */
	void AccumulateTranslucentVolumeObjectShadowing(const FProjectedShadowInfo* InProjectedShadowInfo, bool bClearVolume);

	/** Accumulates direct lighting for the given light.  InProjectedShadowInfo can be NULL in which case the light will be unshadowed. */
	void InjectTranslucentVolumeLighting(const FLightSceneInfo& LightSceneInfo, const FProjectedShadowInfo* InProjectedShadowInfo);

	/** Accumulates direct lighting for an array of unshadowed lights. */
	void InjectTranslucentVolumeLightingArray(const TArray<FSortedLightSceneInfo, SceneRenderingAllocator>& SortedLights, int32 NumLights);

	/** Accumulates direct lighting for simple lights. */
	void InjectSimpleTranslucentVolumeLightingArray(const TArray<FSimpleLightEntry, SceneRenderingAllocator>& SimpleLights);

	/** Filters the translucency lighting volumes to reduce aliasing. */
	void FilterTranslucentVolumeLighting();

	friend class FTranslucentPrimSet;
};
