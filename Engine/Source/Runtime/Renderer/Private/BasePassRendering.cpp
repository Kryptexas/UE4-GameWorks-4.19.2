// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BasePassRendering.cpp: Base pass rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"

/** Whether to replace lightmap textures with solid colors to visualize the mip-levels. */
bool GVisualizeMipLevels = false;

// Typedef is necessary because the C preprocessor thinks the comma in the template parameter list is a comma in the macro parameter list.
// BasePass Vertex Shader needs to include hull and domain shaders for tessellation, these only compile for D3D11
#define IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	typedef TBasePassVS< LightMapPolicyType, false > TBasePassVS##LightMapPolicyName ; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassVS##LightMapPolicyName,TEXT("BasePassVertexShader"),TEXT("Main"),SF_Vertex); \
	typedef TBasePassHS< LightMapPolicyType > TBasePassHS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassHS##LightMapPolicyName,TEXT("BasePassTessellationShaders"),TEXT("MainHull"),SF_Hull); \
	typedef TBasePassDS< LightMapPolicyType > TBasePassDS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassDS##LightMapPolicyName,TEXT("BasePassTessellationShaders"),TEXT("MainDomain"),SF_Domain); 

#define IMPLEMENT_BASEPASS_VERTEXSHADER_ONLY_TYPE(LightMapPolicyType,LightMapPolicyName,AtmosphericFogShaderName) \
	typedef TBasePassVS<LightMapPolicyType,true> TBasePassVS##LightMapPolicyName##AtmosphericFogShaderName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassVS##LightMapPolicyName##AtmosphericFogShaderName,TEXT("BasePassVertexShader"),TEXT("Main"),SF_Vertex)

#define IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName,bEnableSkyLight,SkyLightName) \
	typedef TBasePassPS<LightMapPolicyType, bEnableSkyLight> TBasePassPS##LightMapPolicyName##SkyLightName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassPS##LightMapPolicyName##SkyLightName,TEXT("BasePassPixelShader"),TEXT("Main"),SF_Pixel);

// Implement a pixel shader type for skylights and one without, and one vertex shader that will be shared between them
#define IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	IMPLEMENT_BASEPASS_VERTEXSHADER_ONLY_TYPE(LightMapPolicyType,LightMapPolicyName,AtmosphericFog) \
	IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName,true,Skylight) \
	IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName,false,);

// Implement shader types per lightmap policy
// If renaming or refactoring these, remember to update FMaterialResource::GetRepresentativeInstructionCounts and FPreviewMaterial::ShouldCache().
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FNoLightMapPolicy, FNoLightMapPolicy ); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TLightMapPolicy<HQ_LIGHTMAP>, TLightMapPolicyHQ ); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TLightMapPolicy<LQ_LIGHTMAP>, TLightMapPolicyLQ ); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TDistanceFieldShadowsAndLightMapPolicy<HQ_LIGHTMAP>, TDistanceFieldShadowsAndLightMapPolicyHQ );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FSelfShadowedTranslucencyPolicy, FSelfShadowedTranslucencyPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FSelfShadowedCachedPointIndirectLightingPolicy, FSelfShadowedCachedPointIndirectLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FCachedVolumeIndirectLightingPolicy, FCachedVolumeIndirectLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FCachedPointIndirectLightingPolicy, FCachedPointIndirectLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FSimpleDynamicLightingPolicy, FSimpleDynamicLightingPolicy );

