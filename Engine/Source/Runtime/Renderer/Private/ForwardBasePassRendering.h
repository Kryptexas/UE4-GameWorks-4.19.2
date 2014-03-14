// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ForwardBasePassRendering.h: base pass rendering definitions.
=============================================================================*/

#pragma once

#include "LightMapRendering.h"
#include "ShaderBaseClasses.h"

enum EOutputFormat
{
	LDR_GAMMA_32,
	HDR_LINEAR_32,
	HDR_LINEAR_64,
};

/**
 * The base shader type for vertex shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 */
template<typename LightMapPolicyType>
class TBasePassForForwardShadingVSBaseType : public FMeshMaterialShader, public LightMapPolicyType::VertexParametersType
{
	DECLARE_SHADER_TYPE(TBasePassForForwardShadingVSBaseType,MeshMaterial);

protected:

	TBasePassForForwardShadingVSBaseType() {}
	TBasePassForForwardShadingVSBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		LightMapPolicyType::VertexParametersType::Bind(Initializer.ParameterMap);
		HeightFogParameters.Bind(Initializer.ParameterMap);
	}

public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsES2Platform(Platform) && LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType);
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
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FVertexFactory* VertexFactory,
		const FMaterial& InMaterialResource,
		const FSceneView& View,
		ESceneRenderTargetsMode::Type TextureMode
		)
	{
		HeightFogParameters.Set(GetVertexShader(), &View, true);
		FMeshMaterialShader::SetParameters(GetVertexShader(),MaterialRenderProxy,InMaterialResource,View,TextureMode);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(),VertexFactory,View,Proxy,BatchElement);
	}

private:
	FHeightFogShaderParameters HeightFogParameters;
};

template< typename LightMapPolicyType, EOutputFormat OutputFormat >
class TBasePassForForwardShadingVS : public TBasePassForForwardShadingVSBaseType<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TBasePassForForwardShadingVS,MeshMaterial);
public:
	
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		TBasePassForForwardShadingVSBaseType<LightMapPolicyType>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine( TEXT("OUTPUT_GAMMA_SPACE"), OutputFormat == LDR_GAMMA_32 ? 1u : 0u );
	}
	
	/** Initialization constructor. */
	TBasePassForForwardShadingVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		TBasePassForForwardShadingVSBaseType<LightMapPolicyType>(Initializer)
	{}

	/** Default constructor. */
	TBasePassForForwardShadingVS() {}
};


/**
 * The base type for pixel shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 */
template<typename LightMapPolicyType>
class TBasePassForForwardShadingPSBaseType : public FMeshMaterialShader, public LightMapPolicyType::PixelParametersType
{
	DECLARE_SHADER_TYPE(TBasePassForForwardShadingPSBaseType,MeshMaterial);

public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsES2Platform(Platform) && LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		LightMapPolicyType::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	TBasePassForForwardShadingPSBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		LightMapPolicyType::PixelParametersType::Bind(Initializer.ParameterMap);
		ReflectionCubemap.Bind(Initializer.ParameterMap, TEXT("ReflectionCubemap"));
		ReflectionSampler.Bind(Initializer.ParameterMap, TEXT("ReflectionCubemapSampler"));
	}
	TBasePassForForwardShadingPSBaseType() {}

	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FMaterial& MaterialResource,const FSceneView* View,ESceneRenderTargetsMode::Type TextureMode)
	{
		FMeshMaterialShader::SetParameters(GetPixelShader(),MaterialRenderProxy,MaterialResource,*View,TextureMode);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		if (ReflectionCubemap.IsBound())
		{
			FTexture* ReflectionTexture = GBlackTextureCube;
			FPrimitiveSceneInfo* PrimitiveSceneInfo = Proxy ? Proxy->GetPrimitiveSceneInfo() : NULL;

			if (PrimitiveSceneInfo 
				&& PrimitiveSceneInfo->CachedReflectionCaptureProxy
				&& PrimitiveSceneInfo->CachedReflectionCaptureProxy->EncodedHDRCubemap
				&& PrimitiveSceneInfo->CachedReflectionCaptureProxy->EncodedHDRCubemap->IsInitialized())
			{
				ReflectionTexture = PrimitiveSceneInfo->CachedReflectionCaptureProxy->EncodedHDRCubemap;
			}

			// Set the reflection cubemap
			SetTextureParameter(GetPixelShader(), ReflectionCubemap, ReflectionSampler, ReflectionTexture);
		}

		FMeshMaterialShader::SetMesh(GetPixelShader(),VertexFactory,View,Proxy,BatchElement);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		LightMapPolicyType::PixelParametersType::Serialize(Ar);
		Ar << ReflectionCubemap;
		Ar << ReflectionSampler;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter ReflectionCubemap;
	FShaderResourceParameter ReflectionSampler;
};


