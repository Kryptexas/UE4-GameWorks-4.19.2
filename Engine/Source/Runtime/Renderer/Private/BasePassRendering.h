// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BasePassRendering.h: Base pass rendering definitions.
=============================================================================*/

#pragma once

#include "LightMapRendering.h"
#include "ShaderBaseClasses.h"

/** Whether to allow the indirect lighting cache to be applied to dynamic objects. */
extern int32 GIndirectLightingCache;

/**
 * The base shader type for vertex shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 * The base type is shared between the versions with and without atmospheric fog.
 */

template<typename LightMapPolicyType>
class TBasePassVertexShaderBaseType : public FMeshMaterialShader, public LightMapPolicyType::VertexParametersType
{
protected:
	TBasePassVertexShaderBaseType() {}
	TBasePassVertexShaderBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
	FMeshMaterialShader(Initializer)
	{
		LightMapPolicyType::VertexParametersType::Bind(Initializer.ParameterMap);
		HeightFogParameters.Bind(Initializer.ParameterMap);
		AtmosphericFogTextureParameters.Bind(Initializer.ParameterMap);
	}

public:
	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Opaque and modulated materials shouldn't apply fog volumes in their base pass.
		const EBlendMode BlendMode = Material->GetBlendMode();
		return LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		LightMapPolicyType::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		LightMapPolicyType::VertexParametersType::Serialize(Ar);
		Ar << HeightFogParameters;
		Ar << AtmosphericFogTextureParameters;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FVertexFactory* VertexFactory,
		const FMaterial& InMaterialResource,
		const FSceneView& View,
		bool bAllowGlobalFog,
		ESceneRenderTargetsMode::Type TextureMode
		)
	{
		FMeshMaterialShader::SetParameters(GetVertexShader(),MaterialRenderProxy,InMaterialResource,View,TextureMode);

		HeightFogParameters.Set(GetVertexShader(), &View, bAllowGlobalFog);
		if (bAllowGlobalFog)
		{
			AtmosphericFogTextureParameters.Set(GetVertexShader(), View);
		}
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(),VertexFactory,View,Proxy,BatchElement);
	}

private:
	
	/** The parameters needed to calculate the fog contribution from height fog layers. */
	FHeightFogShaderParameters HeightFogParameters;
	FAtmosphereShaderTextureParameters AtmosphericFogTextureParameters;
};

template<typename LightMapPolicyType, bool bEnableAtmosphericFog>
class TBasePassVS : public TBasePassVertexShaderBaseType<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TBasePassVS,MeshMaterial);

protected:

	TBasePassVS() {}
	TBasePassVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		TBasePassVertexShaderBaseType<LightMapPolicyType>(Initializer)
	{
	}

public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		bool bShouldCache = TBasePassVertexShaderBaseType<LightMapPolicyType>::ShouldCache(Platform, Material, VertexFactoryType);
		return bShouldCache 
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3)
			&& (!bEnableAtmosphericFog || IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4));
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		TBasePassVertexShaderBaseType<LightMapPolicyType>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("BASEPASS_ATMOSPHERIC_FOG"),(uint32)(bEnableAtmosphericFog ? 1 : 0));
	}
};

/**
 * The base shader type for hull shaders.
 */
template<typename LightMapPolicyType>
class TBasePassHS : public FBaseHS
{
	DECLARE_SHADER_TYPE(TBasePassHS,MeshMaterial);

protected:

	TBasePassHS() {}

	TBasePassHS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FBaseHS(Initializer)
	{}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Re-use vertex shader gating
		return FBaseHS::ShouldCache(Platform, Material, VertexFactoryType)
			&& TBasePassVS<LightMapPolicyType,false>::ShouldCache(Platform,Material,VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		// Re-use vertex shader compilation environment
		TBasePassVS<LightMapPolicyType,false>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}
};

/**
 * The base shader type for Domain shaders.
 */
template<typename LightMapPolicyType>
class TBasePassDS : public FBaseDS
{
	DECLARE_SHADER_TYPE(TBasePassDS,MeshMaterial);

protected:

	TBasePassDS() {}

