// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BasePassRendering.cpp: Base pass rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PlanarReflectionSceneProxy.h"

// Changing this causes a full shader recompile
static TAutoConsoleVariable<int32> CVarSelectiveBasePassOutputs(
	TEXT("r.SelectiveBasePassOutputs"),
	0,
	TEXT("Enables shaders to only export to relevant rendertargets.\n") \
	TEXT(" 0: Export in all rendertargets.\n") \
	TEXT(" 1: Export only into relevant rendertarget.\n"),
	ECVF_ReadOnly | ECVF_RenderThreadSafe);

// Changing this causes a full shader recompile
static TAutoConsoleVariable<int32> CVarGlobalClipPlane(
	TEXT("r.AllowGlobalClipPlane"),
	0,
	TEXT("Enables mesh shaders to support a global clip plane, needed for planar reflections, which adds about 15% BasePass GPU cost on PS4."),
	ECVF_ReadOnly | ECVF_RenderThreadSafe);

bool UseSelectiveBasePassOutputs()
{
	return CVarSelectiveBasePassOutputs.GetValueOnAnyThread() == 1;
}

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
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassPS##LightMapPolicyName##SkyLightName,TEXT("BasePassPixelShader"),TEXT("MainPS"),SF_Pixel);

// Implement a pixel shader type for skylights and one without, and one vertex shader that will be shared between them
#define IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	IMPLEMENT_BASEPASS_VERTEXSHADER_ONLY_TYPE(LightMapPolicyType,LightMapPolicyName,AtmosphericFog) \
	IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName,true,Skylight) \
	IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,LightMapPolicyName,false,);

// Implement shader types per lightmap policy
// If renaming or refactoring these, remember to update FMaterialResource::GetRepresentativeInstructionCounts and FPreviewMaterial::ShouldCache().
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FSelfShadowedTranslucencyPolicy, FSelfShadowedTranslucencyPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( FSelfShadowedCachedPointIndirectLightingPolicy, FSelfShadowedCachedPointIndirectLightingPolicy );

IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_NO_LIGHTMAP>, FNoLightMapPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_CACHED_VOLUME_INDIRECT_LIGHTING>, FCachedVolumeIndirectLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_CACHED_POINT_INDIRECT_LIGHTING>, FCachedPointIndirectLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_SIMPLE_NO_LIGHTMAP>, FSimpleNoLightmapLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING>, FSimpleLightmapOnlyLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING>, FSimpleDirectionalLightLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING>, FSimpleStationaryLightPrecomputedShadowsLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING>, FSimpleStationaryLightSingleSampleShadowsLightingPolicy );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_LQ_LIGHTMAP>, TLightMapPolicyLQ );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_HQ_LIGHTMAP>, TLightMapPolicyHQ );
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE( TUniformLightMapPolicy<LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP>, TDistanceFieldShadowsAndLightMapPolicyHQ  );

void FSkyLightReflectionParameters::GetSkyParametersFromScene(
	const FScene* Scene, 
	bool bApplySkyLight, 
	FTexture*& OutSkyLightTextureResource, 
	FTexture*& OutSkyLightBlendDestinationTextureResource, 
	float& OutApplySkyLightMask, 
	float& OutSkyMipCount, 
	bool& bOutSkyLightIsDynamic, 
	float& OutBlendFraction)
{
	OutSkyLightTextureResource = GBlackTextureCube;
	OutSkyLightBlendDestinationTextureResource = GBlackTextureCube;
	OutApplySkyLightMask = 0;
	bOutSkyLightIsDynamic = false;
	OutBlendFraction = 0;

	if (Scene
		&& Scene->SkyLight 
		&& Scene->SkyLight->ProcessedTexture
		&& bApplySkyLight)
	{
		const FSkyLightSceneProxy& SkyLight = *Scene->SkyLight;
		OutSkyLightTextureResource = SkyLight.ProcessedTexture;
		OutBlendFraction = SkyLight.BlendFraction;

		if (SkyLight.BlendFraction > 0.0f && SkyLight.BlendDestinationProcessedTexture)
		{
			if (SkyLight.BlendFraction < 1.0f)
			{
				OutSkyLightBlendDestinationTextureResource = SkyLight.BlendDestinationProcessedTexture;
			}
			else
			{
				OutSkyLightTextureResource = SkyLight.BlendDestinationProcessedTexture;
				OutBlendFraction = 0;
			}
		}
		
		OutApplySkyLightMask = 1;
		bOutSkyLightIsDynamic = !SkyLight.bHasStaticLighting && !SkyLight.bWantsStaticShadowing;
	}

	OutSkyMipCount = 1;

	if (OutSkyLightTextureResource)
	{
		int32 CubemapWidth = OutSkyLightTextureResource->GetSizeX();
		OutSkyMipCount = FMath::Log2(CubemapWidth) + 1.0f;
	}
}

