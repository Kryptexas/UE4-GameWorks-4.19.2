// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LightMapRendering.h: Light map rendering definitions.
=============================================================================*/

#ifndef __LIGHTMAPRENDERING_H__
#define __LIGHTMAPRENDERING_H__

extern ENGINE_API bool GShowDebugSelectedLightmap;
extern ENGINE_API FLightMap2D* GDebugSelectedLightmap;
extern bool GVisualizeMipLevels;

/**
 */
class FNullLightMapShaderComponent
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{}
	void Serialize(FArchive& Ar)
	{}
};

/**
 * A policy for shaders without a light-map.
 */
class FNoLightMapPolicy
{
public:

	typedef FNullLightMapShaderComponent VertexParametersType;
	typedef FNullLightMapShaderComponent PixelParametersType;
	typedef FMeshDrawingPolicy::ElementDataType ElementDataType;

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{}

	void Set(
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView* View
		) const
	{
		check(VertexFactory);
		VertexFactory->Set();
	}

	void SetMesh(
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
	{}

	friend bool operator==(const FNoLightMapPolicy A,const FNoLightMapPolicy B)
	{
		return true;
	}

	friend int32 CompareDrawingPolicy(const FNoLightMapPolicy&,const FNoLightMapPolicy&)
	{
		return 0;
	}
};

enum ELightmapQuality
{
	LQ_LIGHTMAP,
	HQ_LIGHTMAP,
};

/**
 * Base policy for shaders with lightmaps.
 */
template< ELightmapQuality LightmapQuality >
class TLightMapPolicy
{
public:

	typedef FLightMapInteraction ElementDataType;

	class VertexParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			LightMapCoordinateScaleBiasParameter.Bind(ParameterMap,TEXT("LightMapCoordinateScaleBias"));
		}

		void SetLightMapScale(FShader* VertexShader,const FLightMapInteraction& LightMapInteraction) const
		{
			const FVector2D LightmapCoordinateScale = LightMapInteraction.GetCoordinateScale();
			const FVector2D LightmapCoordinateBias = LightMapInteraction.GetCoordinateBias();
			SetShaderValue(VertexShader->GetVertexShader(),LightMapCoordinateScaleBiasParameter,FVector4(
				LightmapCoordinateScale.X,
				LightmapCoordinateScale.Y,
				LightmapCoordinateBias.X,
				LightmapCoordinateBias.Y
				));
		}

		void Serialize(FArchive& Ar)
		{
			Ar << LightMapCoordinateScaleBiasParameter;
		}

