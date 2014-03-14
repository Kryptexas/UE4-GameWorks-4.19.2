// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LightRendering.cpp: Light rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "LightRendering.h"
#include "LightPropagationVolume.h"

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FDeferredLightUniformStruct,TEXT("DeferredLightUniforms"));

extern int32 GUseTranslucentLightingVolumes;

// Implement a version for directional lights, and a version for point / spot lights
IMPLEMENT_SHADER_TYPE(template<>,TDeferredLightVS<false>,TEXT("DeferredLightVertexShaders"),TEXT("DirectionalVertexMain"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>,TDeferredLightVS<true>,TEXT("DeferredLightVertexShaders"),TEXT("RadialVertexMain"),SF_Vertex);

/** A pixel shader for rendering the light in a deferred pass. */
template<bool bUseIESProfile, bool bRadialAttenuation, bool bInverseSquaredFalloff, bool bVisualizeLightCulling>
class TDeferredLightPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TDeferredLightPS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_IES_PROFILE"), (uint32)bUseIESProfile);
		OutEnvironment.SetDefine(TEXT("RADIAL_ATTENUATION"), (uint32)bRadialAttenuation);
		OutEnvironment.SetDefine(TEXT("INVERSE_SQUARED_FALLOFF"), (uint32)bInverseSquaredFalloff);
		OutEnvironment.SetDefine(TEXT("LIGHT_SOURCE_SHAPE"), 1);
		OutEnvironment.SetDefine(TEXT("VISUALIZE_LIGHT_CULLING"), (uint32)bVisualizeLightCulling);
	}

	TDeferredLightPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	:	FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		LightAttenuationTexture.Bind(Initializer.ParameterMap, TEXT("LightAttenuationTexture"));
		LightAttenuationTextureSampler.Bind(Initializer.ParameterMap, TEXT("LightAttenuationTextureSampler"));
		PreIntegratedBRDF.Bind(Initializer.ParameterMap, TEXT("PreIntegratedBRDF"));
		PreIntegratedBRDFSampler.Bind(Initializer.ParameterMap, TEXT("PreIntegratedBRDFSampler"));
		IESTexture.Bind(Initializer.ParameterMap, TEXT("IESTexture"));
		IESTextureSampler.Bind(Initializer.ParameterMap, TEXT("IESTextureSampler"));
	}

	TDeferredLightPS()
	{
	}

	void SetParameters(const FSceneView& View, const FLightSceneInfo* LightSceneInfo)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		SetParametersBase(ShaderRHI, View, LightSceneInfo->Proxy->GetIESTextureResource());
		SetDeferredLightParameters(ShaderRHI, GetUniformBufferParameter<FDeferredLightUniformStruct>(), LightSceneInfo, View);
	}

	void SetParametersSimpleLight(const FSceneView& View, const FSimpleLightEntry& SimpleLight)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		SetParametersBase(ShaderRHI, View, NULL);
		SetSimpleDeferredLightParameters(ShaderRHI, GetUniformBufferParameter<FDeferredLightUniformStruct>(), SimpleLight, View);
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << LightAttenuationTexture;
		Ar << LightAttenuationTextureSampler;
		Ar << PreIntegratedBRDF;
		Ar << PreIntegratedBRDFSampler;
		Ar << IESTexture;
		Ar << IESTextureSampler;
		return bShaderHasOutdatedParameters;
	}

	FGlobalBoundShaderState& GetBoundShaderState()
	{
		static FGlobalBoundShaderState State;

		return State;
	}

private:

	void SetParametersBase(const FPixelShaderRHIParamRef ShaderRHI, const FSceneView& View, FTexture* IESTextureResource)
	{
		FGlobalShader::SetParameters(ShaderRHI,View);
		DeferredParameters.Set(ShaderRHI, View);

		if(LightAttenuationTexture.IsBound())
		{
			SetTextureParameter(
				ShaderRHI,
				LightAttenuationTexture,
				LightAttenuationTextureSampler,
				TStaticSamplerState<SF_Point,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
				GSceneRenderTargets.GetEffectiveLightAttenuationTexture(true)
				);
		}

		SetTextureParameter(
			ShaderRHI,
			PreIntegratedBRDF,
			PreIntegratedBRDFSampler,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GEngine->PreIntegratedSkinBRDFTexture->Resource->TextureRHI
			);

		{
			FTextureRHIParamRef TextureRHI = IESTextureResource ? IESTextureResource->TextureRHI : GSystemTextures.WhiteDummy->GetRenderTargetItem().TargetableTexture;

			SetTextureParameter(
				ShaderRHI,
				IESTexture,
				IESTextureSampler,
				TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				TextureRHI
				);
		}
	}

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter LightAttenuationTexture;
	FShaderResourceParameter LightAttenuationTextureSampler;
	FShaderResourceParameter PreIntegratedBRDF;
	FShaderResourceParameter PreIntegratedBRDFSampler;
	FShaderResourceParameter IESTexture;
	FShaderResourceParameter IESTextureSampler;
};