void FBasePassReflectionParameters::Set(FRHICommandList& RHICmdList, FPixelShaderRHIParamRef PixelShaderRHI, const FViewInfo* View)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	SkyLightReflectionParameters.SetParameters(RHICmdList, PixelShaderRHI, (const FScene*)(View->Family->Scene), true);
}

void FBasePassReflectionParameters::SetMesh(FRHICommandList& RHICmdList, FPixelShaderRHIParamRef PixelShaderRHI, const FPrimitiveSceneProxy* Proxy, ERHIFeatureLevel::Type FeatureLevel)
{
	const FPrimitiveSceneInfo* PrimitiveSceneInfo = Proxy ? Proxy->GetPrimitiveSceneInfo() : NULL;
	const FPlanarReflectionSceneProxy* ReflectionSceneProxy = NULL;

	if (PrimitiveSceneInfo && PrimitiveSceneInfo->CachedPlanarReflectionProxy)
	{
		ReflectionSceneProxy = PrimitiveSceneInfo->CachedPlanarReflectionProxy;
	}

	PlanarReflectionParameters.SetParameters(RHICmdList, PixelShaderRHI, ReflectionSceneProxy);

	// Note: GBlackCubeArrayTexture has an alpha of 0, which is needed to represent invalid data so the sky cubemap can still be applied
	FTextureRHIParamRef CubeArrayTexture = FeatureLevel >= ERHIFeatureLevel::SM5 ? GBlackCubeArrayTexture->TextureRHI : GBlackTextureCube->TextureRHI;
	int32 ArrayIndex = 0;
	const FReflectionCaptureProxy* ReflectionProxy = PrimitiveSceneInfo ? PrimitiveSceneInfo->CachedReflectionCaptureProxy : nullptr;

	FMatrix BoxTransformVal = FMatrix::Identity;
	FVector4 PositionAndRadius = FVector::ZeroVector;
	FVector4 BoxScalesVal = FVector::ZeroVector;
	FVector CaptureOffsetVal = FVector::ZeroVector;
	EReflectionCaptureShape::Type CaptureShape = EReflectionCaptureShape::Box;
	

	if (PrimitiveSceneInfo && ReflectionProxy)
	{
		PrimitiveSceneInfo->Scene->GetCaptureParameters(ReflectionProxy, CubeArrayTexture, ArrayIndex);
		PositionAndRadius = FVector4(ReflectionProxy->Position, ReflectionProxy->InfluenceRadius);
		CaptureShape = ReflectionProxy->Shape;
		BoxTransformVal = ReflectionProxy->BoxTransform;
		BoxScalesVal = ReflectionProxy->BoxScales;
		CaptureOffsetVal = ReflectionProxy->CaptureOffset;
	}

	SetTextureParameter(
		RHICmdList, 
		PixelShaderRHI, 
		ReflectionCubemap, 
		ReflectionCubemapSampler, 
		TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
		CubeArrayTexture);

	SetShaderValue(RHICmdList, PixelShaderRHI, CubemapArrayIndex, ArrayIndex);
	SetShaderValue(RHICmdList, PixelShaderRHI, ReflectionPositionAndRadius, PositionAndRadius);
	SetShaderValue(RHICmdList, PixelShaderRHI, ReflectionShape, (float)CaptureShape);
	SetShaderValue(RHICmdList, PixelShaderRHI, BoxTransform, BoxTransformVal);
	SetShaderValue(RHICmdList, PixelShaderRHI, BoxScales, BoxScalesVal);
	SetShaderValue(RHICmdList, PixelShaderRHI, CaptureOffset, CaptureOffsetVal);
	
}

