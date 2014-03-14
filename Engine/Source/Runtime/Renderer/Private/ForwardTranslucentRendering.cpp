// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ForwardTranslucentRendering.cpp: translucent rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"


/** Pixel shader used to copy scene color into another texture so that materials can read from scene color with a node. */
class FForwardCopySceneAlphaPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FForwardCopySceneAlphaPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FForwardCopySceneAlphaPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
	}
	FForwardCopySceneAlphaPS() {}

	void SetParameters()
	{
		SceneTextureParameters.Set(GetPixelShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SceneTextureParameters;
		return bShaderHasOutdatedParameters;
	}

private:
	FSceneTextureShaderParameters SceneTextureParameters;
};

IMPLEMENT_SHADER_TYPE(,FForwardCopySceneAlphaPS,TEXT("TranslucentLightingShaders"),TEXT("CopySceneAlphaMain"),SF_Pixel);

FGlobalBoundShaderState ForwardCopySceneAlphaBoundShaderState;

void FForwardShadingSceneRenderer::CopySceneAlpha(void)
{
	SCOPED_DRAW_EVENTF(EventCopy, DEC_SCENE_ITEMS, TEXT("CopySceneAlpha"));
	RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	GSceneRenderTargets.ResolveSceneColor();

	GSceneRenderTargets.BeginRenderingSceneAlphaCopy();

	int X = GSceneRenderTargets.GetBufferSizeXY().X;
	int Y = GSceneRenderTargets.GetBufferSizeXY().Y;

	RHISetViewport(0, 0, 0.0f, X, Y, 1.0f );

	TShaderMapRef<FScreenVS> ScreenVertexShader(GetGlobalShaderMap());
	TShaderMapRef<FForwardCopySceneAlphaPS> PixelShader(GetGlobalShaderMap());
	SetGlobalBoundShaderState(ForwardCopySceneAlphaBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *ScreenVertexShader, *PixelShader);

	PixelShader->SetParameters();

	DrawRectangle( 
		0, 0, 
		X, Y, 
		0, 0, 
		X, Y,
		FIntPoint(X, Y),
		GSceneRenderTargets.GetBufferSizeXY(),
		EDRF_UseTriangleOptimization);

	GSceneRenderTargets.FinishRenderingSceneAlphaCopy();
}







/** The parameters used to draw a translucent mesh. */
class FDrawTranslucentMeshForwardShadingAction
{
public:

	const FViewInfo& View;
	bool bBackFace;
	FHitProxyId HitProxyId;

	/** Initialization constructor. */
	FDrawTranslucentMeshForwardShadingAction(
		const FViewInfo& InView,
		bool bInBackFace,
		FHitProxyId InHitProxyId
		):
		View(InView),
		bBackFace(bInBackFace),
		HitProxyId(InHitProxyId)
	{}
	
	inline bool ShouldPackAmbientSH() const
	{
		// So shader code can read a single constant to get the ambient term
		return true;
	}

	const FLightSceneInfo* GetSimpleDirectionalLight() const 
	{ 
		return ((FScene*)View.Family->Scene)->SimpleDirectionalLight;
	}

	/** Draws the translucent mesh with a specific light-map type, and fog volume type */
	template<typename LightMapPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
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

		for (int32 BatchElementIndex = 0; BatchElementIndex<Parameters.Mesh.Elements.Num(); BatchElementIndex++)
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
	}
};

/**
 * Render a dynamic mesh using a translucent draw policy
 * @return true if the mesh rendered
 */
bool FTranslucencyForwardShadingDrawingPolicyFactory::DrawDynamicMesh(
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

	// Determine the mesh's material and blend mode.
	const FMaterial* Material = Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Only render translucent materials.
	if (IsTranslucentBlendMode(BlendMode))
	{
		ProcessBasePassMeshForForwardShading(
			FProcessBasePassMeshParameters(
				Mesh,
				Material,
				PrimitiveSceneProxy,
				true,
				false,
				ESceneRenderTargetsMode::SetTextures
				),
			FDrawTranslucentMeshForwardShadingAction(
				View,
				bBackFace,
				HitProxyId
				)
			);

		bDirty = true;
	}
	return bDirty;
}

/**
 * Render a static mesh using a translucent draw policy
 * @return true if the mesh rendered
 */
bool FTranslucencyForwardShadingDrawingPolicyFactory::DrawStaticMesh(
	const FViewInfo& View,
	ContextType DrawingContext,
	const FStaticMesh& StaticMesh,
	bool bPreFog,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	bool bDirty = false;
	const FMaterial* Material = StaticMesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);

	bDirty |= DrawDynamicMesh(
		View,
		DrawingContext,
		StaticMesh,
		false,
		bPreFog,
		PrimitiveSceneProxy,
		HitProxyId
		);
	return bDirty;
}