// Typedef is necessary because the C preprocessor thinks the comma in the template parameter list is a comma in the macro parameter list.
#define IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(A, B, C, D, EntryName) \
	typedef TDeferredLightPS<A,B,C,D> TDeferredLightPS##A##B##C##D; \
	IMPLEMENT_SHADER_TYPE(template<>,TDeferredLightPS##A##B##C##D,TEXT("DeferredLightPixelShaders"),EntryName,SF_Pixel);

// Implement a version for each light type, and it's shader permutations
IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(true, true, true, false, TEXT("RadialPixelMain"));
IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(true, true, false, false, TEXT("RadialPixelMain"));
IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(true, false, false, false, TEXT("DirectionalPixelMain"));
IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(false, true, true, false, TEXT("RadialPixelMain"));
IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(false, true, false, false, TEXT("RadialPixelMain"));
IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(false, false, false, false, TEXT("DirectionalPixelMain"));

IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(false, false, false, true, TEXT("DirectionalPixelMain"));
IMPLEMENT_DEFERREDLIGHT_PIXELSHADER_TYPE(false, true, false, true, TEXT("RadialPixelMain"));

/** Shader used to visualize stationary light overlap. */
template<bool bRadialAttenuation>
class TDeferredLightOverlapPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TDeferredLightOverlapPS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RADIAL_ATTENUATION"), (uint32)bRadialAttenuation);
	}

	TDeferredLightOverlapPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	:	FGlobalShader(Initializer)
	{
		HasValidChannel.Bind(Initializer.ParameterMap, TEXT("HasValidChannel"));
		DeferredParameters.Bind(Initializer.ParameterMap);
	}

	TDeferredLightOverlapPS()
	{
	}

	void SetParameters(const FSceneView& View, const FLightSceneInfo* LightSceneInfo)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(ShaderRHI,View);
		const float HasValidChannelValue = LightSceneInfo->Proxy->GetPreviewShadowMapChannel() == INDEX_NONE ? 0.0f : 1.0f;
		SetShaderValue(ShaderRHI, HasValidChannel, HasValidChannelValue);
		DeferredParameters.Set(ShaderRHI, View);
		SetDeferredLightParameters(ShaderRHI, GetUniformBufferParameter<FDeferredLightUniformStruct>(), LightSceneInfo, View);
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << HasValidChannel;
		Ar << DeferredParameters;
		return bShaderHasOutdatedParameters;
	}

	FGlobalBoundShaderState& GetBoundShaderState()
	{
		static FGlobalBoundShaderState State;

		return State;
	}

private:
	FShaderParameter HasValidChannel;
	FDeferredPixelShaderParameters DeferredParameters;
};

IMPLEMENT_SHADER_TYPE(template<>, TDeferredLightOverlapPS<true>, TEXT("StationaryLightOverlapShaders"), TEXT("OverlapRadialPixelMain"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, TDeferredLightOverlapPS<false>, TEXT("StationaryLightOverlapShaders"), TEXT("OverlapDirectionalPixelMain"), SF_Pixel);

/** Gathers simple lights from visible primtives in the passed in views. */
void GatherSimpleLights(const TArray<FViewInfo>& Views, TArray<FSimpleLightEntry, SceneRenderingAllocator>& SimpleLights)
{
	TArray<const FPrimitiveSceneInfo*, SceneRenderingAllocator> PrimitivesWithSimpleLights;

	// Gather visible primitives from all views that might have simple lights
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];
		for (int32 PrimitiveIndex = 0; PrimitiveIndex < View.VisibleDynamicPrimitives.Num(); PrimitiveIndex++)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives[PrimitiveIndex];
			const int32 PrimitiveId = PrimitiveSceneInfo->GetIndex();
			const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveId];

			if (PrimitiveViewRelevance.bHasSimpleLights)
			{
				// TArray::AddUnique is slow, but not expecting many entries in PrimitivesWithSimpleLights
				PrimitivesWithSimpleLights.AddUnique(PrimitiveSceneInfo);
			}
		}
	}

	// Gather simple lights from the primitives
	for (int32 PrimitiveIndex = 0; PrimitiveIndex < PrimitivesWithSimpleLights.Num(); PrimitiveIndex++)
	{
		const FPrimitiveSceneInfo* Primitive = PrimitivesWithSimpleLights[PrimitiveIndex];
		Primitive->Proxy->GatherSimpleLights(SimpleLights);
	}
}

