// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LightMapDensityRendering.h: Definitions for rendering lightmap density.
=============================================================================*/

#pragma once

/**
 * The base shader type for vertex shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 */
template<typename LightMapPolicyType>
class TLightMapDensityVS : public FMeshMaterialShader, public LightMapPolicyType::VertexParametersType
{
	DECLARE_SHADER_TYPE(TLightMapDensityVS,MeshMaterial);

public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return (Material->IsSpecialEngineMaterial() || Material->IsMasked() || Material->MaterialModifiesMeshPosition())
				&& LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType)
				&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		LightMapPolicyType::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	TLightMapDensityVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		LightMapPolicyType::VertexParametersType::Bind(Initializer.ParameterMap);
	}
	TLightMapDensityVS() {}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		LightMapPolicyType::VertexParametersType::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(		
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View
		)
	{
		FMeshMaterialShader::SetParameters(GetVertexShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::SetTextures);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(),VertexFactory,View,Proxy,BatchElement);
	}
};

/**
 * The base shader type for hull shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 */
template<typename LightMapPolicyType>
class TLightMapDensityHS : public FBaseHS
{
	DECLARE_SHADER_TYPE(TLightMapDensityHS,MeshMaterial);

public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseHS::ShouldCache(Platform, Material, VertexFactoryType)
			&& TLightMapDensityVS<LightMapPolicyType>::ShouldCache(Platform, Material, VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		LightMapPolicyType::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	TLightMapDensityHS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FBaseHS(Initializer)
	{}

	TLightMapDensityHS() {}
};

/**
 * The base shader type for domain shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 */
template<typename LightMapPolicyType>
class TLightMapDensityDS : public FBaseDS
{
	DECLARE_SHADER_TYPE(TLightMapDensityDS,MeshMaterial);

public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseDS::ShouldCache(Platform, Material, VertexFactoryType)
			&& TLightMapDensityVS<LightMapPolicyType>::ShouldCache(Platform, Material, VertexFactoryType);		
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		LightMapPolicyType::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	TLightMapDensityDS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FBaseDS(Initializer)
	{}

	TLightMapDensityDS() {}
};

/**
 * The base type for pixel shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 * The base type is shared between the versions with and without sky light.
 */
template<typename LightMapPolicyType>
class TLightMapDensityPS : public FMeshMaterialShader, public LightMapPolicyType::PixelParametersType
{
	DECLARE_SHADER_TYPE(TLightMapDensityPS,MeshMaterial);

public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return (Material->IsSpecialEngineMaterial() || Material->IsMasked() || Material->MaterialModifiesMeshPosition()) 
				&& LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType)
				&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		LightMapPolicyType::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	}

	/** Initialization constructor. */
	TLightMapDensityPS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		LightMapPolicyType::PixelParametersType::Bind(Initializer.ParameterMap);
		LightMapDensity.Bind(Initializer.ParameterMap,TEXT("LightMapDensityParameters"));
		BuiltLightingAndSelectedFlags.Bind(Initializer.ParameterMap,TEXT("BuiltLightingAndSelectedFlags"));
		DensitySelectedColor.Bind(Initializer.ParameterMap,TEXT("DensitySelectedColor"));
		LightMapResolutionScale.Bind(Initializer.ParameterMap,TEXT("LightMapResolutionScale"));
		LightMapDensityDisplayOptions.Bind(Initializer.ParameterMap,TEXT("LightMapDensityDisplayOptions"));
		VertexMappedColor.Bind(Initializer.ParameterMap,TEXT("VertexMappedColor"));
		GridTexture.Bind(Initializer.ParameterMap,TEXT("GridTexture"));
		GridTextureSampler.Bind(Initializer.ParameterMap,TEXT("GridTextureSampler"));
	}
	TLightMapDensityPS() {}

	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		FMeshMaterialShader::SetParameters(GetPixelShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),*View,ESceneRenderTargetsMode::SetTextures);

		if (GridTexture.IsBound())
		{
			SetTextureParameter(
				GetPixelShader(),
				GridTexture,
				GridTextureSampler,
				TStaticSamplerState<SF_Bilinear,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
				GEngine->LightMapDensityTexture->Resource->TextureRHI);
		}
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FPrimitiveSceneProxy* PrimitiveSceneProxy,const FMeshBatchElement& BatchElement,const FSceneView& View,bool bBackFace,FVector& InBuiltLightingAndSelectedFlags,FVector2D& InLightMapResolutionScale, bool bTextureMapped)
	{
		FMeshMaterialShader::SetMesh(GetPixelShader(),VertexFactory,View,PrimitiveSceneProxy,BatchElement);

		if (LightMapDensity.IsBound())
		{
			FVector4 DensityParameters(
				1,
				GEngine->MinLightMapDensity * GEngine->MinLightMapDensity,
				GEngine->IdealLightMapDensity * GEngine->IdealLightMapDensity,
				GEngine->MaxLightMapDensity * GEngine->MaxLightMapDensity );
			SetShaderValue(GetPixelShader(),LightMapDensity,DensityParameters,0);
		}
		SetShaderValue(GetPixelShader(),BuiltLightingAndSelectedFlags,InBuiltLightingAndSelectedFlags,0);
		SetShaderValue(GetPixelShader(),DensitySelectedColor,GEngine->LightMapDensitySelectedColor,0);
		SetShaderValue(GetPixelShader(),LightMapResolutionScale,InLightMapResolutionScale,0);
		if (LightMapDensityDisplayOptions.IsBound())
		{
			FVector4 OptionsParameter(
				GEngine->bRenderLightMapDensityGrayscale ? GEngine->RenderLightMapDensityGrayscaleScale : 0.0f,
				GEngine->bRenderLightMapDensityGrayscale ? 0.0f : GEngine->RenderLightMapDensityColorScale,
				(bTextureMapped == true) ? 1.0f : 0.0f,
				(bTextureMapped == false) ? 1.0f : 0.0f
				);
			SetShaderValue(GetPixelShader(),LightMapDensityDisplayOptions,OptionsParameter,0);
		}
		SetShaderValue(GetPixelShader(),VertexMappedColor,GEngine->LightMapDensityVertexMappedColor,0);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		LightMapPolicyType::PixelParametersType::Serialize(Ar);
		Ar << LightMapDensity;
		Ar << BuiltLightingAndSelectedFlags;
		Ar << DensitySelectedColor;
		Ar << LightMapResolutionScale;
		Ar << LightMapDensityDisplayOptions;
		Ar << VertexMappedColor;
		Ar << GridTexture;
		Ar << GridTextureSampler;
		return bShaderHasOutdatedParameters;
	}

