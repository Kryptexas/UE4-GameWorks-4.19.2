// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ForwardBasePassRendering.cpp: Base pass rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"

#define IMPLEMENT_FORWARD_SHADING_BASEPASS_LIGHTMAPPED_SHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	typedef TBasePassForForwardShadingVS< LightMapPolicyType, LDR_GAMMA_32 > TBasePassForForwardShadingVS##LightMapPolicyName##LDRGamma32; \
	typedef TBasePassForForwardShadingVS< LightMapPolicyType, HDR_LINEAR_64 > TBasePassForForwardShadingVS##LightMapPolicyName##HDRLinear64; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassForForwardShadingVS##LightMapPolicyName##LDRGamma32,TEXT("BasePassForForwardShadingVertexShader"),TEXT("Main"),SF_Vertex); \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassForForwardShadingVS##LightMapPolicyName##HDRLinear64,TEXT("BasePassForForwardShadingVertexShader"),TEXT("Main"),SF_Vertex); \
	typedef TBasePassForForwardShadingPS< LightMapPolicyType, LDR_GAMMA_32 > TBasePassForForwardShadingPS##LightMapPolicyName##LDRGamma32; \
	typedef TBasePassForForwardShadingPS< LightMapPolicyType, HDR_LINEAR_32 > TBasePassForForwardShadingPS##LightMapPolicyName##HDRLinear32; \
	typedef TBasePassForForwardShadingPS< LightMapPolicyType, HDR_LINEAR_64 > TBasePassForForwardShadingPS##LightMapPolicyName##HDRLinear64; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassForForwardShadingPS##LightMapPolicyName##LDRGamma32,TEXT("BasePassForForwardShadingPixelShader"),TEXT("Main"),SF_Pixel); \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassForForwardShadingPS##LightMapPolicyName##HDRLinear32,TEXT("BasePassForForwardShadingPixelShader"),TEXT("Main"),SF_Pixel); \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassForForwardShadingPS##LightMapPolicyName##HDRLinear64,TEXT("BasePassForForwardShadingPixelShader"),TEXT("Main"),SF_Pixel);

// Implement shader types per lightmap policy
IMPLEMENT_FORWARD_SHADING_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FNoLightMapPolicy, FNoLightMapPolicy ); 
IMPLEMENT_FORWARD_SHADING_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TLightMapPolicy<LQ_LIGHTMAP>, TLightMapPolicyLQ );
IMPLEMENT_FORWARD_SHADING_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TDistanceFieldShadowsAndLightMapPolicy<LQ_LIGHTMAP>, TDistanceFieldShadowsAndLightMapPolicyLQ );
IMPLEMENT_FORWARD_SHADING_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FSimpleDirectionalLightAndSHIndirectPolicy, FSimpleDirectionalLightAndSHIndirectPolicy );

/** The action used to draw a base pass static mesh element. */
class FDrawBasePassForwardShadingStaticMeshAction
{
public:

	FScene* Scene;
	FStaticMesh* StaticMesh;

	/** Initialization constructor. */
	FDrawBasePassForwardShadingStaticMeshAction(FScene* InScene,FStaticMesh* InStaticMesh):
		Scene(InScene),
		StaticMesh(InStaticMesh)
	{}

	inline bool ShouldPackAmbientSH() const
	{
		return false;
	}

	const FLightSceneInfo* GetSimpleDirectionalLight() const 
	{ 
		return Scene->SimpleDirectionalLight;
	}

	/** Draws the translucent mesh with a specific light-map type, and fog volume type */
	template<typename LightMapPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
		FScene::EBasePassDrawListType DrawType = FScene::EBasePass_Default;		
 
		if (StaticMesh->IsMasked())
		{
			DrawType = FScene::EBasePass_Masked;	
		}

		// Find the appropriate draw list for the static mesh based on the light-map policy type.
		TStaticMeshDrawList<TBasePassForForwardShadingDrawingPolicy<LightMapPolicyType> >& DrawList =
			Scene->GetForwardShadingBasePassDrawList<LightMapPolicyType>(DrawType);

		// Add the static mesh to the draw list.
		DrawList.AddMesh(
			StaticMesh,
			typename TBasePassForForwardShadingDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData),
			TBasePassForForwardShadingDrawingPolicy<LightMapPolicyType>(
				StaticMesh->VertexFactory,
				StaticMesh->MaterialRenderProxy,
				*Parameters.Material,
				LightMapPolicy,
				Parameters.BlendMode,
				Parameters.TextureMode
				)
			);
	}
};