	TBasePassDS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FBaseDS(Initializer)
	{
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Re-use vertex shader gating
		return FBaseDS::ShouldCache(Platform, Material, VertexFactoryType)
			&& TBasePassVS<LightMapPolicyType,false>::ShouldCache(Platform,Material,VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		// Re-use vertex shader compilation environment
		TBasePassVS<LightMapPolicyType,false>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

public:
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FBaseDS::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FVertexFactory* VertexFactory,
		const FSceneView& View
		)
	{
		FBaseDS::SetParameters(MaterialRenderProxy, View);
	}
};

/** Parameters needed for lighting translucency, shared by multiple shaders. */
class FTranslucentLightingParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		TranslucencyLightingVolumeAmbientInner.Bind(ParameterMap, TEXT("TranslucencyLightingVolumeAmbientInner"));
		TranslucencyLightingVolumeAmbientInnerSampler.Bind(ParameterMap, TEXT("TranslucencyLightingVolumeAmbientInnerSampler"));
		TranslucencyLightingVolumeAmbientOuter.Bind(ParameterMap, TEXT("TranslucencyLightingVolumeAmbientOuter"));
		TranslucencyLightingVolumeAmbientOuterSampler.Bind(ParameterMap, TEXT("TranslucencyLightingVolumeAmbientOuterSampler"));
		TranslucencyLightingVolumeDirectionalInner.Bind(ParameterMap, TEXT("TranslucencyLightingVolumeDirectionalInner"));
		TranslucencyLightingVolumeDirectionalInnerSampler.Bind(ParameterMap, TEXT("TranslucencyLightingVolumeDirectionalInnerSampler"));
		TranslucencyLightingVolumeDirectionalOuter.Bind(ParameterMap, TEXT("TranslucencyLightingVolumeDirectionalOuter"));
		TranslucencyLightingVolumeDirectionalOuterSampler.Bind(ParameterMap, TEXT("TranslucencyLightingVolumeDirectionalOuterSampler"));
		ReflectionCubemap.Bind(ParameterMap, TEXT("ReflectionCubemap"));
		ReflectionCubemapSampler.Bind(ParameterMap, TEXT("ReflectionCubemapSampler"));
		CubemapArrayIndex.Bind(ParameterMap, TEXT("CubemapArrayIndex"));
	}

	void Set(FShader* Shader);

	void SetMesh(FShader* Shader, const FPrimitiveSceneProxy* Proxy);

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FTranslucentLightingParameters& P)
	{
		Ar << P.TranslucencyLightingVolumeAmbientInner;
		Ar << P.TranslucencyLightingVolumeAmbientInnerSampler;
		Ar << P.TranslucencyLightingVolumeAmbientOuter;
		Ar << P.TranslucencyLightingVolumeAmbientOuterSampler;
		Ar << P.TranslucencyLightingVolumeDirectionalInner;
		Ar << P.TranslucencyLightingVolumeDirectionalInnerSampler;
		Ar << P.TranslucencyLightingVolumeDirectionalOuter;
		Ar << P.TranslucencyLightingVolumeDirectionalOuterSampler;
		Ar << P.ReflectionCubemap;
		Ar << P.ReflectionCubemapSampler;
		Ar << P.CubemapArrayIndex;
		return Ar;
	}

private:

	FShaderResourceParameter TranslucencyLightingVolumeAmbientInner;
	FShaderResourceParameter TranslucencyLightingVolumeAmbientInnerSampler;
	FShaderResourceParameter TranslucencyLightingVolumeAmbientOuter;
	FShaderResourceParameter TranslucencyLightingVolumeAmbientOuterSampler;
	FShaderResourceParameter TranslucencyLightingVolumeDirectionalInner;
	FShaderResourceParameter TranslucencyLightingVolumeDirectionalInnerSampler;
	FShaderResourceParameter TranslucencyLightingVolumeDirectionalOuter;
	FShaderResourceParameter TranslucencyLightingVolumeDirectionalOuterSampler;
	FShaderResourceParameter ReflectionCubemap;
	FShaderResourceParameter ReflectionCubemapSampler;
	FShaderParameter CubemapArrayIndex;
};

/**
 * The base type for pixel shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 * The base type is shared between the versions with and without sky light.
 */