	private:
		FShaderParameter LightMapCoordinateScaleBiasParameter;
	};

	class PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			LightMapTextureParameter.Bind(ParameterMap,TEXT("LightMapTexture"));
			LightMapSamplerParameter.Bind(ParameterMap,TEXT("LightMapSampler"));
			SkyOcclusionTexture.Bind(ParameterMap,TEXT("SkyOcclusionTexture"));
			SkyOcclusionSampler.Bind(ParameterMap,TEXT("SkyOcclusionSampler"));
			LightMapScaleParameter.Bind(ParameterMap,TEXT("LightMapScale"));
			LightMapAddParameter.Bind(ParameterMap,TEXT("LightMapAdd"));
		}

		void SetLightMapTexture(FShader* PixelShader, const UTexture2D* LightMapTexture) const
		{
			FTexture* TextureResource = GBlackTexture;

			if (LightMapTexture)
			{
				TextureResource = LightMapTexture->Resource;
			}

			SetTextureParameter(
				PixelShader->GetPixelShader(),
				LightMapTextureParameter,
				LightMapSamplerParameter,
				TextureResource
				);
		}

		void SetSkyOcclusionTexture(FShader* PixelShader, const UTexture2D* SkyOcclusionTextureValue) const
		{
			FTexture* TextureResource = GWhiteTexture;

			if (SkyOcclusionTextureValue)
			{
				TextureResource = SkyOcclusionTextureValue->Resource;
			}

			SetTextureParameter(
				PixelShader->GetPixelShader(),
				SkyOcclusionTexture,
				SkyOcclusionSampler,
				TextureResource
				);
		}

		void SetLightMapScale(FShader* PixelShader,const FLightMapInteraction& LightMapInteraction) const
		{
			SetShaderValueArray(PixelShader->GetPixelShader(),LightMapScaleParameter,LightMapInteraction.GetScaleArray(),LightMapInteraction.GetNumLightmapCoefficients());
			SetShaderValueArray(PixelShader->GetPixelShader(),LightMapAddParameter,LightMapInteraction.GetAddArray(),LightMapInteraction.GetNumLightmapCoefficients());
		}

		void Serialize(FArchive& Ar)
		{
			Ar << LightMapTextureParameter;
			Ar << LightMapSamplerParameter;
			Ar << SkyOcclusionTexture;
			Ar << SkyOcclusionSampler;
			Ar << LightMapScaleParameter;
			Ar << LightMapAddParameter;
		}

	private:
		FShaderResourceParameter LightMapTextureParameter;
		FShaderResourceParameter LightMapSamplerParameter;
		FShaderResourceParameter SkyOcclusionTexture;
		FShaderResourceParameter SkyOcclusionSampler;
		FShaderParameter LightMapScaleParameter;
		FShaderParameter LightMapAddParameter;
	};

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		switch( LightmapQuality )
		{
			case LQ_LIGHTMAP:
				OutEnvironment.SetDefine(TEXT("LQ_TEXTURE_LIGHTMAP"),TEXT("1"));
				OutEnvironment.SetDefine(TEXT("NUM_LIGHTMAP_COEFFICIENTS"), NUM_LQ_LIGHTMAP_COEF);
				break;
			case HQ_LIGHTMAP:
				OutEnvironment.SetDefine(TEXT("HQ_TEXTURE_LIGHTMAP"),TEXT("1"));
				OutEnvironment.SetDefine(TEXT("NUM_LIGHTMAP_COEFFICIENTS"), NUM_HQ_LIGHTMAP_COEF);
				break;
			default:
				check(0);
		}
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));

		// GetValueOnAnyThread() as it's possible that ShouldCache is called from rendering thread. That is to output some error message.
		return Material->GetLightingModel() != MLM_Unlit 
			&& VertexFactoryType->SupportsStaticLighting() 
			&& (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnAnyThread() != 0)
			&& (Material->IsUsedWithStaticLighting() || Material->IsSpecialEngineMaterial());

		// if LQ
		//return (IsPCPlatform(Platform) || IsES2Platform(Platform));
	}

	void Set(
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView* View
		) const
	{
		check(VertexFactory);
		VertexFactory->Set();
	}

	void SetMesh(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FLightMapInteraction& LightMapInteraction
		) const
	{
		if(VertexShaderParameters)
		{
			VertexShaderParameters->SetLightMapScale(VertexShader,LightMapInteraction);
		}

		if(PixelShaderParameters)
		{
			PixelShaderParameters->SetLightMapScale(PixelShader,LightMapInteraction);
			PixelShaderParameters->SetLightMapTexture(PixelShader, LightMapInteraction.GetTexture());
			PixelShaderParameters->SetSkyOcclusionTexture(PixelShader, LightMapInteraction.GetSkyOcclusionTexture());
		}
	}

	friend bool operator==( const TLightMapPolicy< LightmapQuality > A, const TLightMapPolicy< LightmapQuality > B )
	{
		return true;
	}

	friend int32 CompareDrawingPolicy( const TLightMapPolicy< LightmapQuality >& A, const TLightMapPolicy< LightmapQuality >& B )
	{
		return 0;
	}
};

// A light map policy for computing up to 4 signed distance field shadow factors in the base pass.
template< ELightmapQuality LightmapQuality >
class TDistanceFieldShadowsAndLightMapPolicy : public TLightMapPolicy< LightmapQuality >
{
	typedef TLightMapPolicy< LightmapQuality >	Super;
	typedef typename Super::ElementDataType		SuperElementDataType;
public:

	struct ElementDataType
	{
		SuperElementDataType SuperElementData;
		
		FShadowMapInteraction ShadowMapInteraction;
		FVector4 DistanceFieldValues;

		/** Initialization constructor. */
		ElementDataType( const FShadowMapInteraction& InShadowMapInteraction, const SuperElementDataType& InSuperElementData )
			: SuperElementData(InSuperElementData)
		{
			ShadowMapInteraction = InShadowMapInteraction;

			static const auto CVar0 = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.Shadow.DistanceFieldPenumbraSize"));
			const float PenumbraSize = CVar0->GetValueOnAnyThread();

			// Bias to convert distance from the distance field into the shadow penumbra based on penumbra size
			DistanceFieldValues.X = 1.0f / FMath::Min(PenumbraSize, 1.0f);
			DistanceFieldValues.Y = -0.5f * DistanceFieldValues.X + 0.5f;
			DistanceFieldValues.Z = 0.0f;
			DistanceFieldValues.W = 0.0f;
		}
	};

	class VertexParametersType : public Super::VertexParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			ShadowMapCoordinateScaleBias.Bind(ParameterMap, TEXT("ShadowMapCoordinateScaleBias"));
			Super::VertexParametersType::Bind(ParameterMap);
		}

		void SetMesh(FShader* VertexShader, const FShadowMapInteraction& ShadowMapInteraction) const
		{
			SetShaderValue(VertexShader->GetVertexShader(), ShadowMapCoordinateScaleBias, FVector4(ShadowMapInteraction.GetCoordinateScale(), ShadowMapInteraction.GetCoordinateBias()));
		}

		void Serialize(FArchive& Ar)
		{
			Ar << ShadowMapCoordinateScaleBias;
			Super::VertexParametersType::Serialize(Ar);
		}

	private:
		FShaderParameter ShadowMapCoordinateScaleBias;
	};


	class PixelParametersType : public Super::PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			DistanceFieldParameters.Bind(ParameterMap,TEXT("DistanceFieldParameters"));
			StaticShadowMapMasks.Bind(ParameterMap,TEXT("StaticShadowMapMasks"));
			StaticShadowTexture.Bind(ParameterMap,TEXT("StaticShadowTexture"));
			StaticShadowSampler.Bind(ParameterMap, TEXT("StaticShadowTextureSampler"));

			Super::PixelParametersType::Bind(ParameterMap);
		}

		void SetMesh(FShader* PixelShader, const FShadowMapInteraction& ShadowMapInteraction, const FVector4& DistanceFieldValues) const
		{
			SetShaderValue(PixelShader->GetPixelShader(), DistanceFieldParameters, DistanceFieldValues);
			
			SetShaderValue(PixelShader->GetPixelShader(), StaticShadowMapMasks, FVector4(
				ShadowMapInteraction.GetChannelValid(0),
				ShadowMapInteraction.GetChannelValid(1),
				ShadowMapInteraction.GetChannelValid(2),
				ShadowMapInteraction.GetChannelValid(3)
				));

			SetTextureParameter(
				PixelShader->GetPixelShader(),
				StaticShadowTexture,
				StaticShadowSampler,
				ShadowMapInteraction.GetTexture() ? ShadowMapInteraction.GetTexture()->Resource : GWhiteTexture
				);
		}

		void Serialize(FArchive& Ar)
		{
			Ar << DistanceFieldParameters;
			Ar << StaticShadowMapMasks;
			Ar << StaticShadowTexture;
			Ar << StaticShadowSampler;
			Super::PixelParametersType::Serialize(Ar);
		}

	private:
		FShaderParameter StaticShadowMapMasks;
		FShaderResourceParameter StaticShadowTexture;
		FShaderResourceParameter StaticShadowSampler;
		FShaderParameter DistanceFieldParameters;
	};

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("STATICLIGHTING_TEXTUREMASK"), 1);
		OutEnvironment.SetDefine(TEXT("STATICLIGHTING_SIGNEDDISTANCEFIELD"), 1);
		Super::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	TDistanceFieldShadowsAndLightMapPolicy() {}

	void SetMesh(
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
		if (VertexShaderParameters)
		{
			VertexShaderParameters->SetMesh(VertexShader, ElementData.ShadowMapInteraction);
		}

		if (PixelShaderParameters)
		{
			PixelShaderParameters->SetMesh(PixelShader, ElementData.ShadowMapInteraction, ElementData.DistanceFieldValues);
		}

		Super::SetMesh(View, PrimitiveSceneProxy, VertexShaderParameters, PixelShaderParameters, VertexShader, PixelShader, VertexFactory, MaterialRenderProxy, ElementData.SuperElementData);
	}
};