void FBasePassForwardOpaqueDrawingPolicyFactory::AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh)
{
	// Determine the mesh's material and blend mode.
	const FMaterial* Material = StaticMesh->MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Only draw opaque materials.
	if( !IsTranslucentBlendMode(BlendMode) )
	{
		ProcessBasePassMeshForForwardShading(
			FProcessBasePassMeshParameters(
				*StaticMesh,
				Material,
				StaticMesh->PrimitiveSceneInfo->Proxy,
				true,
				false,
				ESceneRenderTargetsMode::DontSet
				),
			FDrawBasePassForwardShadingStaticMeshAction(Scene,StaticMesh)
			);
	}
}

/** The action used to draw a base pass dynamic mesh element. */
class FDrawBasePassForwardShadingDynamicMeshAction
{
public:

	const FSceneView& View;
	bool bBackFace;
	FHitProxyId HitProxyId;

	inline bool ShouldPackAmbientSH() const
	{
		return false;
	}

	const FLightSceneInfo* GetSimpleDirectionalLight() const 
	{ 
		return ((FScene*)View.Family->Scene)->SimpleDirectionalLight;
	}

	/** Initialization constructor. */
	FDrawBasePassForwardShadingDynamicMeshAction(
		const FSceneView& InView,
		const bool bInBackFace,
		const FHitProxyId InHitProxyId
		)
		: View(InView)
		, bBackFace(bInBackFace)
		, HitProxyId(InHitProxyId)
	{}

	/** Draws the translucent mesh with a specific light-map type, and shader complexity predicate. */
	template<typename LightMapPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// Treat masked materials as if they don't occlude in shader complexity, which is PVR behavior
		if(Parameters.BlendMode == BLEND_Masked && View.Family->EngineShowFlags.ShaderComplexity)
		{
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_GreaterEqual>::GetRHI());
		}
#endif

		const bool bIsLitMaterial = Parameters.LightingModel != MLM_Unlit;

		TBasePassForForwardShadingDrawingPolicy<LightMapPolicyType> DrawingPolicy(
			Parameters.Mesh.VertexFactory,
			Parameters.Mesh.MaterialRenderProxy,
			*Parameters.Material,
			LightMapPolicy,
			Parameters.BlendMode,
			Parameters.TextureMode,
			View.Family->EngineShowFlags.ShaderComplexity
			);
		DrawingPolicy.DrawShared(
			&View,
			DrawingPolicy.CreateBoundShaderState()
			);

		for( int32 BatchElementIndex=0;BatchElementIndex<Parameters.Mesh.Elements.Num();BatchElementIndex++ )
		{
			DrawingPolicy.SetMeshRenderState(
				View,
				Parameters.PrimitiveSceneProxy,
				Parameters.Mesh,
				BatchElementIndex,
				bBackFace,
				typename TBasePassForForwardShadingDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData)
				);
			DrawingPolicy.DrawMesh(Parameters.Mesh, BatchElementIndex);
		}

		// Restore
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if(Parameters.BlendMode == BLEND_Masked && View.Family->EngineShowFlags.ShaderComplexity)
		{
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual>::GetRHI());
		}
#endif
	}
};

bool FBasePassForwardOpaqueDrawingPolicyFactory::DrawDynamicMesh(
	const FSceneView& View,
	ContextType DrawingContext,
	const FMeshBatch& Mesh,
	bool bBackFace,
	bool bPreFog,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	// Determine the mesh's material and blend mode.
	const FMaterial* Material = Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Only draw opaque materials.
	if(!IsTranslucentBlendMode(BlendMode))
	{
		ProcessBasePassMeshForForwardShading(
			FProcessBasePassMeshParameters(
				Mesh,
				Material,
				PrimitiveSceneProxy,
				true,
				false,
				DrawingContext.TextureMode
				),
			FDrawBasePassForwardShadingDynamicMeshAction(
				View,
				bBackFace,
				HitProxyId
				)
			);
		return true;
	}
	else
	{
		return false;
	}
}

/** Base pass sorting modes. */
namespace EBasePassSort
{
	enum Type
	{
		/** Automatically select based on hardware/platform. */
		Auto = 0,
		/** No sorting. */
		None = 1,
		/** Sorts state buckets, not individual meshes. */
		SortStateBuckets = 2,
		/** Per mesh sorting. */
		SortPerMesh = 3,