template<typename LightMapPolicyType>
class TBasePassPixelShaderBaseType : public FMeshMaterialShader, public LightMapPolicyType::PixelParametersType
{
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		LightMapPolicyType::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	TBasePassPixelShaderBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		LightMapPolicyType::PixelParametersType::Bind(Initializer.ParameterMap);
		TranslucentLightingParameters.Bind(Initializer.ParameterMap);
		EditorCompositeDepthTestParameter.Bind(Initializer.ParameterMap,TEXT("bEnableEditorPrimitiveDepthTest"));
		MSAASampleCount.Bind(Initializer.ParameterMap,TEXT("MSAASampleCount"));
		FilteredSceneDepthTexture.Bind(Initializer.ParameterMap,TEXT("FilteredSceneDepthTexture"));
		FilteredSceneDepthTextureSampler.Bind(Initializer.ParameterMap,TEXT("FilteredSceneDepthTextureSampler"));
	}
	TBasePassPixelShaderBaseType() {}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy, 
		const FMaterial& MaterialResource, 
		const FSceneView* View, 
		EBlendMode BlendMode, 
		bool bEnableEditorPrimitveDepthTest,
		ESceneRenderTargetsMode::Type TextureMode)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FMeshMaterialShader::SetParameters(ShaderRHI, MaterialRenderProxy, MaterialResource, *View, TextureMode);

		if (GRHIFeatureLevel >= ERHIFeatureLevel::SM4)
		{
			if (IsTranslucentBlendMode(BlendMode))
			{
				TranslucentLightingParameters.Set(this);
			}

			if( MaterialResource.IsUsedWithEditorCompositing() )
			{
				// Compute parameters for converting from screen space to pixel 
				FIntRect DestRect = View->ViewRect;
				FIntPoint ViewportOffset = DestRect.Min;
				FIntPoint ViewportExtent = DestRect.Size();

				FVector4 ScreenPosToPixelValue(
					ViewportExtent.X * 0.5f,
					-ViewportExtent.Y * 0.5f, 
					ViewportExtent.X * 0.5f - 0.5f + ViewportOffset.X,
					ViewportExtent.Y * 0.5f - 0.5f + ViewportOffset.Y);

				if(FilteredSceneDepthTexture.IsBound())
				{
					const FTexture2DRHIRef* DepthTexture = GSceneRenderTargets.GetActualDepthTexture();
					check(DepthTexture != NULL);
					SetTextureParameter(
						ShaderRHI,
						FilteredSceneDepthTexture,
						FilteredSceneDepthTextureSampler,
						TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
						*DepthTexture
						);
				}

				SetShaderValue(ShaderRHI, EditorCompositeDepthTestParameter, bEnableEditorPrimitveDepthTest );
				SetShaderValue(ShaderRHI, MSAASampleCount, GSceneRenderTargets.EditorPrimitivesColor ? GSceneRenderTargets.EditorPrimitivesColor->GetDesc().NumSamples : 0 );
			}
		}
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement, EBlendMode BlendMode)
	{
		if (GRHIFeatureLevel >= ERHIFeatureLevel::SM4
			&& IsTranslucentBlendMode(BlendMode))
		{
			TranslucentLightingParameters.SetMesh(this, Proxy);
		}

		FMeshMaterialShader::SetMesh(GetPixelShader(),VertexFactory,View,Proxy,BatchElement);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		LightMapPolicyType::PixelParametersType::Serialize(Ar);
		Ar << TranslucentLightingParameters;
		Ar << EditorCompositeDepthTestParameter;
		Ar << MSAASampleCount;
		Ar << FilteredSceneDepthTexture;
		Ar << FilteredSceneDepthTextureSampler;
		return bShaderHasOutdatedParameters;
	}

private:
	FTranslucentLightingParameters TranslucentLightingParameters;
	/** Parameter for whether or not to depth test in the pixel shader for editor primitives */
	FShaderParameter EditorCompositeDepthTestParameter;
	FShaderParameter MSAASampleCount;
	/** Parameter for reading filtered depth values */
	FShaderResourceParameter FilteredSceneDepthTexture; 
	FShaderResourceParameter FilteredSceneDepthTextureSampler; 
};