/**
 * Policy for 'fake' texture lightmaps, such as the LightMap density rendering mode
 */
class FDummyLightMapPolicy : public TLightMapPolicy< HQ_LIGHTMAP >
{
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return Material->GetLightingModel() != MLM_Unlit && VertexFactoryType->SupportsStaticLighting();
	}

	void Set(
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView* View
		) const
	{
		check(VertexFactory);
		VertexFactory->Set();
	}

	void SetMesh(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FLightMapInteraction& LightMapInteraction
		) const
	{
		if(VertexShaderParameters)
		{
			VertexShaderParameters->SetLightMapScale(VertexShader,LightMapInteraction);
		}

		if(PixelShaderParameters)
		{
			PixelShaderParameters->SetLightMapScale(PixelShader,LightMapInteraction);
		}
	}

	/** Initialization constructor. */
	FDummyLightMapPolicy()
	{
	}

	friend bool operator==(const FDummyLightMapPolicy A,const FDummyLightMapPolicy B)
	{
		return true;
	}

	friend int32 CompareDrawingPolicy(const FDummyLightMapPolicy& A,const FDummyLightMapPolicy& B)
	{
		return 0;
	}
};

/**
 * Policy for self shadowing translucency from a directional light
 */
class FSelfShadowedTranslucencyPolicy : public FNoLightMapPolicy
{
public:

	struct ElementDataType
	{
		ElementDataType(const FProjectedShadowInfo* InTranslucentSelfShadow) :
			TranslucentSelfShadow(InTranslucentSelfShadow)
		{}

		const FProjectedShadowInfo* TranslucentSelfShadow;
	};

	class PixelParametersType : public FNoLightMapPolicy::PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			TranslucencyShadowParameters.Bind(ParameterMap);
			WorldToShadowMatrix.Bind(ParameterMap, TEXT("WorldToShadowMatrix"));
			ShadowUVMinMax.Bind(ParameterMap, TEXT("ShadowUVMinMax"));
			DirectionalLightDirection.Bind(ParameterMap, TEXT("DirectionalLightDirection"));
			DirectionalLightColor.Bind(ParameterMap, TEXT("DirectionalLightColor"));
		}

		void Serialize(FArchive& Ar)
		{
			Ar << TranslucencyShadowParameters;
			Ar << WorldToShadowMatrix;
			Ar << ShadowUVMinMax;
			Ar << DirectionalLightDirection;
			Ar << DirectionalLightColor;
		}

		FTranslucencyShadowProjectionShaderParameters TranslucencyShadowParameters;
		FShaderParameter WorldToShadowMatrix;
		FShaderParameter ShadowUVMinMax;
		FShaderParameter DirectionalLightDirection;
		FShaderParameter DirectionalLightColor;
	};

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return Material->GetLightingModel() != MLM_Unlit && IsTranslucentBlendMode(Material->GetBlendMode()) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("TRANSLUCENT_SELF_SHADOWING"),TEXT("1"));
		FNoLightMapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	FSelfShadowedTranslucencyPolicy() {}

	void SetMesh(
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
			// Set these even if ElementData.TranslucentSelfShadow is NULL to avoid a d3d debug error from the shader expecting texture SRV's when a different type are bound
			PixelShaderParameters->TranslucencyShadowParameters.Set(PixelShader);

			if (ElementData.TranslucentSelfShadow)
			{
				FVector4 ShadowmapMinMax;
				FMatrix WorldToShadowMatrixValue = ElementData.TranslucentSelfShadow->GetWorldToShadowMatrix(ShadowmapMinMax);

				SetShaderValue(PixelShader->GetPixelShader(), PixelShaderParameters->WorldToShadowMatrix, WorldToShadowMatrixValue);
				SetShaderValue(PixelShader->GetPixelShader(), PixelShaderParameters->ShadowUVMinMax, ShadowmapMinMax);
				SetShaderValue(PixelShader->GetPixelShader(), PixelShaderParameters->DirectionalLightDirection, ElementData.TranslucentSelfShadow->LightSceneInfo->Proxy->GetDirection());
				//@todo - support fading from both views
				const float FadeAlpha = ElementData.TranslucentSelfShadow->FadeAlphas[0];
				// Incorporate the diffuse scale of 1 / PI into the light color
				const FVector4 DirectionalLightColorValue(FVector(ElementData.TranslucentSelfShadow->LightSceneInfo->Proxy->GetColor() * FadeAlpha / PI), FadeAlpha);
				SetShaderValue(PixelShader->GetPixelShader(), PixelShaderParameters->DirectionalLightColor, DirectionalLightColorValue);
			}
			else
			{
				SetShaderValue(PixelShader->GetPixelShader(), PixelShaderParameters->DirectionalLightColor, FVector4(0, 0, 0, 0));
			}
		}
	}
};