		/** Useful range of sort modes. */
		FirstForcedMode = None,
		LastForcedMode = SortPerMesh
	};
};
TAutoConsoleVariable<int32> GSortBasePass(TEXT("r.ForwardBasePassSort"),0,
	TEXT("How to sort the forward shading base pass:\n")
	TEXT("\t0: Decide automatically based on the hardware.\n")
	TEXT("\t1: Sort drawing policies.\n")
	TEXT("\t2: Sort drawing policies and the meshes within them."),
	ECVF_RenderThreadSafe);
TAutoConsoleVariable<int32> GMaxBasePassDraws(TEXT("r.MaxForwardBasePassDraws"),0,TEXT("Stops rendering static forward base pass draws after the specified number of times. Useful for seeing the order in which meshes render when optimizing."),ECVF_RenderThreadSafe);

EBasePassSort::Type GetSortMode()
{
	int32 SortMode = GSortBasePass.GetValueOnRenderThread();
	if (SortMode >= EBasePassSort::FirstForcedMode && SortMode <= EBasePassSort::LastForcedMode)
	{
		return (EBasePassSort::Type)SortMode;
	}

	// Determine automatically.
	if (GHardwareHiddenSurfaceRemoval)
	{
		return EBasePassSort::None;
	}
	else
	{
		return EBasePassSort::SortPerMesh;
	}
}