/** The concrete base pass pixel shader type. */
template<typename LightMapPolicyType, bool bEnableSkyLight>
class TBasePassPS : public TBasePassPixelShaderBaseType<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TBasePassPS,MeshMaterial);
public:
	
	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile skylight version for lit materials
		const bool bCacheShaders = !bEnableSkyLight || (Material->GetLightingModel() != MLM_Unlit);

		return bCacheShaders
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3)
			&& TBasePassPixelShaderBaseType<LightMapPolicyType>::ShouldCache(Platform, Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("ENABLE_SKY_LIGHT"),(uint32)(bEnableSkyLight ? 1 : 0));
		TBasePassPixelShaderBaseType<LightMapPolicyType>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}
	
	/** Initialization constructor. */
	TBasePassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		TBasePassPixelShaderBaseType<LightMapPolicyType>(Initializer)
	{}

	/** Default constructor. */
	TBasePassPS() {}
};

/**
 * Draws the emissive color and the light-map of a mesh.
 */
template<typename LightMapPolicyType>
class TBasePassDrawingPolicy : public FMeshDrawingPolicy
{
public:

	/** The data the drawing policy uses for each mesh element. */
	class ElementDataType
	{
	public:

		/** The element's light-map data. */
		typename LightMapPolicyType::ElementDataType LightMapElementData;

		/** Default constructor. */
		ElementDataType()
		{}

		/** Initialization constructor. */
		ElementDataType(const typename LightMapPolicyType::ElementDataType& InLightMapElementData)
		:	LightMapElementData(InLightMapElementData)
		{}
	};