/**
 * Allows a dynamic object to access indirect lighting through a per-object allocation in a volume texture atlas
 */
class FCachedVolumeIndirectLightingPolicy : public FNoLightMapPolicy
{
public:

	struct ElementDataType {};

	class PixelParametersType : public FNoLightMapPolicy::PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			IndirectLightingCacheTexture0.Bind(ParameterMap, TEXT("IndirectLightingCacheTexture0"));
			IndirectLightingCacheTexture1.Bind(ParameterMap, TEXT("IndirectLightingCacheTexture1"));
			IndirectLightingCacheTexture2.Bind(ParameterMap, TEXT("IndirectLightingCacheTexture2"));
			IndirectLightingCacheSampler0.Bind(ParameterMap, TEXT("IndirectLightingCacheTextureSampler0"));
			IndirectLightingCacheSampler1.Bind(ParameterMap, TEXT("IndirectLightingCacheTextureSampler1"));
			IndirectLightingCacheSampler2.Bind(ParameterMap, TEXT("IndirectLightingCacheTextureSampler2"));
			IndirectlightingCachePrimitiveAdd.Bind(ParameterMap, TEXT("IndirectlightingCachePrimitiveAdd"));
			IndirectlightingCachePrimitiveScale.Bind(ParameterMap, TEXT("IndirectlightingCachePrimitiveScale"));
			IndirectlightingCacheMinUV.Bind(ParameterMap, TEXT("IndirectlightingCacheMinUV"));
			IndirectlightingCacheMaxUV.Bind(ParameterMap, TEXT("IndirectlightingCacheMaxUV"));
		}

		void Serialize(FArchive& Ar)
		{
			Ar << IndirectLightingCacheTexture0;
			Ar << IndirectLightingCacheTexture1;
			Ar << IndirectLightingCacheTexture2;
			Ar << IndirectLightingCacheSampler0;
			Ar << IndirectLightingCacheSampler1;
			Ar << IndirectLightingCacheSampler2;
			Ar << IndirectlightingCachePrimitiveAdd;
			Ar << IndirectlightingCachePrimitiveScale;
			Ar << IndirectlightingCacheMinUV;
			Ar << IndirectlightingCacheMaxUV;
		}

		FShaderResourceParameter IndirectLightingCacheTexture0;
		FShaderResourceParameter IndirectLightingCacheTexture1;
		FShaderResourceParameter IndirectLightingCacheTexture2;
		FShaderResourceParameter IndirectLightingCacheSampler0;
		FShaderResourceParameter IndirectLightingCacheSampler1;
		FShaderResourceParameter IndirectLightingCacheSampler2;
		FShaderParameter IndirectlightingCachePrimitiveAdd;
		FShaderParameter IndirectlightingCachePrimitiveScale;
		FShaderParameter IndirectlightingCacheMinUV;
		FShaderParameter IndirectlightingCacheMaxUV;
	};

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));

		return Material->GetLightingModel() != MLM_Unlit 
			&& !IsTranslucentBlendMode(Material->GetBlendMode()) 
			&& (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnAnyThread() != 0)
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("CACHED_VOLUME_INDIRECT_LIGHTING"),TEXT("1"));
		FNoLightMapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	FCachedVolumeIndirectLightingPolicy() {}

	void Set(
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView* View
		) const;

	void SetMesh(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const ElementDataType& ElementData
		) const;
};