void FTranslucentLightingParameters::Set(FRHICommandList& RHICmdList, FPixelShaderRHIParamRef PixelShaderRHI, const FViewInfo* View)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	TranslucentLightingVolumeParameters.Set(RHICmdList, PixelShaderRHI);

	if (View->HZB)
	{
		SetTextureParameter(
			RHICmdList, 
			PixelShaderRHI, 
			HZBTexture, 
			HZBSampler, 
			TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			View->HZB->GetRenderTargetItem().ShaderResourceTexture );

		TRefCountPtr<IPooledRenderTarget>* PrevSceneColorRT = &GSystemTextures.BlackDummy;

		FSceneViewState* ViewState = (FSceneViewState*)View->State;
		if( ViewState && ViewState->TemporalAAHistoryRT && !View->bCameraCut )
		{
			PrevSceneColorRT = &ViewState->TemporalAAHistoryRT;
		}

		SetTextureParameter(
			RHICmdList, 
			PixelShaderRHI, 
			PrevSceneColor, 
			PrevSceneColorSampler, 
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			(*PrevSceneColorRT)->GetRenderTargetItem().ShaderResourceTexture );
		
		const FVector2D HZBUvFactor(
			float(View->ViewRect.Width()) / float(2 * View->HZBMipmap0Size.X),
			float(View->ViewRect.Height()) / float(2 * View->HZBMipmap0Size.Y)
			);
		const FVector4 HZBUvFactorAndInvFactorValue(
			HZBUvFactor.X,
			HZBUvFactor.Y,
			1.0f / HZBUvFactor.X,
			1.0f / HZBUvFactor.Y
			);
			
		SetShaderValue(RHICmdList, PixelShaderRHI, HZBUvFactorAndInvFactor, HZBUvFactorAndInvFactorValue);
	}
	else
	{
		//set dummies for platforms that require bound resources.
		SetTextureParameter(
			RHICmdList,
			PixelShaderRHI,
			HZBTexture,
			HZBSampler,
			TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(),
			GBlackTexture->TextureRHI);

		SetTextureParameter(
			RHICmdList,
			PixelShaderRHI,
			PrevSceneColor,
			PrevSceneColorSampler,
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(),
			GBlackTexture->TextureRHI);
	}
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
		// Note: can't disallow based on presence of PrecomputedLightVolumes in the scene as this is registration time
		// Unless extra handling is added to recreate static draw lists when new volumes are added
		return true; 
	}

	bool AllowIndirectLightingCacheVolumeTexture() const
	{
		return true;
	}

	/** Draws the mesh with a specific light-map type */
	template<typename LightMapPolicyType>
	void Process(
		FRHICommandList& RHICmdList,
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
		EBasePassDrawListType DrawType = EBasePass_Default;

		if (StaticMesh->IsMasked(Parameters.FeatureLevel))
		{
			DrawType = EBasePass_Masked;
		}

		if (Scene)
		{
			// Find the appropriate draw list for the static mesh based on the light-map policy type.
			TStaticMeshDrawList<TBasePassDrawingPolicy<LightMapPolicyType> >& DrawList =
				Scene->GetBasePassDrawList<LightMapPolicyType>(DrawType);

			const bool bRenderSkylight = Parameters.ShadingModel != MSM_Unlit
				&& Scene->SkyLight 
				&& !Scene->SkyLight->bHasStaticLighting 
				// The deferred shading renderer does movable skylight diffuse in a later deferred pass, not in the base pass
				&& (Scene->SkyLight->bWantsStaticShadowing || IsSimpleForwardShadingEnabled(Scene->GetShaderPlatform()));

			// Add the static mesh to the draw list.
			DrawList.AddMesh(
				StaticMesh,
				typename TBasePassDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData),
				TBasePassDrawingPolicy<LightMapPolicyType>(
					StaticMesh->VertexFactory,
					StaticMesh->MaterialRenderProxy,
					*Parameters.Material,
					Parameters.FeatureLevel,
					LightMapPolicy,
					Parameters.BlendMode,
					Parameters.TextureMode,
					bRenderSkylight,
					IsTranslucentBlendMode(Parameters.BlendMode) && Scene->HasAtmosphericFog(),
					/* DebugViewShaderMode = */ DVSM_None,
					/* bInAllowGlobalFog = */ false,
					/* bInEnableEditorPrimitiveDepthTest = */ false,
					/* bInEnableReceiveDecalOutput = */ true
				),
				Scene->GetFeatureLevel()
				);
		}
	}
};