	/** Initialization constructor. */
	TBasePassDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		LightMapPolicyType InLightMapPolicy,
		EBlendMode InBlendMode,
		ESceneRenderTargetsMode::Type InSceneTextureMode,
		bool bInEnableSkyLight,
		bool bInEnableAtmosphericFog,
		bool bOverrideWithShaderComplexity = false,
		bool bInAllowGlobalFog = false,
		bool bInEnableEditorPrimitiveDepthTest = false
		):
		FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy,InMaterialResource,bOverrideWithShaderComplexity),
		LightMapPolicy(InLightMapPolicy),
		BlendMode(InBlendMode),
		SceneTextureMode(InSceneTextureMode),
		bAllowGlobalFog(bInAllowGlobalFog),
		bEnableSkyLight(bInEnableSkyLight),
		bEnableEditorPrimitiveDepthTest(bInEnableEditorPrimitiveDepthTest),
		bEnableAtmosphericFog(bInEnableAtmosphericFog)
	{
		HullShader = NULL;
		DomainShader = NULL;
	
		const EMaterialTessellationMode MaterialTessellationMode = InMaterialResource.GetTessellationMode();

		if (RHISupportsTessellation(GRHIShaderPlatform)
			&& InVertexFactory->GetType()->SupportsTessellationShaders() 
			&& MaterialTessellationMode != MTM_NoTessellation)
		{
			// Find the base pass tessellation shaders since the material is tessellated
			HullShader = InMaterialResource.GetShader<TBasePassHS<LightMapPolicyType> >(VertexFactory->GetType());
			DomainShader = InMaterialResource.GetShader<TBasePassDS<LightMapPolicyType> >(VertexFactory->GetType());
		}

		if (bEnableAtmosphericFog)
		{
			VertexShader = InMaterialResource.GetShader<TBasePassVS<LightMapPolicyType, true> >(InVertexFactory->GetType());
		}
		else
		{
			VertexShader = InMaterialResource.GetShader<TBasePassVS<LightMapPolicyType, false> >(InVertexFactory->GetType());
		}

		if (bEnableSkyLight)
		{
			PixelShader = InMaterialResource.GetShader<TBasePassPS<LightMapPolicyType, true> >(InVertexFactory->GetType());
		}
		else
		{
			PixelShader = InMaterialResource.GetShader<TBasePassPS<LightMapPolicyType, false> >(InVertexFactory->GetType());
		}

#if DO_GUARD_SLOW
		// Somewhat hacky
		if (SceneTextureMode == ESceneRenderTargetsMode::DontSet && !bEnableEditorPrimitiveDepthTest && InMaterialResource.IsUsedWithEditorCompositing())
		{
			SceneTextureMode = ESceneRenderTargetsMode::DontSetIgnoreBoundByEditorCompositing;
		}
#endif
	}

	// FMeshDrawingPolicy interface.

	bool Matches(const TBasePassDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) &&
			VertexShader == Other.VertexShader &&
			PixelShader == Other.PixelShader &&
			HullShader == Other.HullShader &&
			DomainShader == Other.DomainShader &&
			SceneTextureMode == Other.SceneTextureMode &&
			bAllowGlobalFog == Other.bAllowGlobalFog &&
			bEnableSkyLight == Other.bEnableSkyLight && 

			LightMapPolicy == Other.LightMapPolicy;
	}

	void DrawShared(const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const
	{
		// Set the actual shader & vertex declaration state
		RHISetBoundShaderState( BoundShaderState);

		// Set the light-map policy.
		LightMapPolicy.Set(VertexShader,bOverrideWithShaderComplexity ? NULL : PixelShader,VertexShader,PixelShader,VertexFactory,MaterialRenderProxy,View);

		VertexShader->SetParameters(MaterialRenderProxy, VertexFactory, *MaterialResource, *View, bAllowGlobalFog, SceneTextureMode);

		if(HullShader)
		{
			HullShader->SetParameters(MaterialRenderProxy, *View);
		}

		if (DomainShader)
		{
			DomainShader->SetParameters(MaterialRenderProxy, VertexFactory, *View);
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bOverrideWithShaderComplexity)
		{
			// If we are in the translucent pass then override the blend mode, otherwise maintain additive blending.
			if (IsTranslucentBlendMode(BlendMode))
			{
				// Add complexity to existing
				RHISetBlendState(TStaticBlendState<CW_RGBA,BO_Add,BF_One,BF_One,BO_Add,BF_Zero,BF_One>::GetRHI());
			}

			TShaderMapRef<FShaderComplexityAccumulatePS> ShaderComplexityPixelShader(GetGlobalShaderMap());
			const uint32 NumPixelShaderInstructions = PixelShader->GetNumInstructions();
			const uint32 NumVertexShaderInstructions = VertexShader->GetNumInstructions();
			ShaderComplexityPixelShader->SetParameters(NumVertexShaderInstructions,NumPixelShaderInstructions);
		}
		else
#endif
		{
			PixelShader->SetParameters(MaterialRenderProxy,*MaterialResource,View,BlendMode,bEnableEditorPrimitiveDepthTest,SceneTextureMode);

			switch(BlendMode)
			{
			default:
			case BLEND_Opaque:
				// Opaque materials are rendered together in the base pass, where the blend state is set at a higher level
				break;
			case BLEND_Masked:
				// Masked materials are rendered together in the base pass, where the blend state is set at a higher level
				break;
			case BLEND_Translucent:
				// Alpha channel is only needed for SeparateTranslucency, before this was preserving the alpha channel but we no longer store depth in the alpha channel so it's no problem
				RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_InverseSourceAlpha>::GetRHI());
				break;
			case BLEND_Additive:
				// Add to the existing scene color
				// Alpha channel is only needed for SeparateTranslucency, before this was preserving the alpha channel but we no longer store depth in the alpha channel so it's no problem
				RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add,BF_One,BF_One,BO_Add,BF_Zero,BF_InverseSourceAlpha>::GetRHI());
				break;
			case BLEND_Modulate:
				// Modulate with the existing scene color, preserve destination alpha.
				RHISetBlendState(TStaticBlendState<CW_RGB,BO_Add,BF_DestColor,BF_Zero>::GetRHI());
				break;
			};
		}
	}

	/** 
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @param DynamicStride - optional stride for dynamic vertex data
	* @return new bound shader state object
	*/
	FBoundShaderStateRHIRef CreateBoundShaderState()
	{
		FPixelShaderRHIParamRef PixelShaderRHIRef = PixelShader->GetPixelShader();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bOverrideWithShaderComplexity)
		{
			TShaderMapRef<FShaderComplexityAccumulatePS> ShaderComplexityAccumulatePixelShader(GetGlobalShaderMap());
			PixelShaderRHIRef = ShaderComplexityAccumulatePixelShader->GetPixelShader();
		}