/** Gets a readable light name for use with a draw event. */
void GetLightNameForDrawEvent(const FLightSceneProxy* LightProxy, FString& LightNameWithLevel)
{
#if WANTS_DRAW_MESH_EVENTS
	if (GEmitDrawEvents)
	{
		FString FullLevelName = LightProxy->GetLevelName().ToString();
		const int32 LastSlashIndex = FullLevelName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);

		if (LastSlashIndex != INDEX_NONE)
		{
			// Trim the leading path before the level name to make it more readable
			// The level FName was taken directly from the Outermost UObject, otherwise we would do this operation on the game thread
			FullLevelName = FullLevelName.Mid(LastSlashIndex + 1, FullLevelName.Len() - (LastSlashIndex + 1));
		}

		LightNameWithLevel = FullLevelName + TEXT(".") + LightProxy->GetComponentName().ToString();
	}
#endif
}

uint32 GetShadowQuality();

/** Renders the scene's lighting. */
void FDeferredShadingSceneRenderer::RenderLights()
{
	SCOPED_DRAW_EVENT(Lights, DEC_SCENE_ITEMS);

	if(IsSimpleDynamicLightingEnabled())
	{
		return;
	}

	bool bStencilBufferDirty = false;	// The stencil buffer should've been cleared to 0 already

	SCOPE_CYCLE_COUNTER(STAT_LightingDrawTime);
	SCOPE_CYCLE_COUNTER(STAT_LightRendering);

	TArray<FSimpleLightEntry, SceneRenderingAllocator> SimpleLights;
	GatherSimpleLights(Views, SimpleLights);

	TArray<FSortedLightSceneInfo, SceneRenderingAllocator> SortedLights;
	SortedLights.Empty(Scene->Lights.Num());

	bool bDynamicShadows = ViewFamily.EngineShowFlags.DynamicShadows && GetShadowQuality() > 0;
	
	// Build a list of visible lights.
	for (TSparseArray<FLightSceneInfoCompact>::TConstIterator LightIt(Scene->Lights); LightIt; ++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		const FLightSceneInfo* const LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		if (!LightSceneInfoCompact.Color.IsAlmostBlack()
			// Only render lights with dynamic lighting or unbuilt static lights
			&& (!LightSceneInfo->Proxy->HasStaticLighting() || !LightSceneInfo->bPrecomputedLightingIsValid)
			// Reflection override skips direct specular because it tends to be blindingly bright with a perfectly smooth surface
			&& !ViewFamily.EngineShowFlags.ReflectionOverride)
		{
			// Check if the light is visible in any of the views.
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (LightSceneInfo->ShouldRenderLight(Views[ViewIndex]))
				{
					FSortedLightSceneInfo* SortedLightInfo = new(SortedLights) FSortedLightSceneInfo(LightSceneInfoCompact);

					// Check for shadows and light functions.
					SortedLightInfo->SortKey.Fields.LightType = LightSceneInfoCompact.LightType;
					SortedLightInfo->SortKey.Fields.bTextureProfile = ViewFamily.EngineShowFlags.TexturedLightProfiles && LightSceneInfo->Proxy->GetIESTextureResource();
					SortedLightInfo->SortKey.Fields.bShadowed = bDynamicShadows && CheckForProjectedShadows(LightSceneInfo);
					SortedLightInfo->SortKey.Fields.bLightFunction = ViewFamily.EngineShowFlags.LightFunctions && CheckForLightFunction(LightSceneInfo);
					break;
				}
			}
		}
	}

	// Sort non-shadowed, non-light function lights first to avoid render target switches.
	struct FCompareFSortedLightSceneInfo
	{
		FORCEINLINE bool operator()( const FSortedLightSceneInfo& A, const FSortedLightSceneInfo& B ) const
		{
			return A.SortKey.Packed < B.SortKey.Packed;
		}
	};
	SortedLights.Sort( FCompareFSortedLightSceneInfo() );

	{
		SCOPED_DRAW_EVENT(DirectLighting, DEC_SCENE_ITEMS);

		int32 AttenuationLightStart = SortedLights.Num();
		int32 SupportedByTiledDeferredLightEnd = SortedLights.Num();
		bool bAnyUnsupportedByTiledDeferred = false;

		// Iterate over all lights to be rendered and build ranges for tiled deferred and unshadowed lights
		for (int32 LightIndex = 0; LightIndex < SortedLights.Num(); LightIndex++)
		{
			const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
			bool bDrawShadows = SortedLightInfo.SortKey.Fields.bShadowed;
			bool bDrawLightFunction = SortedLightInfo.SortKey.Fields.bLightFunction;
			bool bTextureLightProfile = SortedLightInfo.SortKey.Fields.bTextureProfile;

			if (bTextureLightProfile && SupportedByTiledDeferredLightEnd == SortedLights.Num())
			{
				// Mark the first index to not support tiled deferred due to texture light profile
				SupportedByTiledDeferredLightEnd = LightIndex;
			}

			if( bDrawShadows || bDrawLightFunction )
			{
				AttenuationLightStart = LightIndex;

				if (SupportedByTiledDeferredLightEnd == SortedLights.Num())
				{
					// Mark the first index to not support tiled deferred due to shadowing
					SupportedByTiledDeferredLightEnd = LightIndex;
				}
				break;
			}

			if (LightIndex < SupportedByTiledDeferredLightEnd)
			{
				// Directional lights currently not supported by tiled deferred
				bAnyUnsupportedByTiledDeferred = bAnyUnsupportedByTiledDeferred 
					|| (SortedLightInfo.SortKey.Fields.LightType != LightType_Point && SortedLightInfo.SortKey.Fields.LightType != LightType_Spot);
			}
		}
		
		if(ViewFamily.EngineShowFlags.DirectLighting)
		{
			SCOPED_DRAW_EVENT(NonShadowedLights, DEC_SCENE_ITEMS);
			INC_DWORD_STAT_BY(STAT_NumUnshadowedLights, AttenuationLightStart);
			GSceneRenderTargets.SetLightAttenuationMode(false);

			int32 StandardDeferredStart = 0;

			if (CanUseTiledDeferred())
			{
				int32 NumSortedLightsTiledDeferred = SupportedByTiledDeferredLightEnd;

				bool bAnyViewIsStereo = false;
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
				{
					if (Views[ViewIndex].StereoPass != eSSP_FULL)
					{
						bAnyViewIsStereo = true;
						break;
					}
				}

				// Use tiled deferred shading on any unshadowed lights without a texture light profile
				if (!ShouldUseTiledDeferred(SupportedByTiledDeferredLightEnd, SimpleLights.Num()) || bAnyUnsupportedByTiledDeferred || bAnyViewIsStereo)
				{
					NumSortedLightsTiledDeferred = 0;
				}

				if (NumSortedLightsTiledDeferred > 0 || SimpleLights.Num() > 0)
				{
					// Update the range that needs to be processed by standard deferred to exclude the lights done with tiled
					StandardDeferredStart = NumSortedLightsTiledDeferred;
					RenderTiledDeferredLighting(SortedLights, NumSortedLightsTiledDeferred, SimpleLights);
				}
			}
			else if (SimpleLights.Num() > 0)
			{
				GSceneRenderTargets.BeginRenderingSceneColor();
				RenderSimpleLightsStandardDeferred(SimpleLights);
			}

			{
				SCOPED_DRAW_EVENT(StandardDeferredLighting, DEC_SCENE_ITEMS);

				GSceneRenderTargets.BeginRenderingSceneColor();

				// Draw non-shadowed non-light function lights without changing render targets between them
				for (int32 LightIndex = StandardDeferredStart; LightIndex < AttenuationLightStart; LightIndex++)
				{
					const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
					const FLightSceneInfoCompact& LightSceneInfoCompact = SortedLightInfo.SceneInfo;
					const FLightSceneInfo* const LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

					// Render the light to the scene color buffer, using a 1x1 white texture as input 
					RenderLight( LightSceneInfo, false, false );
				}
			}

			if (GUseTranslucentLightingVolumes)
			{
				if (AttenuationLightStart)
				{
					// Inject non-shadowed, non-light function lights in to the volume.
					SCOPED_DRAW_EVENT(InjectNonShadowedTranslucentLighting, DEC_SCENE_ITEMS);
					InjectTranslucentVolumeLightingArray(SortedLights, AttenuationLightStart);
				}
				
				if (SimpleLights.Num() > 0)
				{
					SCOPED_DRAW_EVENT(InjectSimpleLightsTranslucentLighting, DEC_SCENE_ITEMS);
					InjectSimpleTranslucentVolumeLightingArray(SimpleLights);
				}
			}
		}

		bool bDirectLighting = ViewFamily.EngineShowFlags.DirectLighting;

		// Draw shadowed and light function lights
		for (int32 LightIndex = AttenuationLightStart; LightIndex < SortedLights.Num(); LightIndex++)
		{
			const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
			const FLightSceneInfoCompact& LightSceneInfoCompact = SortedLightInfo.SceneInfo;
			const FLightSceneInfo& LightSceneInfo = *LightSceneInfoCompact.LightSceneInfo;
			bool bDrawShadows = SortedLightInfo.SortKey.Fields.bShadowed;
			bool bDrawLightFunction = SortedLightInfo.SortKey.Fields.bLightFunction;
			bool bInjectedTranslucentVolume = false;
			FScopeCycleCounter Context(LightSceneInfo.Proxy->GetStatId());

			FString LightNameWithLevel;
			GetLightNameForDrawEvent(LightSceneInfo.Proxy, LightNameWithLevel);
			SCOPED_DRAW_EVENTF(EventLightPass, DEC_SCENE_ITEMS, *LightNameWithLevel);

			// Do not resolve to scene color texture, this is done lazily
			GSceneRenderTargets.FinishRenderingSceneColor(false);

			if (bDrawShadows)
			{
				INC_DWORD_STAT(STAT_NumShadowedLights);

				// All shadows render with min blending
				GSceneRenderTargets.BeginRenderingLightAttenuation();
				RHIClear(true, FLinearColor::White, false, 0, false, 0, FIntRect());

				bool bRenderedTranslucentObjectShadows = RenderTranslucentProjectedShadows( &LightSceneInfo );
				// Render non-modulated projected shadows to the attenuation buffer.
				RenderProjectedShadows( &LightSceneInfo, bRenderedTranslucentObjectShadows, bInjectedTranslucentVolume );
			}

			// Render any reflective shadow maps (if necessary)
			if(ViewFamily.EngineShowFlags.GlobalIllumination && LightSceneInfo.Proxy->NeedsLPVInjection())
			{
				if ( LightSceneInfo.Proxy->HasReflectiveShadowMap() )
				{
					INC_DWORD_STAT(STAT_NumReflectiveShadowMapLights);
					RenderReflectiveShadowMaps( &LightSceneInfo );
				}
			}
			
			// Render light function to the attenuation buffer.
			if (bDirectLighting)
			{
				const bool bLightFunctionRendered = RenderLightFunction(&LightSceneInfo, bDrawShadows);

				if (ViewFamily.EngineShowFlags.PreviewShadowsIndicator
					&& !LightSceneInfo.bPrecomputedLightingIsValid 
					&& LightSceneInfo.Proxy->HasStaticShadowing())
				{
					const bool bLightAttenuationCleared = bDrawShadows || bLightFunctionRendered;
					RenderPreviewShadowsIndicator(&LightSceneInfo, bLightAttenuationCleared);
				}

				if (!bDrawShadows)
				{
					INC_DWORD_STAT(STAT_NumLightFunctionOnlyLights);
				}
			}
				
			// Resolve light attenuation buffer
			GSceneRenderTargets.FinishRenderingLightAttenuation();
			
			if(bDirectLighting && !bInjectedTranslucentVolume)
			{
				SCOPED_DRAW_EVENT(InjectTranslucentVolume, DEC_SCENE_ITEMS);
				// Accumulate this light's unshadowed contribution to the translucency lighting volume
				InjectTranslucentVolumeLighting(LightSceneInfo, NULL);
			}

			GSceneRenderTargets.SetLightAttenuationMode(true);
			GSceneRenderTargets.BeginRenderingSceneColor();

			// Render the light to the scene color buffer, conditionally using the attenuation buffer or a 1x1 white texture as input 
			if(bDirectLighting)
			{
				RenderLight( &LightSceneInfo, false, true );
			}
		}

		// Do not resolve to scene color texture, this is done lazily
		GSceneRenderTargets.FinishRenderingSceneColor(false);

		// Restore the default mode
		GSceneRenderTargets.SetLightAttenuationMode(true);

		// LPV Direct Light Injection
		for (int32 LightIndex = 0; LightIndex < SortedLights.Num(); LightIndex++)
		{
			const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
			const FLightSceneInfoCompact& LightSceneInfoCompact = SortedLightInfo.SceneInfo;
			const FLightSceneInfo* const LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

			// Render any reflective shadow maps (if necessary)
			if ( LightSceneInfo && LightSceneInfo->Proxy && LightSceneInfo->Proxy->NeedsLPVInjection() )
			{
				if ( !LightSceneInfo->Proxy->HasReflectiveShadowMap() )
				{
					// Inject the light directly into all relevant LPVs
					for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
					{
						if (LightSceneInfo->ShouldRenderLight(Views[ViewIndex]))
						{
							FSceneViewState* ViewState = (FSceneViewState*)Views[ViewIndex].State;

							FLightPropagationVolume* Lpv = ViewState->GetLightPropagationVolume();
							if ( Lpv && LightSceneInfo->Proxy )
							{
								Lpv->InjectLightDirect( *LightSceneInfo->Proxy );
							}
						}
					}					
				}
			}
		}
	}
}