void FTranslucentLightingParameters::Set(FShader* Shader)
{
	SetTextureParameter(
		Shader->GetPixelShader(), 
		TranslucencyLightingVolumeAmbientInner, 
		TranslucencyLightingVolumeAmbientInnerSampler, 
		TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
		GSceneRenderTargets.GetTranslucencyVolumeAmbient(TVC_Inner)->GetRenderTargetItem().ShaderResourceTexture);


	SetTextureParameter(
		Shader->GetPixelShader(), 
		TranslucencyLightingVolumeAmbientOuter, 
		TranslucencyLightingVolumeAmbientOuterSampler, 
		TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
		GSceneRenderTargets.GetTranslucencyVolumeAmbient(TVC_Outer)->GetRenderTargetItem().ShaderResourceTexture);

	SetTextureParameter(
		Shader->GetPixelShader(), 
		TranslucencyLightingVolumeDirectionalInner, 
		TranslucencyLightingVolumeDirectionalInnerSampler, 
		TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
		GSceneRenderTargets.GetTranslucencyVolumeDirectional(TVC_Inner)->GetRenderTargetItem().ShaderResourceTexture);

	SetTextureParameter(
		Shader->GetPixelShader(), 
		TranslucencyLightingVolumeDirectionalOuter, 
		TranslucencyLightingVolumeDirectionalOuterSampler, 
		TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
		GSceneRenderTargets.GetTranslucencyVolumeDirectional(TVC_Outer)->GetRenderTargetItem().ShaderResourceTexture);
}

void FTranslucentLightingParameters::SetMesh(FShader* Shader, const FPrimitiveSceneProxy* Proxy)
{
	FTextureRHIParamRef CubeArrayTexture = GBlackCubeArrayTexture->TextureRHI;
	int32 ArrayIndex = 0;
	const FPrimitiveSceneInfo* PrimitiveSceneInfo = Proxy ? Proxy->GetPrimitiveSceneInfo() : NULL;

	if (PrimitiveSceneInfo && PrimitiveSceneInfo->CachedReflectionCaptureProxy)
	{
		PrimitiveSceneInfo->Scene->GetCaptureParameters(PrimitiveSceneInfo->CachedReflectionCaptureProxy, CubeArrayTexture, ArrayIndex);
	}

	SetTextureParameter(
		Shader->GetPixelShader(), 
		ReflectionCubemap, 
		ReflectionCubemapSampler, 
		TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
		CubeArrayTexture);

	SetShaderValue(Shader->GetPixelShader(), CubemapArrayIndex, ArrayIndex);
}

/** The action used to draw a base pass static mesh element. */
class FDrawBasePassStaticMeshAction
{
public:

	FScene* Scene;
	FStaticMesh* StaticMesh;

	/** Initialization constructor. */
	FDrawBasePassStaticMeshAction(FScene* InScene,FStaticMesh* InStaticMesh):
		Scene(InScene),
		StaticMesh(InStaticMesh)
	{}

	bool UseTranslucentSelfShadowing() const { return false; }
	const FProjectedShadowInfo* GetTranslucentSelfShadow() const { return NULL; }

	bool AllowIndirectLightingCache() const 
	{ 
		return true; 
	}

	bool AllowIndirectLightingCacheVolumeTexture() const
	{
		return true;
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
		TStaticMeshDrawList<TBasePassDrawingPolicy<LightMapPolicyType> >& DrawList =
			Scene->GetBasePassDrawList<LightMapPolicyType>(DrawType);

		// Add the static mesh to the draw list.
		DrawList.AddMesh(
			StaticMesh,
			typename TBasePassDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData),
			TBasePassDrawingPolicy<LightMapPolicyType>(
				StaticMesh->VertexFactory,
				StaticMesh->MaterialRenderProxy,
				*Parameters.Material,
				LightMapPolicy,
				Parameters.BlendMode,
				Parameters.TextureMode,
				Parameters.LightingModel != MLM_Unlit && Scene && Scene->SkyLight,
				IsTranslucentBlendMode(Parameters.BlendMode) && Scene->HasAtmosphericFog()
				)
			);
	}
};