/**
 * Allows a dynamic object to access indirect lighting through a per-object lighting sample
 */
class FCachedPointIndirectLightingPolicy : public FNoLightMapPolicy
{
public:

	struct ElementDataType 
	{
		ElementDataType(bool bInPackAmbientTermInFirstVector) :
			bPackAmbientTermInFirstVector(bInPackAmbientTermInFirstVector)
		{}

		bool bPackAmbientTermInFirstVector;
	};

	class PixelParametersType : public FNoLightMapPolicy::PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			IndirectLightingSHCoefficients.Bind(ParameterMap, TEXT("IndirectLightingSHCoefficients"));
			DirectionalLightShadowing.Bind(ParameterMap, TEXT("DirectionalLightShadowing"));
		}

		void Serialize(FArchive& Ar)
		{
			Ar << IndirectLightingSHCoefficients;
			Ar << DirectionalLightShadowing;
		}

		FShaderParameter IndirectLightingSHCoefficients;
		FShaderParameter DirectionalLightShadowing;
	};

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));

		return Material->GetLightingModel() != MLM_Unlit 
			&& (IsTranslucentBlendMode(Material->GetBlendMode()) || GetMaxSupportedFeatureLevel(Platform) == ERHIFeatureLevel::ES2) 
			&& (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnAnyThread() != 0);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("CACHED_POINT_INDIRECT_LIGHTING"),TEXT("1"));
		FNoLightMapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	FCachedPointIndirectLightingPolicy() {}

	void SetMesh(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const ElementDataType& ElementData
		) const;
};

/**
 * Self shadowing translucency from a directional light + allows a dynamic object to access indirect lighting through a per-object lighting sample
 */
class FSelfShadowedCachedPointIndirectLightingPolicy : public FSelfShadowedTranslucencyPolicy
{
public:

	class PixelParametersType : public FSelfShadowedTranslucencyPolicy::PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			IndirectLightingSHCoefficients.Bind(ParameterMap, TEXT("IndirectLightingSHCoefficients"));
			FSelfShadowedTranslucencyPolicy::PixelParametersType::Bind(ParameterMap);
		}

		void Serialize(FArchive& Ar)
		{
			Ar << IndirectLightingSHCoefficients;

			FSelfShadowedTranslucencyPolicy::PixelParametersType::Serialize(Ar);
		}

		FShaderParameter IndirectLightingSHCoefficients;
	};

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		static IConsoleVariable* AllowStaticLightingVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AllowStaticLighting"));

		return Material->GetLightingModel() != MLM_Unlit 
			&& IsTranslucentBlendMode(Material->GetBlendMode()) 
			&& (!AllowStaticLightingVar || AllowStaticLightingVar->GetInt() != 0)
			&& FSelfShadowedTranslucencyPolicy::ShouldCache(Platform, Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("CACHED_POINT_INDIRECT_LIGHTING"),TEXT("1"));
		FSelfShadowedTranslucencyPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	FSelfShadowedCachedPointIndirectLightingPolicy() {}

	void SetMesh(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const ElementDataType& ElementData
		) const;
};