void FDeferredShadingSceneRenderer::RenderLightArrayForOverlapViewmode(const TSparseArray<FLightSceneInfoCompact>& LightArray)
{
	for (TSparseArray<FLightSceneInfoCompact>::TConstIterator LightIt(LightArray); LightIt; ++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		const FLightSceneInfo* LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		// Nothing to do for black lights.
		if(LightSceneInfoCompact.Color.IsAlmostBlack())
		{
			continue;
		}

		bool bShouldRender = false;

		// Check if the light is visible in any of the views.
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			bShouldRender |= LightSceneInfo->ShouldRenderLight(Views[ViewIndex]);
		}

		if (bShouldRender 
			// Only render shadow casting stationary lights
			&& LightSceneInfo->Proxy->HasStaticShadowing() 
			&& !LightSceneInfo->Proxy->HasStaticLighting()
			&& LightSceneInfo->Proxy->CastsStaticShadow())
		{
			RenderLight(LightSceneInfo, true, false);
		}
	}
}

void FDeferredShadingSceneRenderer::RenderStationaryLightOverlap()
{
	if (Scene->bIsEditorScene)
	{
		GSceneRenderTargets.BeginRenderingSceneColor();

		// Clear to discard base pass values in scene color since we didn't skip that, to have valid scene depths
		RHIClear(true, FLinearColor::Black, false, 0, false, 0, FIntRect());

		RenderLightArrayForOverlapViewmode(Scene->Lights);

		//Note: making use of FScene::InvisibleLights, which contains lights that haven't been added to the scene in the same way as visible lights
		// So code called by RenderLightArrayForOverlapViewmode must be careful what it accesses
		RenderLightArrayForOverlapViewmode(Scene->InvisibleLights);
	}
}