void FBasePassOpaqueDrawingPolicyFactory::AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh)
{
	// Determine the mesh's material and blend mode.
	const FMaterial* Material = StaticMesh->MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Don't composite static meshes
	const bool bEditorCompositeDepthTest = false;

	// Only draw opaque materials.
	if( !IsTranslucentBlendMode(BlendMode) )
	{
		ProcessBasePassMesh(
			FProcessBasePassMeshParameters(
				*StaticMesh,
				Material,
				StaticMesh->PrimitiveSceneInfo->Proxy,
				false,
				bEditorCompositeDepthTest,
				ESceneRenderTargetsMode::DontSet),
			FDrawBasePassStaticMeshAction(Scene,StaticMesh)
			);
	}
}

/** The action used to draw a base pass dynamic mesh element. */
class FDrawBasePassDynamicMeshAction
{
public:

	const FSceneView& View;
	bool bBackFace;
	bool bPreFog;
	FHitProxyId HitProxyId;

	/** Initialization constructor. */
	FDrawBasePassDynamicMeshAction(
		const FSceneView& InView,
		const bool bInBackFace,
		const FHitProxyId InHitProxyId
		)
		: View(InView)
		, bBackFace(bInBackFace)
		, HitProxyId(InHitProxyId)
	{}

	bool UseTranslucentSelfShadowing() const { return false; }
	const FProjectedShadowInfo* GetTranslucentSelfShadow() const { return NULL; }

	bool AllowIndirectLightingCache() const 
	{ 
		return View.Family->EngineShowFlags.IndirectLightingCache; 
	}

	bool AllowIndirectLightingCacheVolumeTexture() const
	{
		return true;
	}

	/** Draws the translucent mesh with a specific light-map type, and shader complexity predicate. */
	template<typename LightMapPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
		const bool bIsLitMaterial = Parameters.LightingModel != MLM_Unlit;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// When rendering masked materials in the shader complexity viewmode, 
		// We want to overwrite complexity for the pixels which get depths written,
		// And accumulate complexity for pixels which get killed due to the opacity mask being below the clip value.
		// This is accomplished by forcing the masked materials to render depths in the depth only pass, 
		// Then rendering in the base pass with additive complexity blending, depth tests on, and depth writes off.
		if(View.Family->EngineShowFlags.ShaderComplexity)
		{
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_GreaterEqual>::GetRHI());
		}
#endif
		const FScene* Scene = Parameters.PrimitiveSceneProxy ? Parameters.PrimitiveSceneProxy->GetPrimitiveSceneInfo()->Scene : NULL;

		TBasePassDrawingPolicy<LightMapPolicyType> DrawingPolicy(
			Parameters.Mesh.VertexFactory,
			Parameters.Mesh.MaterialRenderProxy,
			*Parameters.Material,
			LightMapPolicy,
			Parameters.BlendMode,
			Parameters.TextureMode,
			Scene && Scene->SkyLight && bIsLitMaterial,
			IsTranslucentBlendMode(Parameters.BlendMode) && (Scene && Scene->HasAtmosphericFog()) && View.Family->EngineShowFlags.Atmosphere,
			View.Family->EngineShowFlags.ShaderComplexity,
			false,
			Parameters.bEditorCompositeDepthTest
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
				typename TBasePassDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData)
				);
			DrawingPolicy.DrawMesh(Parameters.Mesh, BatchElementIndex);
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if(View.Family->EngineShowFlags.ShaderComplexity)
		{
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual>::GetRHI());
		}
#endif
	}
};