/**
 * Renders an unshadowed directional light in the base pass, used to support low end hardware where deferred shading is too expensive.
 */
class FSimpleDynamicLightingPolicy : public FNoLightMapPolicy
{
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return Material->GetLightingModel() != MLM_Unlit;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("SIMPLE_DYNAMIC_LIGHTING"),TEXT("1"));
		FNoLightMapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	FSimpleDynamicLightingPolicy() {}
};

/** Combines an unshadowed directional light with indirect lighting from a single SH sample. */
class FSimpleDirectionalLightAndSHIndirectPolicy
{
public:

	struct ElementDataType : public FSimpleDynamicLightingPolicy::ElementDataType, FCachedPointIndirectLightingPolicy::ElementDataType
	{
		ElementDataType(bool bInPackAmbientTermInFirstVector)
			: FCachedPointIndirectLightingPolicy::ElementDataType(bInPackAmbientTermInFirstVector)
		{}
	};

	class VertexParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			SimpleDynamicLightingParameters.Bind(ParameterMap);
			CachedPointIndirectParameters.Bind(ParameterMap);
		}

		void Serialize(FArchive& Ar)
		{
			SimpleDynamicLightingParameters.Serialize(Ar);
			CachedPointIndirectParameters.Serialize(Ar);
		}

		FSimpleDynamicLightingPolicy::VertexParametersType SimpleDynamicLightingParameters;
		FCachedPointIndirectLightingPolicy::VertexParametersType CachedPointIndirectParameters;
	};

	class PixelParametersType
	{
	public:
		void Bind(const FShaderParameterMap& ParameterMap)
		{
			SimpleDynamicLightingParameters.Bind(ParameterMap);
			CachedPointIndirectParameters.Bind(ParameterMap);
		}

		void Serialize(FArchive& Ar)
		{
			SimpleDynamicLightingParameters.Serialize(Ar);
			CachedPointIndirectParameters.Serialize(Ar);
		}

		FSimpleDynamicLightingPolicy::PixelParametersType SimpleDynamicLightingParameters;
		FCachedPointIndirectLightingPolicy::PixelParametersType CachedPointIndirectParameters;
	};

	/** Initialization constructor. */
	FSimpleDirectionalLightAndSHIndirectPolicy() {}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FSimpleDynamicLightingPolicy::ShouldCache(Platform, Material, VertexFactoryType) && FCachedPointIndirectLightingPolicy::ShouldCache(Platform, Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FSimpleDynamicLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		FCachedPointIndirectLightingPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	void Set(
		const VertexParametersType* VertexShaderParameters,
		const PixelParametersType* PixelShaderParameters,
		FShader* VertexShader,
		FShader* PixelShader,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView* View
		) const
	{
		check(VertexFactory);
		VertexFactory->Set();
	}

	void SetMesh(
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
		if (VertexShaderParameters && PixelShaderParameters)
		{
			CachedPointIndirectLightingPolicy.SetMesh(
				View, 
				PrimitiveSceneProxy, 
				&VertexShaderParameters->CachedPointIndirectParameters, 
				&PixelShaderParameters->CachedPointIndirectParameters, 
				VertexShader, 
				PixelShader, 
				VertexFactory, 
				MaterialRenderProxy, 
				ElementData.bPackAmbientTermInFirstVector);
		}
	}

	friend bool operator==(const FSimpleDirectionalLightAndSHIndirectPolicy A,const FSimpleDirectionalLightAndSHIndirectPolicy B)
	{
		return true;
	}

	friend int32 CompareDrawingPolicy(const FSimpleDirectionalLightAndSHIndirectPolicy&,const FSimpleDirectionalLightAndSHIndirectPolicy&)
	{
		return 0;
	}

private:

	FSimpleDynamicLightingPolicy SimpleDynamicLightingPolicy;
	FCachedPointIndirectLightingPolicy CachedPointIndirectLightingPolicy;
};

#endif // __LIGHTMAPRENDERING_H__