#endif
		FBoundShaderStateRHIRef BoundShaderState;

		BoundShaderState = RHICreateBoundShaderState(
			FMeshDrawingPolicy::GetVertexDeclaration(), 
			VertexShader->GetVertexShader(),
			GETSAFERHISHADER_HULL(HullShader), 
			GETSAFERHISHADER_DOMAIN(DomainShader), 
			PixelShaderRHIRef,
			FGeometryShaderRHIRef());

		return BoundShaderState;
	}

	void SetMeshRenderState(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		const ElementDataType& ElementData
		) const
	{
		// Set the light-map policy's mesh-specific settings.
		LightMapPolicy.SetMesh(
			View,
			PrimitiveSceneProxy,
			VertexShader,
			bOverrideWithShaderComplexity ? NULL : PixelShader,
			VertexShader,
			PixelShader,
			VertexFactory,
			MaterialRenderProxy,
			ElementData.LightMapElementData);

		const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];
		VertexShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
		
		if(HullShader && DomainShader)
		{
			HullShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
			DomainShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bOverrideWithShaderComplexity)
		{
			// If we are in the translucent pass or rendering a masked material then override the blend mode, otherwise maintain opaque blending
			if (BlendMode != BLEND_Opaque)
			{
				// Add complexity to existing, keep alpha
				RHISetBlendState(TStaticBlendState<CW_RGB,BO_Add,BF_One,BF_One>::GetRHI());
			}

			TShaderMapRef<FShaderComplexityAccumulatePS> ShaderComplexityPixelShader(GetGlobalShaderMap());
			const uint32 NumPixelShaderInstructions = PixelShader->GetNumInstructions();
			const uint32 NumVertexShaderInstructions = VertexShader->GetNumInstructions();
			ShaderComplexityPixelShader->SetParameters(NumVertexShaderInstructions,NumPixelShaderInstructions);
		}
		else
#endif
		{
			PixelShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement,BlendMode);
		}

		FMeshDrawingPolicy::SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,FMeshDrawingPolicy::ElementDataType());
	}

	friend int32 CompareDrawingPolicy(const TBasePassDrawingPolicy& A,const TBasePassDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
		COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
		COMPAREDRAWINGPOLICYMEMBERS(HullShader);
		COMPAREDRAWINGPOLICYMEMBERS(DomainShader);
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		COMPAREDRAWINGPOLICYMEMBERS(SceneTextureMode);
		COMPAREDRAWINGPOLICYMEMBERS(bAllowGlobalFog);
		COMPAREDRAWINGPOLICYMEMBERS(bEnableSkyLight);

		return CompareDrawingPolicy(A.LightMapPolicy,B.LightMapPolicy);
	}

protected:
	TBasePassVertexShaderBaseType<LightMapPolicyType>* VertexShader;
	TBasePassHS<LightMapPolicyType>* HullShader;
	TBasePassDS<LightMapPolicyType>* DomainShader;
	TBasePassPixelShaderBaseType<LightMapPolicyType>* PixelShader;

	LightMapPolicyType LightMapPolicy;
	EBlendMode BlendMode;

	ESceneRenderTargetsMode::Type SceneTextureMode;

	uint32 bAllowGlobalFog : 1;

	uint32 bEnableSkyLight : 1;

	/** Whether or not this policy is compositing editor primitives and needs to depth test against the scene geometry in the base pass pixel shader */
	uint32 bEnableEditorPrimitiveDepthTest : 1;
	/** Whether or not this policy enables atmospheric fog */
	uint32 bEnableAtmosphericFog : 1;
};

/**
 * A drawing policy factory for the base pass drawing policy.
 */
class FBasePassOpaqueDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = true };
	struct ContextType 
	{
		/** Whether or not to perform depth test in the pixel shader */
		bool bEditorCompositeDepthTest;

		ESceneRenderTargetsMode::Type TextureMode;

		ContextType(bool bInEditorCompositeDepthTest, ESceneRenderTargetsMode::Type InTextureMode)
			: bEditorCompositeDepthTest( bInEditorCompositeDepthTest )
			, TextureMode( InTextureMode )
		{}
	};

	static void AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh);
	static bool DrawDynamicMesh(
		const FSceneView& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);
	static bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		// Ignore non-opaque materials in the opaque base pass.
		// Note: blend mode does not depend on the feature level.
		return MaterialRenderProxy && IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetBlendMode());
	}
};