void FBasePassOpaqueDrawingPolicyFactory::AddStaticMesh(FRHICommandList& RHICmdList, FScene* Scene, FStaticMesh* StaticMesh)
{
	// Determine the mesh's material and blend mode.
	const FMaterial* Material = StaticMesh->MaterialRenderProxy->GetMaterial(Scene->GetFeatureLevel());
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Don't composite static meshes
	const bool bEditorCompositeDepthTest = false;

	// Only draw opaque materials.
	if( !IsTranslucentBlendMode(BlendMode) )
	{
		ProcessBasePassMesh(
			RHICmdList, 
			FProcessBasePassMeshParameters(
				*StaticMesh,
				Material,
				StaticMesh->PrimitiveSceneInfo->Proxy,
				false,
				bEditorCompositeDepthTest,
				ESceneRenderTargetsMode::DontSet,
				Scene->GetFeatureLevel()),
			FDrawBasePassStaticMeshAction(Scene,StaticMesh)
			);
	}
}

/** The action used to draw a base pass dynamic mesh element. */
class FDrawBasePassDynamicMeshAction
{
public:

	const FViewInfo& View;
	bool bBackFace;
	float DitheredLODTransitionAlpha;
	bool bPreFog;
	FHitProxyId HitProxyId;

	/** Initialization constructor. */
	FDrawBasePassDynamicMeshAction(
		const FViewInfo& InView,
		const bool bInBackFace,
		float InDitheredLODTransitionValue,
		const FHitProxyId InHitProxyId
		)
		: View(InView)
		, bBackFace(bInBackFace)
		, DitheredLODTransitionAlpha(InDitheredLODTransitionValue)
		, HitProxyId(InHitProxyId)
	{}

	bool UseTranslucentSelfShadowing() const { return false; }
	const FProjectedShadowInfo* GetTranslucentSelfShadow() const { return NULL; }

	bool AllowIndirectLightingCache() const 
	{ 
		const FScene* Scene = (const FScene*)View.Family->Scene;
		return View.Family->EngineShowFlags.IndirectLightingCache && Scene && Scene->PrecomputedLightVolumes.Num() > 0;
	}

	bool AllowIndirectLightingCacheVolumeTexture() const
	{
		return true;
	}

	/** Draws the translucent mesh with a specific light-map type, and shader complexity predicate. */
	template<typename LightMapPolicyType>
	void Process(
		FRHICommandList& RHICmdList, 
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
		const bool bIsLitMaterial = Parameters.ShadingModel != MSM_Unlit;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// When rendering masked materials in the shader complexity viewmode, 
		// We want to overwrite complexity for the pixels which get depths written,
		// And accumulate complexity for pixels which get killed due to the opacity mask being below the clip value.
		// This is accomplished by forcing the masked materials to render depths in the depth only pass, 
		// Then rendering in the base pass with additive complexity blending, depth tests on, and depth writes off.
		if(View.Family->EngineShowFlags.ShaderComplexity)
		{
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false,CF_DepthNearOrEqual>::GetRHI());
		}

		const FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
#endif
		const FScene* Scene = Parameters.PrimitiveSceneProxy ? Parameters.PrimitiveSceneProxy->GetPrimitiveSceneInfo()->Scene : NULL;

		const bool bRenderSkylight = Scene && Scene->SkyLight 
			&& !Scene->SkyLight->bHasStaticLighting 
			// The deferred shading renderer does movable skylight diffuse in a later deferred pass, not in the base pass
			&& (Scene->SkyLight->bWantsStaticShadowing || IsSimpleForwardShadingEnabled(Scene->GetShaderPlatform()))
			&& bIsLitMaterial;

