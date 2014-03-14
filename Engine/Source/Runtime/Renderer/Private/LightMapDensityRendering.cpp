// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LightMapDensityRendering.cpp: Implementation for rendering lightmap density.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"

//-----------------------------------------------------------------------------

#if !UE_BUILD_DOCS
// Typedef is necessary because the C preprocessor thinks the comma in the template parameter list is a comma in the macro parameter list.
#define IMPLEMENT_DENSITY_VERTEXSHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	typedef TLightMapDensityVS< LightMapPolicyType > TLightMapDensityVS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TLightMapDensityVS##LightMapPolicyName,TEXT("LightMapDensityShader"),TEXT("MainVertexShader"),SF_Vertex); \
	typedef TLightMapDensityHS< LightMapPolicyType > TLightMapDensityHS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TLightMapDensityHS##LightMapPolicyName,TEXT("LightMapDensityShader"),TEXT("MainHull"),SF_Hull); \
	typedef TLightMapDensityDS< LightMapPolicyType > TLightMapDensityDS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TLightMapDensityDS##LightMapPolicyName,TEXT("LightMapDensityShader"),TEXT("MainDomain"),SF_Domain); 

#define IMPLEMENT_DENSITY_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	typedef TLightMapDensityPS< LightMapPolicyType > TLightMapDensityPS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TLightMapDensityPS##LightMapPolicyName,TEXT("LightMapDensityShader"),TEXT("MainPixelShader"),SF_Pixel);

// Implement a pixel shader type for skylights and one without, and one vertex shader that will be shared between them
#define IMPLEMENT_DENSITY_LIGHTMAPPED_SHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	IMPLEMENT_DENSITY_VERTEXSHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	IMPLEMENT_DENSITY_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName);

IMPLEMENT_DENSITY_LIGHTMAPPED_SHADER_TYPE( FNoLightMapPolicy, FNoLightMapPolicy );
IMPLEMENT_DENSITY_LIGHTMAPPED_SHADER_TYPE( FDummyLightMapPolicy, FDummyLightMapPolicy );
IMPLEMENT_DENSITY_LIGHTMAPPED_SHADER_TYPE( TLightMapPolicy<LQ_LIGHTMAP>, TLightMapPolicyLQ );
IMPLEMENT_DENSITY_LIGHTMAPPED_SHADER_TYPE( TLightMapPolicy<HQ_LIGHTMAP>, TLightMapPolicyHQ );
#endif

bool FDeferredShadingSceneRenderer::RenderLightMapDensities()
{
	bool bDirty=0;

	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM3)
	{
		SCOPED_DRAW_EVENT(LightMapDensity, DEC_SCENE_ITEMS);

		// Draw the scene's emissive and light-map color.
		for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_CONDITIONAL_DRAW_EVENTF(EventView, Views.Num() > 1, DEC_SCENE_ITEMS, TEXT("View%d"), ViewIndex);

			FViewInfo& View = Views[ViewIndex];

			// Opaque blending, depth tests and writes.
			RHISetBlendState(TStaticBlendState<>::GetRHI());
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual>::GetRHI());
			RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);

			{
				SCOPED_DRAW_EVENT(Dynamic, DEC_SCENE_ITEMS);

				if( View.VisibleDynamicPrimitives.Num() > 0 )
				{
					// Draw the dynamic non-occluded primitives using a base pass drawing policy.
					TDynamicPrimitiveDrawer<FLightMapDensityDrawingPolicyFactory> Drawer(
						&View,FLightMapDensityDrawingPolicyFactory::ContextType(),true);
					for (int32 PrimitiveIndex = 0;PrimitiveIndex < View.VisibleDynamicPrimitives.Num();PrimitiveIndex++)
					{
						const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives[PrimitiveIndex];
						int32 PrimitiveId = PrimitiveSceneInfo->GetIndex();
						const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveId];
						const bool bVisible = View.PrimitiveVisibilityMap[PrimitiveId];

						// Only draw the primitive if it's visible
						if( bVisible && 
							// only draw opaque and masked primitives if wireframe is disabled
							(PrimitiveViewRelevance.bOpaqueRelevance || ViewFamily.EngineShowFlags.Wireframe) )
						{
							Drawer.SetPrimitive(PrimitiveSceneInfo->Proxy);
							PrimitiveSceneInfo->Proxy->DrawDynamicElements(
								&Drawer,
								&View		
								);
						}
					}
					bDirty |= Drawer.IsDirty(); 
				}
			}
		}
	}

	return bDirty;
}