/** Sets up rasterizer and depth state for rendering bounding geometry in a deferred pass. */
void SetBoundingGeometryRasterizerAndDepthState(const FViewInfo& View, const FSphere& LightBounds)
{
	const bool bCameraInsideLightGeometry = ((FVector)View.ViewMatrices.ViewOrigin - LightBounds.Center).SizeSquared() < FMath::Square(LightBounds.W * 1.05f + View.NearClippingDistance * 2.0f);
	if (bCameraInsideLightGeometry)
	{
		// Render backfaces with depth tests disabled since the camera is inside (or close to inside) the light geometry
		RHISetRasterizerState(View.bReverseCulling ? TStaticRasterizerState<FM_Solid,CM_CW>::GetRHI() : TStaticRasterizerState<FM_Solid,CM_CCW>::GetRHI());
	}
	else
	{
		// Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside the light geometry
		RHISetRasterizerState(View.bReverseCulling ? TStaticRasterizerState<FM_Solid,CM_CCW>::GetRHI() : TStaticRasterizerState<FM_Solid,CM_CW>::GetRHI());
	}

	// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
	RHISetDepthStencilState(
		bCameraInsideLightGeometry
		? TStaticDepthStencilState<false,CF_Always>::GetRHI()
		: TStaticDepthStencilState<false,CF_GreaterEqual>::GetRHI()
		);
}