/** The parameters used to process a base pass mesh. */
class FProcessBasePassMeshParameters
{
public:

	const FMeshBatch& Mesh;
	const uint64 BatchElementMask;
	const FMaterial* Material;
	const FPrimitiveSceneProxy* PrimitiveSceneProxy;
	EBlendMode BlendMode;
	EMaterialLightingModel LightingModel;
	const bool bAllowFog;
	/** Whether or not to perform depth test in the pixel shader */
	const bool bEditorCompositeDepthTest;
	ESceneRenderTargetsMode::Type TextureMode;

	/** Initialization constructor. */
	FProcessBasePassMeshParameters(
		const FMeshBatch& InMesh,
		const FMaterial* InMaterial,
		const FPrimitiveSceneProxy* InPrimitiveSceneProxy,
		bool InbAllowFog,
		bool bInEditorCompositeDepthTest,
		ESceneRenderTargetsMode::Type InTextureMode
		):
		Mesh(InMesh),
		BatchElementMask(Mesh.Elements.Num()==1 ? 1 : (1<<Mesh.Elements.Num())-1), // 1 bit set for each mesh element
		Material(InMaterial),
		PrimitiveSceneProxy(InPrimitiveSceneProxy),
		BlendMode(InMaterial->GetBlendMode()),
		LightingModel(InMaterial->GetLightingModel()),
		bAllowFog(InbAllowFog),
		bEditorCompositeDepthTest(bInEditorCompositeDepthTest),
		TextureMode(InTextureMode)
	{
	}

	/** Initialization constructor. */
	FProcessBasePassMeshParameters(
		const FMeshBatch& InMesh,
		const uint64& InBatchElementMask,
		const FMaterial* InMaterial,
		const FPrimitiveSceneProxy* InPrimitiveSceneProxy,
		bool InbAllowFog,
		bool bInEditorCompositeDepthTest,
		ESceneRenderTargetsMode::Type InTextureMode
		):
		Mesh(InMesh),
		BatchElementMask(InBatchElementMask),
		Material(InMaterial),
		PrimitiveSceneProxy(InPrimitiveSceneProxy),
		BlendMode(InMaterial->GetBlendMode()),
		LightingModel(InMaterial->GetLightingModel()),
		bAllowFog(InbAllowFog),
		bEditorCompositeDepthTest(bInEditorCompositeDepthTest),
		TextureMode(InTextureMode)
	{
	}
};