private:
 	FShaderParameter LightMapDensity;
	FShaderParameter BuiltLightingAndSelectedFlags;
	FShaderParameter DensitySelectedColor;
	FShaderParameter LightMapResolutionScale;
	FShaderParameter LightMapDensityDisplayOptions;
	FShaderParameter VertexMappedColor;
	FShaderResourceParameter GridTexture;
	FShaderResourceParameter GridTextureSampler;
};

/**
 * 
 */
template<typename LightMapPolicyType>
class TLightMapDensityDrawingPolicy : public FMeshDrawingPolicy
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
		ElementDataType(
			const typename LightMapPolicyType::ElementDataType& InLightMapElementData
			):
			LightMapElementData(InLightMapElementData)
		{}
	};

	/** Initialization constructor. */
	TLightMapDensityDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		LightMapPolicyType InLightMapPolicy,
		EBlendMode InBlendMode
		):
		FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy, *InMaterialRenderProxy->GetMaterial(GRHIFeatureLevel)),
		LightMapPolicy(InLightMapPolicy),
		BlendMode(InBlendMode)
	{
		HullShader = NULL;
		DomainShader = NULL;

		const EMaterialTessellationMode MaterialTessellationMode = MaterialResource->GetTessellationMode();
		if(RHISupportsTessellation(GRHIShaderPlatform)
			&& InVertexFactory->GetType()->SupportsTessellationShaders() 
			&& MaterialTessellationMode != MTM_NoTessellation)
		{
			HullShader = MaterialResource->GetShader<TLightMapDensityHS<LightMapPolicyType> >(VertexFactory->GetType());
			DomainShader = MaterialResource->GetShader<TLightMapDensityDS<LightMapPolicyType> >(VertexFactory->GetType());
		}

		VertexShader = MaterialResource->GetShader<TLightMapDensityVS<LightMapPolicyType> >(InVertexFactory->GetType());
		PixelShader = MaterialResource->GetShader<TLightMapDensityPS<LightMapPolicyType> >(InVertexFactory->GetType());
	}

	// FMeshDrawingPolicy interface.
	bool Matches(const TLightMapDensityDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) &&
			VertexShader == Other.VertexShader &&
			PixelShader == Other.PixelShader &&
			HullShader == Other.HullShader &&
			DomainShader == Other.DomainShader &&
			LightMapPolicy == Other.LightMapPolicy;
	}

	void DrawShared( const FSceneView* View, FBoundShaderStateRHIRef ShaderState ) const
	{
		// Set the actual shader & vertex declaration state
		RHISetBoundShaderState(ShaderState);

		// Set the base pass shader parameters for the material.
		VertexShader->SetParameters(MaterialRenderProxy,*View);
		PixelShader->SetParameters(MaterialRenderProxy,View);

		if(HullShader && DomainShader)
		{
			HullShader->SetParameters(MaterialRenderProxy,*View);
			DomainShader->SetParameters(MaterialRenderProxy,*View);
		}

		RHISetBlendState(TStaticBlendState<>::GetRHI());

		// Set the light-map policy.
		LightMapPolicy.Set(VertexShader,PixelShader,VertexShader,PixelShader,VertexFactory,MaterialRenderProxy,View);
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
		const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

		VertexShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);

		if(HullShader && DomainShader)
		{
			HullShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
			DomainShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
		}

		// Set the light-map policy's mesh-specific settings.
		LightMapPolicy.SetMesh(View,PrimitiveSceneProxy,VertexShader,PixelShader,VertexShader,PixelShader,VertexFactory,MaterialRenderProxy,ElementData.LightMapElementData);

		// BuiltLightingAndSelectedFlags informs the shader is lighting is built or not for this primitive
		FVector BuiltLightingAndSelectedFlags(0.0f,0.0f,0.0f);
		// LMResolutionScale is the physical resolution of the lightmap texture
		FVector2D LMResolutionScale(1.0f,1.0f);

		bool bTextureMapped = false;
		if (Mesh.LCI &&
			(Mesh.LCI->GetLightMapInteraction().GetType() == LMIT_Texture) &&
			Mesh.LCI->GetLightMapInteraction().GetTexture())
		{
			LMResolutionScale.X = Mesh.LCI->GetLightMapInteraction().GetTexture()->GetSizeX();
			LMResolutionScale.Y = Mesh.LCI->GetLightMapInteraction().GetTexture()->GetSizeY();
			bTextureMapped = true;

			BuiltLightingAndSelectedFlags.X = 1.0f;
			BuiltLightingAndSelectedFlags.Y = 0.0f;
		}
		else
 		if (PrimitiveSceneProxy)
 		{
 			LMResolutionScale = PrimitiveSceneProxy->GetLightMapResolutionScale();
			BuiltLightingAndSelectedFlags.X = 0.0f;
			BuiltLightingAndSelectedFlags.Y = 1.0f;
			if (PrimitiveSceneProxy->GetLightMapType() == LMIT_Texture)
			{
				if (PrimitiveSceneProxy->IsLightMapResolutionPadded() == true)
				{
					LMResolutionScale.X -= 2.0f;
					LMResolutionScale.Y -= 2.0f;
				}
				bTextureMapped = true;
				if (PrimitiveSceneProxy->HasStaticLighting())
				{
					BuiltLightingAndSelectedFlags.X = 1.0f;
					BuiltLightingAndSelectedFlags.Y = 0.0f;
				}
			}
		}

		if (Mesh.MaterialRenderProxy && (Mesh.MaterialRenderProxy->IsSelected() == true))
		{
			BuiltLightingAndSelectedFlags.Z = 1.0f;
		}
		else
		{
			BuiltLightingAndSelectedFlags.Z = 0.0f;
		}

		// Adjust for the grid texture being 2x2 repeating pattern...
		LMResolutionScale *= 0.5f;
		PixelShader->SetMesh(VertexFactory,PrimitiveSceneProxy, BatchElement, View, bBackFace, BuiltLightingAndSelectedFlags, LMResolutionScale, bTextureMapped);	
		FMeshDrawingPolicy::SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,FMeshDrawingPolicy::ElementDataType());
	}

	/** 
	 * Create bound shader state using the vertex decl from the mesh draw policy
	 * as well as the shaders needed to draw the mesh
	 * @return new bound shader state object
	 */
	FBoundShaderStateRHIRef CreateBoundShaderState()
	{
		FBoundShaderStateRHIRef BoundShaderState;

		BoundShaderState = RHICreateBoundShaderState(
			FMeshDrawingPolicy::GetVertexDeclaration(), 
			VertexShader->GetVertexShader(),
			GETSAFERHISHADER_HULL(HullShader), 
			GETSAFERHISHADER_DOMAIN(DomainShader),
			PixelShader->GetPixelShader(),
			FGeometryShaderRHIRef());

		return BoundShaderState;
	}

	friend int32 CompareDrawingPolicy(const TLightMapDensityDrawingPolicy& A,const TLightMapDensityDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
		COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
		COMPAREDRAWINGPOLICYMEMBERS(HullShader);
		COMPAREDRAWINGPOLICYMEMBERS(DomainShader);
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		return CompareDrawingPolicy(A.LightMapPolicy,B.LightMapPolicy);
	}

private:
	TLightMapDensityVS<LightMapPolicyType>* VertexShader;
	TLightMapDensityPS<LightMapPolicyType>* PixelShader;
	TLightMapDensityHS<LightMapPolicyType>* HullShader;
	TLightMapDensityDS<LightMapPolicyType>* DomainShader;

	LightMapPolicyType LightMapPolicy;
	EBlendMode BlendMode;
};

/**
 * A drawing policy factory for rendering lightmap density.
 */
class FLightMapDensityDrawingPolicyFactory
{
public:
	enum { bAllowSimpleElements = false };
	struct ContextType {};

	static bool DrawDynamicMesh(
		const FViewInfo& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);
	static bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		// Note: MaterialModifiesMeshPosition may depend on feature level!
		const FMaterial* Material = MaterialRenderProxy ? MaterialRenderProxy->GetMaterial(GRHIFeatureLevel) : NULL;
		return (Material && 
				!(	Material->IsSpecialEngineMaterial() || 
					Material->IsMasked() ||
					Material->MaterialModifiesMeshPosition()
				));
	}
};