template<bool bUseIESProfile, bool bRadialAttenuation, bool bInverseSquaredFalloff>
static void SetShaderTemplLighting(
	const FSceneView& View, 
	FShader* VertexShader,
	const FLightSceneInfo* LightSceneInfo)
{
	if(View.Family->EngineShowFlags.VisualizeLightCulling)
	{
		TShaderMapRef<TDeferredLightPS<false, bRadialAttenuation, false, true> > PixelShader(GetGlobalShaderMap());
		SetGlobalBoundShaderState(PixelShader->GetBoundShaderState(), GFilterVertexDeclaration.VertexDeclarationRHI, VertexShader, *PixelShader);
		PixelShader->SetParameters(View, LightSceneInfo);
	}
	else
	{
		TShaderMapRef<TDeferredLightPS<bUseIESProfile, bRadialAttenuation, bInverseSquaredFalloff, false> > PixelShader(GetGlobalShaderMap());
		SetGlobalBoundShaderState(PixelShader->GetBoundShaderState(), GFilterVertexDeclaration.VertexDeclarationRHI, VertexShader, *PixelShader);
		PixelShader->SetParameters(View, LightSceneInfo);
	}
}

template<bool bUseIESProfile, bool bRadialAttenuation, bool bInverseSquaredFalloff>
static void SetShaderTemplLightingSimple(
	const FSceneView& View, 
	FShader* VertexShader,
	const FSimpleLightEntry& SimpleLight)
{
	if(View.Family->EngineShowFlags.VisualizeLightCulling)
	{
		TShaderMapRef<TDeferredLightPS<false, bRadialAttenuation, false, true> > PixelShader(GetGlobalShaderMap());
		SetGlobalBoundShaderState(PixelShader->GetBoundShaderState(), GFilterVertexDeclaration.VertexDeclarationRHI, VertexShader, *PixelShader);
		PixelShader->SetParametersSimpleLight(View, SimpleLight);
	}
	else
	{
		TShaderMapRef<TDeferredLightPS<bUseIESProfile, bRadialAttenuation, bInverseSquaredFalloff, false> > PixelShader(GetGlobalShaderMap());
		SetGlobalBoundShaderState(PixelShader->GetBoundShaderState(), GFilterVertexDeclaration.VertexDeclarationRHI, VertexShader, *PixelShader);
		PixelShader->SetParametersSimpleLight(View, SimpleLight);
	}}