/** Processes a base pass mesh using an unknown light map policy, and unknown fog density policy. */
template<typename ProcessActionType>
void ProcessBasePassMesh(
	const FProcessBasePassMeshParameters& Parameters,
	const ProcessActionType& Action
	)
{
	// Check for a cached light-map.
	const bool bIsLitMaterial = Parameters.LightingModel != MLM_Unlit;
	const bool bNeedsSceneTextures = Parameters.Material->NeedsSceneTextures();
	static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));
	const bool bAllowStaticLighting = (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnRenderThread() != 0);

	// Skip rendering meshes that want to use scene textures in passes which can't bind scene textures
	// This should be avoided at a higher level when possible, because this is just a silent failure
	// This happens for example when rendering a thumbnail of an opaque post process material that uses scene textures
	if (!(Parameters.TextureMode == ESceneRenderTargetsMode::DontSet && bNeedsSceneTextures))
	{
		if (bIsLitMaterial && Action.UseTranslucentSelfShadowing())
		{
			if (GRHIFeatureLevel >= ERHIFeatureLevel::SM5)
			{
				if (IsIndirectLightingCacheAllowed()
					&& Action.AllowIndirectLightingCache()
					&& Parameters.PrimitiveSceneProxy)
				{
					// Apply cached point indirect lighting as well as self shadowing if needed
					Action.template Process<FSelfShadowedCachedPointIndirectLightingPolicy>(Parameters, FSelfShadowedCachedPointIndirectLightingPolicy(), FSelfShadowedTranslucencyPolicy::ElementDataType(Action.GetTranslucentSelfShadow()));
				}
				else
				{
					Action.template Process<FSelfShadowedTranslucencyPolicy>(Parameters, FSelfShadowedTranslucencyPolicy(), FSelfShadowedTranslucencyPolicy::ElementDataType(Action.GetTranslucentSelfShadow()));
				}
			}
		}
		else
		{
			const FLightMapInteraction LightMapInteraction = (bAllowStaticLighting && Parameters.Mesh.LCI && bIsLitMaterial) 
				? Parameters.Mesh.LCI->GetLightMapInteraction() 
				: FLightMapInteraction();

			// force LQ lightmaps based on system settings
			const bool bAllowHighQualityLightMaps = AllowHighQualityLightmaps() && LightMapInteraction.AllowsHighQualityLightmaps();

			switch(LightMapInteraction.GetType())
			{
				case LMIT_Texture: 
					if( bAllowHighQualityLightMaps ) 
					{ 
						const FShadowMapInteraction ShadowMapInteraction = (bAllowStaticLighting && Parameters.Mesh.LCI && bIsLitMaterial) 
							? Parameters.Mesh.LCI->GetShadowMapInteraction() 
							: FShadowMapInteraction();

						if (ShadowMapInteraction.GetType() == SMIT_Texture)
						{
							Action.template Process< TDistanceFieldShadowsAndLightMapPolicy<HQ_LIGHTMAP> >(
								Parameters,
								TDistanceFieldShadowsAndLightMapPolicy<HQ_LIGHTMAP>(),
								TDistanceFieldShadowsAndLightMapPolicy<HQ_LIGHTMAP>::ElementDataType(ShadowMapInteraction, LightMapInteraction));
						}
						else
						{
							Action.template Process< TLightMapPolicy<HQ_LIGHTMAP> >( Parameters, TLightMapPolicy<HQ_LIGHTMAP>(), LightMapInteraction );
						}
					} 
					else 
					{ 
						Action.template Process< TLightMapPolicy<LQ_LIGHTMAP> >( Parameters, TLightMapPolicy<LQ_LIGHTMAP>(), LightMapInteraction );
					} 
					break;
				default:
					{
						// Use simple dynamic lighting if enabled, which just renders an unshadowed directional light and a skylight
						if (bIsLitMaterial)
						{
							if (IsSimpleDynamicLightingEnabled())
							{
								Action.template Process<FSimpleDynamicLightingPolicy>(Parameters, FSimpleDynamicLightingPolicy(), FSimpleDynamicLightingPolicy::ElementDataType());
							}
							else if (IsIndirectLightingCacheAllowed()
								&& Action.AllowIndirectLightingCache()
								&& Parameters.PrimitiveSceneProxy
								// Use the indirect lighting cache shaders if the object has a cache allocation
								// This happens for objects with unbuilt lighting
								&& ((Parameters.PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation
									&& Parameters.PrimitiveSceneProxy->GetPrimitiveSceneInfo()->IndirectLightingCacheAllocation->IsValid())
									// Use the indirect lighting cache shaders if the object is movable, it may not have a cache allocation yet because that is done in InitViews
									|| Parameters.PrimitiveSceneProxy->IsMovable()))
							{
								if (CanIndirectLightingCacheUseVolumeTexture() && Action.AllowIndirectLightingCacheVolumeTexture())
								{
									// Use a lightmap policy that supports reading indirect lighting from a volume texture for dynamic objects
									Action.template Process<FCachedVolumeIndirectLightingPolicy>(Parameters,FCachedVolumeIndirectLightingPolicy(),FCachedVolumeIndirectLightingPolicy::ElementDataType());
								}
								else
								{
									// Use a lightmap policy that supports reading indirect lighting from a single SH sample
									Action.template Process<FCachedPointIndirectLightingPolicy>(Parameters,FCachedPointIndirectLightingPolicy(),FCachedPointIndirectLightingPolicy::ElementDataType(false));
								}
							}
							else
							{
								Action.template Process<FNoLightMapPolicy>(Parameters,FNoLightMapPolicy(),FNoLightMapPolicy::ElementDataType());
							}
						}
						else
						{
							Action.template Process<FNoLightMapPolicy>(Parameters,FNoLightMapPolicy(),FNoLightMapPolicy::ElementDataType());
						}
					}
					break;
			};
		}
	}
}