/*-----------------------------------------------------------------------------
FTranslucentPrimSet
-----------------------------------------------------------------------------*/

void FTranslucentPrimSet::DrawPrimitivesForForwardShading(const FViewInfo& View, FSceneRenderer& Renderer) const
{
	// Draw sorted scene prims
	for (int32 PrimIdx = 0; PrimIdx < SortedPrims.Num(); PrimIdx++)
	{
		FPrimitiveSceneInfo* PrimitiveSceneInfo = SortedPrims[PrimIdx].PrimitiveSceneInfo;
		int32 PrimitiveId = PrimitiveSceneInfo->GetIndex();
		const FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveId];

		RenderPrimitiveForForwardShading(View, PrimitiveSceneInfo, ViewRelevance);
	}
}

void FTranslucentPrimSet::RenderPrimitiveForForwardShading(
	const FViewInfo& View, 
	FPrimitiveSceneInfo* PrimitiveSceneInfo, 
	const FPrimitiveViewRelevance& ViewRelevance) const
{
	checkSlow(ViewRelevance.HasTranslucency());

	if(ViewRelevance.bDrawRelevance)
	{
		// Render dynamic scene prim
		if( ViewRelevance.bDynamicRelevance )
		{
			TDynamicPrimitiveDrawer<FTranslucencyForwardShadingDrawingPolicyFactory> TranslucencyDrawer(
				&View,
				FTranslucencyForwardShadingDrawingPolicyFactory::ContextType(),
				false
				);

			TranslucencyDrawer.SetPrimitive(PrimitiveSceneInfo->Proxy);
			PrimitiveSceneInfo->Proxy->DrawDynamicElements(
				&TranslucencyDrawer,
				&View
				);
		}

		// Render static scene prim
		if( ViewRelevance.bStaticRelevance )
		{
			// Render static meshes from static scene prim
			for( int32 StaticMeshIdx=0; StaticMeshIdx < PrimitiveSceneInfo->StaticMeshes.Num(); StaticMeshIdx++ )
			{
				FStaticMesh& StaticMesh = PrimitiveSceneInfo->StaticMeshes[StaticMeshIdx];
				if (View.StaticMeshVisibilityMap[StaticMesh.Id]
					// Only render static mesh elements using translucent materials
					&& StaticMesh.IsTranslucent() )
				{
					FTranslucencyForwardShadingDrawingPolicyFactory::DrawStaticMesh(
						View,
						FTranslucencyForwardShadingDrawingPolicyFactory::ContextType(),
						StaticMesh,
						false,
						PrimitiveSceneInfo->Proxy,
						StaticMesh.HitProxyId
						);
				}
			}
		}
	}
}

void FForwardShadingSceneRenderer::RenderTranslucency()
{
	if (ShouldRenderTranslucency())
	{
		const bool bGammaSpace = !IsMobileHDR();
		const bool bLinearHDR64 = !bGammaSpace && !IsMobileHDR32bpp();

		// Copy the view so emulation of framebuffer fetch works for alpha=depth.
		// Possible optimization: this copy shouldn't be needed unless something uses fetch of depth.
		if(bLinearHDR64 && GSupportsRenderTargetFormat_PF_FloatRGBA && (GSupportsShaderFramebufferFetch == false) && (!IsPCPlatform(GRHIShaderPlatform)))
		{
			//CopySceneAlpha();
		}

		SCOPED_DRAW_EVENT(Translucency, DEC_SCENE_ITEMS);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			SCOPED_CONDITIONAL_DRAW_EVENTF(EventView, Views.Num() > 1, DEC_SCENE_ITEMS, TEXT("View%d"), ViewIndex);

			const FViewInfo& View = Views[ViewIndex];

			if (!bGammaSpace)
			{
				GSceneRenderTargets.BeginRenderingTranslucency(View);
			}

			// Enable depth test, disable depth writes.
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_GreaterEqual>::GetRHI());

			// Draw only translucent prims that don't read from scene color
			View.TranslucentPrimSet.DrawPrimitivesForForwardShading(View, *this);
			// Draw the view's mesh elements with the translucent drawing policy.
			DrawViewElements<FTranslucencyForwardShadingDrawingPolicyFactory>(View,FTranslucencyForwardShadingDrawingPolicyFactory::ContextType(),SDPG_World,false);
			// Draw the view's mesh elements with the translucent drawing policy.
			DrawViewElements<FTranslucencyForwardShadingDrawingPolicyFactory>(View,FTranslucencyForwardShadingDrawingPolicyFactory::ContextType(),SDPG_Foreground,false);
		}
	}
}