template< typename LightMapPolicyType, EOutputFormat OutputFormat>
class TBasePassForForwardShadingPS : public TBasePassForForwardShadingPSBaseType<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TBasePassForForwardShadingPS,MeshMaterial);
public:
	
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		TBasePassForForwardShadingPSBaseType<LightMapPolicyType>::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine( TEXT("USE_HDR_MOSAIC"), OutputFormat == HDR_LINEAR_32 ? 1u : 0u );
		OutEnvironment.SetDefine( TEXT("OUTPUT_GAMMA_SPACE"), OutputFormat == LDR_GAMMA_32 ? 1u : 0u );
	}
	
	/** Initialization constructor. */
	TBasePassForForwardShadingPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		TBasePassForForwardShadingPSBaseType<LightMapPolicyType>(Initializer)
	{}

	/** Default constructor. */
	TBasePassForForwardShadingPS() {}
};


/**
 * Draws the emissive color and the light-map of a mesh.
 */
template<typename LightMapPolicyType>
class TBasePassForForwardShadingDrawingPolicy : public FMeshDrawingPolicy
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
	TBasePassForForwardShadingDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		LightMapPolicyType InLightMapPolicy,
		EBlendMode InBlendMode,
		ESceneRenderTargetsMode::Type InSceneTextureMode,
		bool bOverrideWithShaderComplexity = false
		):
		FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy,InMaterialResource,bOverrideWithShaderComplexity),
		LightMapPolicy(InLightMapPolicy),
		BlendMode(InBlendMode),
		SceneTextureMode(InSceneTextureMode)
	{
		if (IsMobileHDR32bpp())
		{
			VertexShader = InMaterialResource.GetShader<TBasePassForForwardShadingVS<LightMapPolicyType, HDR_LINEAR_64> >(InVertexFactory->GetType());
			PixelShader = InMaterialResource.GetShader< TBasePassForForwardShadingPS<LightMapPolicyType, HDR_LINEAR_32> >(InVertexFactory->GetType());
		}
		else if (IsMobileHDR())
		{
			VertexShader = InMaterialResource.GetShader<TBasePassForForwardShadingVS<LightMapPolicyType, HDR_LINEAR_64> >(InVertexFactory->GetType());
			PixelShader = InMaterialResource.GetShader< TBasePassForForwardShadingPS<LightMapPolicyType, HDR_LINEAR_64> >(InVertexFactory->GetType());
		}
		else
		{
			VertexShader = InMaterialResource.GetShader<TBasePassForForwardShadingVS<LightMapPolicyType, LDR_GAMMA_32> >(InVertexFactory->GetType());
			PixelShader = InMaterialResource.GetShader< TBasePassForForwardShadingPS<LightMapPolicyType, LDR_GAMMA_32> >(InVertexFactory->GetType());
		}
	}

	// FMeshDrawingPolicy interface.

	bool Matches(const TBasePassForForwardShadingDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) &&
			VertexShader == Other.VertexShader &&
			PixelShader == Other.PixelShader &&
			LightMapPolicy == Other.LightMapPolicy &&
			SceneTextureMode == Other.SceneTextureMode;
	}

	void DrawShared(const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const
	{
		// Set the actual shader & vertex declaration state
		RHISetBoundShaderState(BoundShaderState);

		VertexShader->SetParameters(MaterialRenderProxy, VertexFactory, *MaterialResource, *View, SceneTextureMode);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bOverrideWithShaderComplexity)
		{
			if (BlendMode == BLEND_Opaque)
			{
				RHISetBlendState(TStaticBlendStateWriteMask<CW_RGBA>::GetRHI());
			}
			else
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
			PixelShader->SetParameters(MaterialRenderProxy, *MaterialResource, View, SceneTextureMode);

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
				RHISetBlendState(TStaticBlendState<CW_RGB, BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_InverseSourceAlpha>::GetRHI());
				break;
			case BLEND_Additive:
				// Add to the existing scene color
				RHISetBlendState(TStaticBlendState<CW_RGB, BO_Add,BF_One,BF_One,BO_Add,BF_Zero,BF_InverseSourceAlpha>::GetRHI());
				break;
			case BLEND_Modulate:
				// Modulate with the existing scene color
				RHISetBlendState(TStaticBlendState<CW_RGB,BO_Add,BF_DestColor,BF_Zero>::GetRHI());
				break;
			};
		}
		
		// Set the light-map policy.
		LightMapPolicy.Set(VertexShader,bOverrideWithShaderComplexity ? NULL : PixelShader,VertexShader,PixelShader,VertexFactory,MaterialRenderProxy,View);		
	}

	/** 
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
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

		FBoundShaderStateRHIRef BoundShaderState = RHICreateBoundShaderState(
			FMeshDrawingPolicy::GetVertexDeclaration(), 
			VertexShader->GetVertexShader(),
			FHullShaderRHIRef(), 
			FDomainShaderRHIRef(), 
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
			PixelShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
		}

		FMeshDrawingPolicy::SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,FMeshDrawingPolicy::ElementDataType());
	}

	friend int32 CompareDrawingPolicy(const TBasePassForForwardShadingDrawingPolicy& A,const TBasePassForForwardShadingDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
		COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		COMPAREDRAWINGPOLICYMEMBERS(SceneTextureMode);

		return CompareDrawingPolicy(A.LightMapPolicy,B.LightMapPolicy);
	}

protected:
	TBasePassForForwardShadingVSBaseType<LightMapPolicyType>* VertexShader;
	TBasePassForForwardShadingPSBaseType<LightMapPolicyType>* PixelShader;

	LightMapPolicyType LightMapPolicy;
	EBlendMode BlendMode;
	ESceneRenderTargetsMode::Type SceneTextureMode;
};

/**
 * A drawing policy factory for the base pass drawing policy.
 */
class FBasePassForwardOpaqueDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = true };
	struct ContextType 
	{
		ContextType(ESceneRenderTargetsMode::Type InTextureMode) :
			TextureMode(InTextureMode)
		{}

		ESceneRenderTargetsMode::Type TextureMode;
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

/** Processes a base pass mesh using an unknown light map policy, and unknown fog density policy. */
template<typename ProcessActionType>
void ProcessBasePassMeshForForwardShading(
	const FProcessBasePassMeshParameters& Parameters,
	const ProcessActionType& Action
	)
{
	// Check for a cached light-map.
	const bool bIsLitMaterial = Parameters.LightingModel != MLM_Unlit;

	const FLightMapInteraction LightMapInteraction = (Parameters.Mesh.LCI && bIsLitMaterial) 
		? Parameters.Mesh.LCI->GetLightMapInteraction() 
		: FLightMapInteraction();

	check(!AllowHighQualityLightmaps());

	if (LightMapInteraction.GetType() == LMIT_Texture)
	{
		const FShadowMapInteraction ShadowMapInteraction = (Parameters.Mesh.LCI && bIsLitMaterial) 
			? Parameters.Mesh.LCI->GetShadowMapInteraction() 
			: FShadowMapInteraction();

		if (ShadowMapInteraction.GetType() == SMIT_Texture)
		{
			Action.template Process< TDistanceFieldShadowsAndLightMapPolicy<LQ_LIGHTMAP> >(
				Parameters,
				TDistanceFieldShadowsAndLightMapPolicy<LQ_LIGHTMAP>(),
				TDistanceFieldShadowsAndLightMapPolicy<LQ_LIGHTMAP>::ElementDataType(ShadowMapInteraction, LightMapInteraction));
		}
		else
		{
			Action.template Process< TLightMapPolicy<LQ_LIGHTMAP> >( Parameters, TLightMapPolicy<LQ_LIGHTMAP>(), LightMapInteraction );
		}
	}
	else if (bIsLitMaterial 
		&& IsIndirectLightingCacheAllowed()
		&& Parameters.PrimitiveSceneProxy
		// Movable objects need to get their GI from the indirect lighting cache
		&& Parameters.PrimitiveSceneProxy->IsMovable())
	{
		Action.template Process<FSimpleDirectionalLightAndSHIndirectPolicy>(Parameters,FSimpleDirectionalLightAndSHIndirectPolicy(),FSimpleDirectionalLightAndSHIndirectPolicy::ElementDataType(Action.ShouldPackAmbientSH())); 
	}
	else
	{
		Action.template Process<FNoLightMapPolicy>(Parameters,FNoLightMapPolicy(),FNoLightMapPolicy::ElementDataType());
	}
}