void FForwardShadingSceneRenderer::RenderForwardShadingBasePass()
{
	SCOPED_DRAW_EVENT(BasePass, DEC_SCENE_ITEMS);
	SCOPE_CYCLE_COUNTER(STAT_BasePassDrawTime);

	EBasePassSort::Type SortMode = GetSortMode();
	int32 MaxDraws = GMaxBasePassDraws.GetValueOnRenderThread();
	if (MaxDraws <= 0)
	{
		MaxDraws = MAX_int32;
	}

	if (SortMode == EBasePassSort::SortStateBuckets)
	{
		SCOPE_CYCLE_COUNTER(STAT_SortStaticDrawLists);

		for (int32 DrawType = 0; DrawType < FScene::EBasePass_MAX; DrawType++)
		{
			Scene->BasePassForForwardShadingLowQualityLightMapDrawList[DrawType].SortFrontToBack(Views[0].ViewLocation);
			Scene->BasePassForForwardShadingDistanceFieldShadowMapLightMapDrawList[DrawType].SortFrontToBack(Views[0].ViewLocation);
			Scene->BasePassForForwardShadingDirectionalLightAndSHIndirectDrawList[DrawType].SortFrontToBack(Views[0].ViewLocation);
			Scene->BasePassForForwardShadingNoLightMapDrawList[DrawType].SortFrontToBack(Views[0].ViewLocation);
		}
	}

	// Draw the scene's emissive and light-map color.
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		SCOPED_CONDITIONAL_DRAW_EVENTF(EventView, Views.Num() > 1, DEC_SCENE_ITEMS, TEXT("View%d"), ViewIndex);
		FViewInfo& View = Views[ViewIndex];

		// Opaque blending
		RHISetBlendState(TStaticBlendStateWriteMask<CW_RGBA>::GetRHI());
		// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
		RHISetDepthStencilState(TStaticDepthStencilState<true, CF_GreaterEqual>::GetRHI());
		RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);

		// Render the base pass static data
		if (SortMode == EBasePassSort::SortPerMesh)
		{
			SCOPE_CYCLE_COUNTER(STAT_StaticDrawListDrawTime);
			MaxDraws -= Scene->BasePassForForwardShadingLowQualityLightMapDrawList[FScene::EBasePass_Default].DrawVisibleFrontToBack(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility,MaxDraws);
			MaxDraws -= Scene->BasePassForForwardShadingDistanceFieldShadowMapLightMapDrawList[FScene::EBasePass_Default].DrawVisibleFrontToBack(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility,MaxDraws);
			MaxDraws -= Scene->BasePassForForwardShadingDirectionalLightAndSHIndirectDrawList[FScene::EBasePass_Default].DrawVisibleFrontToBack(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility,MaxDraws);
			MaxDraws -= Scene->BasePassForForwardShadingNoLightMapDrawList[FScene::EBasePass_Default].DrawVisibleFrontToBack(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility,MaxDraws);
		}
		else
		{
			SCOPE_CYCLE_COUNTER(STAT_StaticDrawListDrawTime);
			Scene->BasePassForForwardShadingLowQualityLightMapDrawList[FScene::EBasePass_Default].DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
			Scene->BasePassForForwardShadingDistanceFieldShadowMapLightMapDrawList[FScene::EBasePass_Default].DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
			Scene->BasePassForForwardShadingDirectionalLightAndSHIndirectDrawList[FScene::EBasePass_Default].DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
			Scene->BasePassForForwardShadingNoLightMapDrawList[FScene::EBasePass_Default].DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
		}

		{
			SCOPE_CYCLE_COUNTER(STAT_DynamicPrimitiveDrawTime);
			SCOPED_DRAW_EVENT(Dynamic, DEC_SCENE_ITEMS);

			if (View.VisibleDynamicPrimitives.Num() > 0)
			{
				// Draw the dynamic non-occluded primitives using a base pass drawing policy.
				TDynamicPrimitiveDrawer<FBasePassForwardOpaqueDrawingPolicyFactory> Drawer(&View, FBasePassForwardOpaqueDrawingPolicyFactory::ContextType(ESceneRenderTargetsMode::DontSet), true);

				for (int32 PrimitiveIndex = 0; PrimitiveIndex < View.VisibleDynamicPrimitives.Num(); PrimitiveIndex++)
				{
					const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives[PrimitiveIndex];
					int32 PrimitiveId = PrimitiveSceneInfo->GetIndex();
					const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveId];

					const bool bVisible = View.PrimitiveVisibilityMap[PrimitiveId];

					// Only draw the primitive if it's visible
					if (bVisible && 
						// only draw opaque and masked primitives if wireframe is disabled
						(PrimitiveViewRelevance.bOpaqueRelevance || ViewFamily.EngineShowFlags.Wireframe))
					{
						FScopeCycleCounter Context(PrimitiveSceneInfo->Proxy->GetStatId());
						Drawer.SetPrimitive(PrimitiveSceneInfo->Proxy);
						PrimitiveSceneInfo->Proxy->DrawDynamicElements(&Drawer, &View);
					}
				}
			}

			const bool bNeedToSwitchVerticalAxis = IsES2Platform(GRHIShaderPlatform) && !IsPCPlatform(GRHIShaderPlatform);

			// Draw the base pass for the view's batched mesh elements.
			DrawViewElements<FBasePassForwardOpaqueDrawingPolicyFactory>(View,FBasePassForwardOpaqueDrawingPolicyFactory::ContextType(ESceneRenderTargetsMode::DontSet), SDPG_World, true);

			// Draw the view's batched simple elements(lines, sprites, etc).
			View.BatchedViewElements.Draw(bNeedToSwitchVerticalAxis, View.ViewProjectionMatrix, View.ViewRect.Width(), View.ViewRect.Height(), false);

			// Draw foreground objects last
			DrawViewElements<FBasePassForwardOpaqueDrawingPolicyFactory>(View,FBasePassForwardOpaqueDrawingPolicyFactory::ContextType(ESceneRenderTargetsMode::DontSet), SDPG_Foreground, true);

			// Draw the view's batched simple elements(lines, sprites, etc).
			View.TopBatchedViewElements.Draw(bNeedToSwitchVerticalAxis, View.ViewProjectionMatrix, View.ViewRect.Width(), View.ViewRect.Height(), false);
		}

		// Issue static draw list masked draw calls last, as PVR wants it
		if (SortMode == EBasePassSort::SortPerMesh)
		{
			SCOPE_CYCLE_COUNTER(STAT_StaticDrawListDrawTime);
			MaxDraws -= Scene->BasePassForForwardShadingNoLightMapDrawList[FScene::EBasePass_Masked].DrawVisibleFrontToBack(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility,MaxDraws);
			MaxDraws -= Scene->BasePassForForwardShadingLowQualityLightMapDrawList[FScene::EBasePass_Masked].DrawVisibleFrontToBack(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility,MaxDraws);
			MaxDraws -= Scene->BasePassForForwardShadingDistanceFieldShadowMapLightMapDrawList[FScene::EBasePass_Masked].DrawVisibleFrontToBack(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility,MaxDraws);
			MaxDraws -= Scene->BasePassForForwardShadingDirectionalLightAndSHIndirectDrawList[FScene::EBasePass_Masked].DrawVisibleFrontToBack(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility,MaxDraws);
		}
		else
		{
			SCOPE_CYCLE_COUNTER(STAT_StaticDrawListDrawTime);
			Scene->BasePassForForwardShadingNoLightMapDrawList[FScene::EBasePass_Masked].DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
			Scene->BasePassForForwardShadingLowQualityLightMapDrawList[FScene::EBasePass_Masked].DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
			Scene->BasePassForForwardShadingDistanceFieldShadowMapLightMapDrawList[FScene::EBasePass_Masked].DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
			Scene->BasePassForForwardShadingDirectionalLightAndSHIndirectDrawList[FScene::EBasePass_Masked].DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
		}
	}
}