/**
 * Used by RenderLights to render a light to the scene color buffer.
 *
 * @param LightSceneInfo Represents the current light
 * @param LightIndex The light's index into FScene::Lights
 * @return true if anything got rendered
 */
void FDeferredShadingSceneRenderer::RenderLight(const FLightSceneInfo* LightSceneInfo, bool bRenderOverlap, bool bIssueDrawEvent)
{
	SCOPE_CYCLE_COUNTER(STAT_DirectLightRenderingTime);
	INC_DWORD_STAT(STAT_NumLightsUsingStandardDeferred);
	SCOPED_CONDITIONAL_DRAW_EVENT(StandardDeferredLighting, bIssueDrawEvent, DEC_SCENE_ITEMS);

	// Use additive blending for color
	RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());

	bool bStencilDirty = false;
	const FSphere LightBounds = LightSceneInfo->Proxy->GetBoundingSphere();

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];

		bool bUseIESTexture = false;

		if(View.Family->EngineShowFlags.TexturedLightProfiles)
		{
			bUseIESTexture = (LightSceneInfo->Proxy->GetIESTextureResource() != 0);
		}

		// Set the device viewport for the view.
		RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

		if (LightSceneInfo->Proxy->GetLightType() == LightType_Directional)
		{
			TShaderMapRef<TDeferredLightVS<false> > VertexShader(GetGlobalShaderMap());

			RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

			if (bRenderOverlap)
			{
				TShaderMapRef<TDeferredLightOverlapPS<false> > PixelShader(GetGlobalShaderMap());
				SetGlobalBoundShaderState(PixelShader->GetBoundShaderState(), GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
				PixelShader->SetParameters(View, LightSceneInfo);
			}
			else
			{
				if(bUseIESTexture)
				{
					SetShaderTemplLighting<true, false, false>(View, *VertexShader, LightSceneInfo);
				}
				else
				{
					SetShaderTemplLighting<false, false, false>(View, *VertexShader, LightSceneInfo);
				}
			}

			VertexShader->SetParameters(View, LightSceneInfo);

			// Apply the directional light as a full screen quad
			DrawRectangle( 
				0, 0,
				View.ViewRect.Width(), View.ViewRect.Height(),
				View.ViewRect.Min.X, View.ViewRect.Min.Y, 
				View.ViewRect.Width(), View.ViewRect.Height(),
				View.ViewRect.Size(),
				GSceneRenderTargets.GetBufferSizeXY(),
				EDRF_UseTriangleOptimization);
		}
		else
		{
			TShaderMapRef<TDeferredLightVS<true> > VertexShader(GetGlobalShaderMap());

			SetBoundingGeometryRasterizerAndDepthState(View, LightBounds);

			if (bRenderOverlap)
			{
				TShaderMapRef<TDeferredLightOverlapPS<true> > PixelShader(GetGlobalShaderMap());
				SetGlobalBoundShaderState(PixelShader->GetBoundShaderState(), GetVertexDeclarationFVector4(), *VertexShader, *PixelShader);
				PixelShader->SetParameters(View, LightSceneInfo);
			}
			else
			{
				if( LightSceneInfo->Proxy->IsInverseSquared() )
				{
					if(bUseIESTexture)
					{
						SetShaderTemplLighting<true, true, true>(View, *VertexShader, LightSceneInfo);
					}
					else
					{
						SetShaderTemplLighting<false, true, true>(View, *VertexShader, LightSceneInfo);
					}
				}
				else
				{
					if(bUseIESTexture)
					{
						SetShaderTemplLighting<true, true, false>(View, *VertexShader, LightSceneInfo);
					}
					else
					{
						SetShaderTemplLighting<false, true, false>(View, *VertexShader, LightSceneInfo);
					}
				}
			}

			VertexShader->SetParameters(View, LightSceneInfo);

			if (LightSceneInfo->Proxy->GetLightType() == LightType_Point)
			{
				// Apply the point or spot light with some approximately bounding geometry, 
				// So we can get speedups from depth testing and not processing pixels outside of the light's influence.
				StencilingGeometry::DrawSphere();
			}
			else if (LightSceneInfo->Proxy->GetLightType() == LightType_Spot)
			{
				StencilingGeometry::DrawCone();
			}
		}
	}

	if (bStencilDirty)
	{
		// Clear the stencil buffer to 0.
		RHIClear(false,FColor(0,0,0),false,0,true,0, FIntRect());
	}
}