		TBasePassDrawingPolicy<LightMapPolicyType> DrawingPolicy(
			Parameters.Mesh.VertexFactory,
			Parameters.Mesh.MaterialRenderProxy,
			*Parameters.Material,
			Parameters.FeatureLevel,
			LightMapPolicy,
			Parameters.BlendMode,
			Parameters.TextureMode,
			bRenderSkylight,
			IsTranslucentBlendMode(Parameters.BlendMode) && (Scene && Scene->HasAtmosphericFog()) && View.Family->EngineShowFlags.AtmosphericFog,
			View.Family->GetDebugViewShaderMode(),
			false,
			Parameters.bEditorCompositeDepthTest,
			/* bInEnableReceiveDecalOutput = */ Scene != nullptr
			);
		RHICmdList.BuildAndSetLocalBoundShaderState(DrawingPolicy.GetBoundShaderStateInput(View.GetFeatureLevel()));
		DrawingPolicy.SetSharedState(RHICmdList, &View, typename TBasePassDrawingPolicy<LightMapPolicyType>::ContextDataType(Parameters.bIsInstancedStereo));
		const FMeshDrawingRenderState DrawRenderState(DitheredLODTransitionAlpha);

		for( int32 BatchElementIndex = 0, Num = Parameters.Mesh.Elements.Num(); BatchElementIndex < Num; BatchElementIndex++ )
		{
			// We draw instanced static meshes twice when rendering with instanced stereo. Once for each eye.
			const bool bIsInstancedMesh = Parameters.Mesh.Elements[BatchElementIndex].bIsInstancedMesh;
			const uint32 InstancedStereoDrawCount = (Parameters.bIsInstancedStereo && bIsInstancedMesh) ? 2 : 1;
			for (uint32 DrawCountIter = 0; DrawCountIter < InstancedStereoDrawCount; ++DrawCountIter)
			{
				DrawingPolicy.SetInstancedEyeIndex(RHICmdList, DrawCountIter);

				TDrawEvent<FRHICommandList> MeshEvent;
				BeginMeshDrawEvent(RHICmdList, Parameters.PrimitiveSceneProxy, Parameters.Mesh, MeshEvent);

				DrawingPolicy.SetMeshRenderState(
					RHICmdList,
					View,
					Parameters.PrimitiveSceneProxy,
					Parameters.Mesh,
					BatchElementIndex,
					bBackFace,
					DrawRenderState,
					typename TBasePassDrawingPolicy<LightMapPolicyType>::ElementDataType(LightMapElementData),
					typename TBasePassDrawingPolicy<LightMapPolicyType>::ContextDataType()
					);
				DrawingPolicy.DrawMesh(RHICmdList, Parameters.Mesh, BatchElementIndex, Parameters.bIsInstancedStereo);
			}
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if(View.Family->EngineShowFlags.ShaderComplexity)
		{
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<true,CF_DepthNearOrEqual>::GetRHI());
		}
#endif
	}
};