bool FLightMapDensityDrawingPolicyFactory::DrawDynamicMesh(
	const FViewInfo& View,
	ContextType DrawingContext,
	const FMeshBatch& Mesh,
	bool bBackFace,
	bool bPreFog,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	bool bDirty = false;

	const FMaterialRenderProxy* MaterialRenderProxy = Mesh.MaterialRenderProxy;
	const FMaterial* Material = MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
	const EBlendMode BlendMode = Material->GetBlendMode();

	const bool bMaterialMasked = Material->IsMasked();
	const bool bMaterialModifiesMesh = Material->MaterialModifiesMeshPosition();
	if (!bMaterialMasked && !bMaterialModifiesMesh)
	{
		// Override with the default material for opaque materials that are not two sided
		MaterialRenderProxy = GEngine->LevelColorationLitMaterial->GetRenderProxy(false);
	}

	bool bIsLitMaterial = (Material->GetLightingModel() != MLM_Unlit);
	/*const */FLightMapInteraction LightMapInteraction = (Mesh.LCI && bIsLitMaterial) ? Mesh.LCI->GetLightMapInteraction() : FLightMapInteraction();
	// force simple lightmaps based on system settings
	bool bAllowHighQualityLightMaps = AllowHighQualityLightmaps() && LightMapInteraction.AllowsHighQualityLightmaps();
	if (bIsLitMaterial && PrimitiveSceneProxy && (PrimitiveSceneProxy->GetLightMapType() == LMIT_Texture))
	{
		// Should this object be texture lightmapped? Ie, is lighting not built for it??
		bool bUseDummyLightMapPolicy = false;
		if ((Mesh.LCI == NULL) || (Mesh.LCI->GetLightMapInteraction().GetType() != LMIT_Texture))
		{
			bUseDummyLightMapPolicy = true;

			LightMapInteraction.SetLightMapInteractionType(LMIT_Texture);
			LightMapInteraction.SetCoordinateScale(FVector2D(1.0f,1.0f));
			LightMapInteraction.SetCoordinateBias(FVector2D(0.0f,0.0f));
		}
		if (bUseDummyLightMapPolicy == false)
		{
			if (bAllowHighQualityLightMaps)
			{
				TLightMapDensityDrawingPolicy< TLightMapPolicy<HQ_LIGHTMAP> > DrawingPolicy( Mesh.VertexFactory, MaterialRenderProxy, TLightMapPolicy<HQ_LIGHTMAP>(), BlendMode );
				DrawingPolicy.DrawShared(&View,DrawingPolicy.CreateBoundShaderState());
				for( int32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.Num(); BatchElementIndex++)
				{
					DrawingPolicy.SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,
						TLightMapDensityDrawingPolicy< TLightMapPolicy<HQ_LIGHTMAP> >::ElementDataType(LightMapInteraction));
					DrawingPolicy.DrawMesh(Mesh,BatchElementIndex);
				}
				bDirty = true;
			}
			else
			{
				TLightMapDensityDrawingPolicy< TLightMapPolicy<LQ_LIGHTMAP> > DrawingPolicy( Mesh.VertexFactory, MaterialRenderProxy, TLightMapPolicy<LQ_LIGHTMAP>(), BlendMode );
				DrawingPolicy.DrawShared(&View,DrawingPolicy.CreateBoundShaderState());
				for( int32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.Num(); BatchElementIndex++)
				{
					DrawingPolicy.SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,
						TLightMapDensityDrawingPolicy< TLightMapPolicy<LQ_LIGHTMAP> >::ElementDataType(LightMapInteraction));
					DrawingPolicy.DrawMesh(Mesh,BatchElementIndex);
				}
				bDirty = true;
			}
		}
		else
		{
			TLightMapDensityDrawingPolicy<FDummyLightMapPolicy> DrawingPolicy(Mesh.VertexFactory,MaterialRenderProxy,FDummyLightMapPolicy(),BlendMode);
			DrawingPolicy.DrawShared(&View,DrawingPolicy.CreateBoundShaderState());
			for( int32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.Num(); BatchElementIndex++)
			{
				DrawingPolicy.SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,
					TLightMapDensityDrawingPolicy<FDummyLightMapPolicy>::ElementDataType(LightMapInteraction));
				DrawingPolicy.DrawMesh(Mesh,BatchElementIndex);
			}
			bDirty = true;
		}
	}
	else
	{
		TLightMapDensityDrawingPolicy<FNoLightMapPolicy> DrawingPolicy(Mesh.VertexFactory,MaterialRenderProxy,FNoLightMapPolicy(),BlendMode);
	 	DrawingPolicy.DrawShared(&View,DrawingPolicy.CreateBoundShaderState());
		for( int32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.Num(); BatchElementIndex++)
		{
			DrawingPolicy.SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,TLightMapDensityDrawingPolicy<FNoLightMapPolicy>::ElementDataType());
			DrawingPolicy.DrawMesh(Mesh,BatchElementIndex);
		}
		bDirty = true;
	}
	
	return bDirty;
}