void FDeferredShadingSceneRenderer::RenderSimpleLightsStandardDeferred(const TArray<FSimpleLightEntry, SceneRenderingAllocator>& SimpleLights)
{
	SCOPE_CYCLE_COUNTER(STAT_DirectLightRenderingTime);
	INC_DWORD_STAT_BY(STAT_NumLightsUsingStandardDeferred, SimpleLights.Num());
	SCOPED_DRAW_EVENT(StandardDeferredSimpleLights, DEC_SCENE_ITEMS);
	
	// Use additive blending for color
	RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());


	for (int32 LightIndex = 0; LightIndex < SimpleLights.Num(); LightIndex++)
	{
		const FSimpleLightEntry& SimpleLight = SimpleLights[LightIndex];
		const FSphere LightBounds(SimpleLight.PositionAndRadius, SimpleLight.PositionAndRadius.W);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			FViewInfo& View = Views[ViewIndex];
			// Set the device viewport for the view.
			RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

			TShaderMapRef<TDeferredLightVS<true> > VertexShader(GetGlobalShaderMap());

			SetBoundingGeometryRasterizerAndDepthState(View, LightBounds);

			if (SimpleLight.Exponent == 0)
			{
				// inverse squared
				SetShaderTemplLightingSimple<false, true, true>(View, *VertexShader, SimpleLight);
			}
			else
			{
				// light's exponent, not inverse squared
				SetShaderTemplLightingSimple<false, true, false>(View, *VertexShader, SimpleLight);
			}

			VertexShader->SetSimpleLightParameters(View, LightBounds);

			// Apply the point or spot light with some approximately bounding geometry, 
			// So we can get speedups from depth testing and not processing pixels outside of the light's influence.
			StencilingGeometry::DrawSphere();
		}
	}
}