bool FBasePassOpaqueDrawingPolicyFactory::DrawDynamicMesh(
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
		ProcessBasePassMesh(
			FProcessBasePassMeshParameters(
				Mesh,
				Material,
				PrimitiveSceneProxy,
				!bPreFog,
				DrawingContext.bEditorCompositeDepthTest,
				DrawingContext.TextureMode
				),
			FDrawBasePassDynamicMeshAction(
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

void FCachedVolumeIndirectLightingPolicy::Set(
	const VertexParametersType* VertexShaderParameters,
	const PixelParametersType* PixelShaderParameters,
	FShader* VertexShader,
	FShader* PixelShader,
	const FVertexFactory* VertexFactory,
	const FMaterialRenderProxy* MaterialRenderProxy,
	const FSceneView* View
	) const
{
	FNoLightMapPolicy::Set(VertexShaderParameters, PixelShaderParameters, VertexShader, PixelShader, VertexFactory, MaterialRenderProxy, View);

	if (PixelShaderParameters)
	{
		FTextureRHIParamRef Texture0 = GBlackVolumeTexture->TextureRHI;
		FTextureRHIParamRef Texture1 = GBlackVolumeTexture->TextureRHI;
		FTextureRHIParamRef Texture2 = GBlackVolumeTexture->TextureRHI;

		FScene* Scene = (FScene*)View->Family->Scene;
		FIndirectLightingCache& LightingCache = Scene->IndirectLightingCache;

		// If we are using FCachedVolumeIndirectLightingPolicy then InitViews should have updated the lighting cache which would have initialized it
		// However the conditions for updating the lighting cache are complex and fail very occasionally in non-reproducible ways
		// Silently skipping setting the cache texture under failure for now
		if (LightingCache.IsInitialized())
		{
			Texture0 = LightingCache.GetTexture0().ShaderResourceTexture;
			Texture1 = LightingCache.GetTexture1().ShaderResourceTexture;
			Texture2 = LightingCache.GetTexture2().ShaderResourceTexture;
		}

		const FPixelShaderRHIParamRef PixelShaderRHI = PixelShader->GetPixelShader();

		SetTextureParameter(
			PixelShaderRHI,
			PixelShaderParameters->IndirectLightingCacheTexture0,
			PixelShaderParameters->IndirectLightingCacheSampler0,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			Texture0
			);

		SetTextureParameter(
			PixelShaderRHI,
			PixelShaderParameters->IndirectLightingCacheTexture1,
			PixelShaderParameters->IndirectLightingCacheSampler1,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			Texture1
			);

		SetTextureParameter(
			PixelShaderRHI,
			PixelShaderParameters->IndirectLightingCacheTexture2,
			PixelShaderParameters->IndirectLightingCacheSampler2,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			Texture2
			);
	}
}

void FCachedVolumeIndirectLightingPolicy::SetMesh(
	const FSceneView& View,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const VertexParametersType* VertexShaderParameters,
	const PixelParametersType* PixelShaderParameters,
	FShader* VertexShader,
	FShader* PixelShader,
	const FVertexFactory* VertexFactory,
	const FMaterialRenderProxy* MaterialRenderProxy,
	const ElementDataType& ElementData
	) const
{
	if (PixelShaderParameters)
	{
		FVector AllocationAdd(0, 0, 0);
		FVector AllocationScale(1, 1, 1);
		FVector MinUV(0, 0, 0);
		FVector MaxUV(1, 1, 1);

		if (PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation
			&& PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation->IsValid())
		{
			const FIndirectLightingCacheAllocation& LightingAllocation = *PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation;
			
			AllocationAdd = LightingAllocation.Add;
			AllocationScale = LightingAllocation.Scale;
			MinUV = LightingAllocation.MinUV;
			MaxUV = LightingAllocation.MaxUV;
		}
		
		const FPixelShaderRHIParamRef PixelShaderRHI = PixelShader->GetPixelShader();
		SetShaderValue(PixelShaderRHI, PixelShaderParameters->IndirectlightingCachePrimitiveAdd, AllocationAdd);
		SetShaderValue(PixelShaderRHI, PixelShaderParameters->IndirectlightingCachePrimitiveScale, AllocationScale);
		SetShaderValue(PixelShaderRHI, PixelShaderParameters->IndirectlightingCacheMinUV, MinUV);
		SetShaderValue(PixelShaderRHI, PixelShaderParameters->IndirectlightingCacheMaxUV, MaxUV);
	}
}

void FCachedPointIndirectLightingPolicy::SetMesh(
	const FSceneView& View,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const VertexParametersType* VertexShaderParameters,
	const PixelParametersType* PixelShaderParameters,
	FShader* VertexShader,
	FShader* PixelShader,
	const FVertexFactory* VertexFactory,
	const FMaterialRenderProxy* MaterialRenderProxy,
	const ElementDataType& ElementData
	) const
{
	if (PixelShaderParameters)
	{
		if (PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation
			&& PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation->IsValid()
			&& View.Family->EngineShowFlags.GlobalIllumination)
		{
			const FIndirectLightingCacheAllocation& LightingAllocation = *PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation;

			if (ElementData.bPackAmbientTermInFirstVector)
			{
				// So ambient term is contained in one shader constant
				const FVector4 SwizzledAmbientTerm = 
					FVector4(LightingAllocation.SingleSamplePacked[0].X, LightingAllocation.SingleSamplePacked[1].X, LightingAllocation.SingleSamplePacked[2].X)
					//@todo - why is .5f needed to match directional?
					* FSHVector2::ConstantBasisIntegral * .5f;

				SetShaderValue(
					PixelShader->GetPixelShader(), 
					PixelShaderParameters->IndirectLightingSHCoefficients, 
					SwizzledAmbientTerm);
			}
			else
			{
				SetShaderValueArray(
					PixelShader->GetPixelShader(), 
					PixelShaderParameters->IndirectLightingSHCoefficients, 
					&LightingAllocation.SingleSamplePacked, 
					ARRAY_COUNT(LightingAllocation.SingleSamplePacked));
			}

			SetShaderValue(
				PixelShader->GetPixelShader(), 
				PixelShaderParameters->DirectionalLightShadowing, 
				LightingAllocation.CurrentDirectionalShadowing);
		}
		else
		{
			const FVector4 ZeroArray[sizeof(FSHVectorRGB2) / sizeof(FVector4)] = {FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0)};

			SetShaderValueArray(
				PixelShader->GetPixelShader(), 
				PixelShaderParameters->IndirectLightingSHCoefficients, 
				&ZeroArray, 
				ARRAY_COUNT(ZeroArray));

			SetShaderValue(
				PixelShader->GetPixelShader(), 
				PixelShaderParameters->DirectionalLightShadowing, 
				1);
		}
	}
}

void FSelfShadowedCachedPointIndirectLightingPolicy::SetMesh(
	const FSceneView& View,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const VertexParametersType* VertexShaderParameters,
	const PixelParametersType* PixelShaderParameters,
	FShader* VertexShader,
	FShader* PixelShader,
	const FVertexFactory* VertexFactory,
	const FMaterialRenderProxy* MaterialRenderProxy,
	const ElementDataType& ElementData
	) const
{
	if (PixelShaderParameters)
	{
		if (PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation
			&& PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation->IsValid()
			&& View.Family->EngineShowFlags.GlobalIllumination)
		{
			const FIndirectLightingCacheAllocation& LightingAllocation = *PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation;

			SetShaderValueArray(
				PixelShader->GetPixelShader(), 
				PixelShaderParameters->IndirectLightingSHCoefficients, 
				&LightingAllocation.SingleSamplePacked, 
				ARRAY_COUNT(LightingAllocation.SingleSamplePacked));
		}
		else
		{
			const FVector4 ZeroArray[sizeof(FSHVectorRGB2) / sizeof(FVector4)] = {FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0), FVector4(0, 0, 0, 0)};

			SetShaderValueArray(
				PixelShader->GetPixelShader(), 
				PixelShaderParameters->IndirectLightingSHCoefficients, 
				&ZeroArray, 
				ARRAY_COUNT(ZeroArray));
		}
	}

	FSelfShadowedTranslucencyPolicy::SetMesh(
		View, 
		PrimitiveSceneProxy, 
		VertexShaderParameters,
		PixelShaderParameters,
		VertexShader,
		PixelShader,
		VertexFactory,
		MaterialRenderProxy,
		ElementData);
}