bool FBasePassOpaqueDrawingPolicyFactory::DrawDynamicMesh(
	FRHICommandList& RHICmdList, 
	const FViewInfo& View,
	ContextType DrawingContext,
	const FMeshBatch& Mesh,
	bool bBackFace,
	bool bPreFog,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId, 
	const bool bIsInstancedStereo
	)
{
	// Determine the mesh's material and blend mode.
	const FMaterial* Material = Mesh.MaterialRenderProxy->GetMaterial(View.GetFeatureLevel());
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Only draw opaque materials.
	if(!IsTranslucentBlendMode(BlendMode))
	{
		ProcessBasePassMesh(
			RHICmdList, 
			FProcessBasePassMeshParameters(
				Mesh,
				Material,
				PrimitiveSceneProxy,
				!bPreFog,
				DrawingContext.bEditorCompositeDepthTest,
				DrawingContext.TextureMode,
				View.GetFeatureLevel(), 
				bIsInstancedStereo
				),
			FDrawBasePassDynamicMeshAction(
				View,
				bBackFace,
				Mesh.DitheredLODTransitionAlpha,
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

void FSelfShadowedCachedPointIndirectLightingPolicy::SetMesh(
	FRHICommandList& RHICmdList, 
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
		FUniformBufferRHIParamRef PrecomputedLightingBuffer = GEmptyPrecomputedLightingUniformBuffer.GetUniformBufferRHI();;
		if (View.Family->EngineShowFlags.GlobalIllumination && PrimitiveSceneProxy && PrimitiveSceneProxy->GetPrimitiveSceneInfo())
		{
			PrecomputedLightingBuffer = PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheUniformBuffer;
		}

		if (PixelShaderParameters->BufferParameter.IsBound())
		{
			SetUniformBufferParameter(RHICmdList, PixelShader->GetPixelShader(), PixelShaderParameters->BufferParameter, PrecomputedLightingBuffer);
		}
	}

	FSelfShadowedTranslucencyPolicy::SetMesh(
		RHICmdList, 
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
/**
 * Get shader templates allowing to redirect between compatible shaders.
 */

template <ELightMapPolicyType Policy>
void GetUniformBasePassShaders(
	const FMaterial& Material, 
	FVertexFactoryType* VertexFactoryType, 
	bool bNeedsHSDS,
	bool bEnableAtmosphericFog,
	bool bEnableSkyLight,
	FBaseHS*& HullShader,
	FBaseDS*& DomainShader,
	TBasePassVertexShaderPolicyParamType<FUniformLightMapPolicyShaderParametersType>*& VertexShader,
	TBasePassPixelShaderPolicyParamType<FUniformLightMapPolicyShaderParametersType>*& PixelShader
	)
{
	if (bNeedsHSDS)
	{
		HullShader = Material.GetShader<TBasePassHS<TUniformLightMapPolicy<Policy> > >(VertexFactoryType);
		DomainShader = Material.GetShader<TBasePassDS<TUniformLightMapPolicy<Policy> > >(VertexFactoryType);
	}

	if (bEnableAtmosphericFog)
	{
		VertexShader = Material.GetShader<TBasePassVS<TUniformLightMapPolicy<Policy>, true> >(VertexFactoryType);
	}
	else
	{
		VertexShader = Material.GetShader<TBasePassVS<TUniformLightMapPolicy<Policy>, false> >(VertexFactoryType);
	}
	if (bEnableSkyLight)
	{
		PixelShader = Material.GetShader<TBasePassPS<TUniformLightMapPolicy<Policy>, true> >(VertexFactoryType);
	}
	else
	{
		PixelShader = Material.GetShader<TBasePassPS<TUniformLightMapPolicy<Policy>, false> >(VertexFactoryType);
	}
}

template <>
void GetBasePassShaders<FUniformLightMapPolicy>(
	const FMaterial& Material, 
	FVertexFactoryType* VertexFactoryType, 
	FUniformLightMapPolicy LightMapPolicy, 
	bool bNeedsHSDS,
	bool bEnableAtmosphericFog,
	bool bEnableSkyLight,
	FBaseHS*& HullShader,
	FBaseDS*& DomainShader,
	TBasePassVertexShaderPolicyParamType<FUniformLightMapPolicyShaderParametersType>*& VertexShader,
	TBasePassPixelShaderPolicyParamType<FUniformLightMapPolicyShaderParametersType>*& PixelShader
	)
{
	switch (LightMapPolicy.GetIndirectPolicy())
	{
	case LMP_CACHED_VOLUME_INDIRECT_LIGHTING:
		GetUniformBasePassShaders<LMP_CACHED_VOLUME_INDIRECT_LIGHTING>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_CACHED_POINT_INDIRECT_LIGHTING:
		GetUniformBasePassShaders<LMP_CACHED_POINT_INDIRECT_LIGHTING>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING:
		GetUniformBasePassShaders<LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_SIMPLE_NO_LIGHTMAP:
		GetUniformBasePassShaders<LMP_SIMPLE_NO_LIGHTMAP>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING:
		GetUniformBasePassShaders<LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING:
		GetUniformBasePassShaders<LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING:
		GetUniformBasePassShaders<LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_LQ_LIGHTMAP:
		GetUniformBasePassShaders<LMP_LQ_LIGHTMAP>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_HQ_LIGHTMAP:
		GetUniformBasePassShaders<LMP_HQ_LIGHTMAP>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	case LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP:
		GetUniformBasePassShaders<LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	default:										
		check(false);
	case LMP_NO_LIGHTMAP:
		GetUniformBasePassShaders<LMP_NO_LIGHTMAP>(Material, VertexFactoryType, bNeedsHSDS, bEnableAtmosphericFog, bEnableSkyLight, HullShader, DomainShader, VertexShader, PixelShader);
		break;
	}
}
