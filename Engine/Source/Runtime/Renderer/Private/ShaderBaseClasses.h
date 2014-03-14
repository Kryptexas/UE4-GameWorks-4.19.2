// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderBaseClasses.h: Shader base classes
=============================================================================*/

#pragma once

#include "Engine.h"
#include "MaterialShader.h"
#include "MeshMaterialShader.h"
#include "AtmosphereRendering.h"
#include "Postprocess/RenderingCompositionGraph.h"
#include "ParameterCollection.h"

template<typename ParameterType> 
struct TUniformParameter
{
	int32 Index;
	ParameterType ShaderParameter;
	friend FArchive& operator<<(FArchive& Ar,TUniformParameter<ParameterType>& P)
	{
		return Ar << P.Index << P.ShaderParameter;
	}
};

/** The uniform shader parameters associated with a LOD fade. */
// This was moved out of ScenePrivate.h to workaround MSVC vs clang template issue (it's used in this header file, so needs to be declared earlier)
BEGIN_UNIFORM_BUFFER_STRUCT(FDistanceCullFadeUniformShaderParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector2D,FadeTimeScaleBias, EShaderPrecisionModifier::Half)
END_UNIFORM_BUFFER_STRUCT(FDistanceCullFadeUniformShaderParameters)

typedef TUniformBufferRef< FDistanceCullFadeUniformShaderParameters > FDistanceCullFadeUniformBufferRef;

/**
 * Debug information related to uniform expression sets.
 */
class FDebugUniformExpressionSet
{
public:
	/** The number of each type of expression contained in the set. */
	int32 NumVectorExpressions;
	int32 NumScalarExpressions;
	int32 Num2DTextureExpressions;
	int32 NumCubeTextureExpressions;

	FDebugUniformExpressionSet()
		: NumVectorExpressions(0)
		, NumScalarExpressions(0)
		, Num2DTextureExpressions(0)
		, NumCubeTextureExpressions(0)
	{
	}

	explicit FDebugUniformExpressionSet(const FUniformExpressionSet& InUniformExpressionSet)
	{
		InitFromExpressionSet(InUniformExpressionSet);
	}

	/** Initialize from a uniform expression set. */
	void InitFromExpressionSet(const FUniformExpressionSet& InUniformExpressionSet)
	{
		NumVectorExpressions = InUniformExpressionSet.UniformVectorExpressions.Num();
		NumScalarExpressions = InUniformExpressionSet.UniformScalarExpressions.Num();
		Num2DTextureExpressions = InUniformExpressionSet.Uniform2DTextureExpressions.Num();
		NumCubeTextureExpressions = InUniformExpressionSet.UniformCubeTextureExpressions.Num();
	}

	/** Returns true if the number of uniform expressions matches those with which the debug set was initialized. */
	bool Matches(const FUniformExpressionSet& InUniformExpressionSet) const
	{
		return NumVectorExpressions == InUniformExpressionSet.UniformVectorExpressions.Num()
			&& NumScalarExpressions == InUniformExpressionSet.UniformScalarExpressions.Num()
			&& Num2DTextureExpressions == InUniformExpressionSet.Uniform2DTextureExpressions.Num()
			&& NumCubeTextureExpressions == InUniformExpressionSet.UniformCubeTextureExpressions.Num();
	}
};

/** Serialization for debug uniform expression sets. */
inline FArchive& operator<<(FArchive& Ar, FDebugUniformExpressionSet& DebugExpressionSet)
{
	Ar << DebugExpressionSet.NumVectorExpressions;
	Ar << DebugExpressionSet.NumScalarExpressions;
	Ar << DebugExpressionSet.Num2DTextureExpressions;
	Ar << DebugExpressionSet.NumCubeTextureExpressions;
	return Ar;
}


/*
class FGlobalShader : public FShader
{
	DECLARE_SHADER_TYPE(FGlobalShader,Global);
public:

	ENGINE_API FGlobalShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);


	typedef void (*ModifyCompilationEnvironmentType)(EShaderPlatform, FShaderCompilerEnvironment&);

	// FShader interface.
	ENGINE_API static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment);

*/

/** Base class of all shaders that need material parameters. */
class FMaterialShader : public FShader
{
public:
	FMaterialShader() {}

	FMaterialShader(const FMaterialShaderType::CompiledShaderInitializerType& Initializer);

	typedef void (*ModifyCompilationEnvironmentType)(EShaderPlatform, const FMaterial*, FShaderCompilerEnvironment&);

	ENGINE_API static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
	}

	FUniformBufferRHIParamRef GetParameterCollectionBuffer(const FGuid& Id, const FSceneInterface* SceneInterface) const;

	template<typename ShaderRHIParamRef>
	void SetParameters(const ShaderRHIParamRef ShaderRHI,const FSceneView& View)
	{
		check(GetUniformBufferParameter<FViewUniformShaderParameters>().IsInitialized());
		CheckShaderIsValid();
		SetUniformBufferParameter(ShaderRHI,GetUniformBufferParameter<FViewUniformShaderParameters>(),View.UniformBuffer);
	}

	/** Sets pixel parameters that are material specific but not FMeshBatch specific. */
	template<typename ShaderRHIParamRef>
	void SetParameters(
		const ShaderRHIParamRef ShaderRHI, 
		const FMaterialRenderProxy* MaterialRenderProxy, 
		const FMaterial& Material,
		const FSceneView& View, 
		bool bDeferredPass, 
		ESceneRenderTargetsMode::Type TextureMode)
	{
		ERHIFeatureLevel::Type FeatureLevel = GRHIFeatureLevel;
		FUniformExpressionCache TempUniformExpressionCache;
		const FUniformExpressionCache* UniformExpressionCache = &MaterialRenderProxy->UniformExpressionCache[FeatureLevel];

		SetParameters(ShaderRHI, View);

		// If the material has cached uniform expressions for selection or hover
		// and that is being overridden by show flags in the editor, recache
		// expressions for this draw call.
		const bool bOverrideSelection =
			GIsEditor &&
			!View.Family->EngineShowFlags.Selection &&
			(MaterialRenderProxy->IsSelected() || MaterialRenderProxy->IsHovered());

		if (!bAllowCachedUniformExpressions || !UniformExpressionCache->bUpToDate || bOverrideSelection)
		{
			FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, Material, &View);
			MaterialRenderProxy->EvaluateUniformExpressions(TempUniformExpressionCache, MaterialRenderContext);
			UniformExpressionCache = &TempUniformExpressionCache;
		}

		check(Material.GetRenderingThreadShaderMap());
		check(Material.GetRenderingThreadShaderMap()->IsValidForRendering());
		check(Material.GetFeatureLevel() == FeatureLevel);

#if NO_LOGGING == 0
		// Validate that the shader is being used for a material that matches the uniform expression set the shader was compiled for.
		const FUniformExpressionSet& MaterialUniformExpressionSet = Material.GetRenderingThreadShaderMap()->GetUniformExpressionSet();
		const bool bUniformExpressionSetMismatch = !DebugUniformExpressionSet.Matches(MaterialUniformExpressionSet)
			|| UniformExpressionCache->CachedUniformExpressionShaderMap != Material.GetRenderingThreadShaderMap();

		if (bUniformExpressionSetMismatch)
		{
			UE_LOG(
				LogShaders,
				Fatal,
				TEXT("%s shader uniform expression set mismatch for material %s/%s.\n")
				TEXT("Shader compilation info:                %s\n")
				TEXT("Material render proxy compilation info: %s\n")
				TEXT("Shader uniform expression set:   %u vectors, %u scalars, %u 2D textures, %u cube textures, shader map %p\n")
				TEXT("Material uniform expression set: %u vectors, %u scalars, %u 2D textures, %u cube textures, shader map %p\n"),
				GetType()->GetName(),
				*MaterialRenderProxy->GetFriendlyName(),
				*Material.GetFriendlyName(),
				*DebugDescription,
				*Material.GetRenderingThreadShaderMap()->GetDebugDescription(),
				DebugUniformExpressionSet.NumVectorExpressions,
				DebugUniformExpressionSet.NumScalarExpressions,
				DebugUniformExpressionSet.Num2DTextureExpressions,
				DebugUniformExpressionSet.NumCubeTextureExpressions,
				UniformExpressionCache->CachedUniformExpressionShaderMap,
				MaterialUniformExpressionSet.UniformVectorExpressions.Num(),
				MaterialUniformExpressionSet.UniformScalarExpressions.Num(),
				MaterialUniformExpressionSet.Uniform2DTextureExpressions.Num(),
				MaterialUniformExpressionSet.UniformCubeTextureExpressions.Num(),
				Material.GetRenderingThreadShaderMap()
				);
		}
#endif

		// Set the material uniform buffer.
		SetUniformBufferParameter(ShaderRHI,MaterialUniformBuffer,UniformExpressionCache->UniformBuffer);

		check(ParameterCollectionUniformBuffers.Num() >= UniformExpressionCache->ParameterCollections.Num());

		// Find each referenced parameter collection's uniform buffer in the scene and set the parameter
		for (int32 CollectionIndex = 0; CollectionIndex < UniformExpressionCache->ParameterCollections.Num(); CollectionIndex++)
		{
			FUniformBufferRHIParamRef UniformBuffer = GetParameterCollectionBuffer(UniformExpressionCache->ParameterCollections[CollectionIndex], View.Family->Scene);
			SetUniformBufferParameter(ShaderRHI,ParameterCollectionUniformBuffers[CollectionIndex],UniformBuffer);
		}

		// Set 2D texture uniform expressions.
		{
			int32 Count = Uniform2DTextureShaderResourceParameters.Num();
			for(int32 ParameterIndex = 0;ParameterIndex < Count;ParameterIndex++)
			{
				const TUniformParameter<FShaderResourceParameter>& TextureResourceParameter = Uniform2DTextureShaderResourceParameters[ParameterIndex];
				check(UniformExpressionCache->Textures.IsValidIndex(TextureResourceParameter.Index));
				const TUniformParameter<FShaderResourceParameter>& SamplerResourceParameter = Uniform2DTextureSamplerShaderResourceParameters[ParameterIndex];

				const UTexture* Texture2D = UniformExpressionCache->Textures[TextureResourceParameter.Index];
				const FTexture* Value = Texture2D ? Texture2D->Resource : NULL;

				if( !Value )
				{
					Value = GWhiteTexture;
				}

				checkSlow(Value);
				Value->LastRenderTime = GCurrentTime;

				SetTextureParameter(
					ShaderRHI, 
					TextureResourceParameter.ShaderParameter, 
					SamplerResourceParameter.ShaderParameter, 
					bDeferredPass && Value->DeferredPassSamplerStateRHI ? Value->DeferredPassSamplerStateRHI : Value->SamplerStateRHI, 
					Value->TextureRHI);
			}
		}

		// Set cube texture uniform expressions.
		{
			int32 Count = UniformCubeTextureShaderResourceParameters.Num(); 
			for(int32 ParameterIndex = 0;ParameterIndex < Count;ParameterIndex++)
			{
				const TUniformParameter<FShaderResourceParameter>& TextureResourceParameter = UniformCubeTextureShaderResourceParameters[ParameterIndex];
				check(UniformExpressionCache->CubeTextures.IsValidIndex(TextureResourceParameter.Index));
				const TUniformParameter<FShaderResourceParameter>& SamplerResourceParameter = UniformCubeTextureSamplerShaderResourceParameters[ParameterIndex];

				const UTexture* CubeTexture = UniformExpressionCache->CubeTextures[TextureResourceParameter.Index];
				const FTexture* Value = CubeTexture ? CubeTexture->Resource : NULL;

				if( !Value )
				{
					Value = GWhiteTextureCube;
				}

				checkSlow(Value);
				Value->LastRenderTime = GCurrentTime;

				SetTextureParameter(
					ShaderRHI,
					TextureResourceParameter.ShaderParameter,
					SamplerResourceParameter.ShaderParameter,
					bDeferredPass && Value->DeferredPassSamplerStateRHI ? Value->DeferredPassSamplerStateRHI : Value->SamplerStateRHI, 
					Value->TextureRHI);
			}
		}

		DeferredParameters.Set(ShaderRHI, View, TextureMode);

		AtmosphericFogTextureParameters.Set(ShaderRHI, View);

		if (GRHIFeatureLevel >= ERHIFeatureLevel::SM3)
		{
			SetTextureParameter(
				ShaderRHI,
				LightAttenuation,
				LightAttenuationSampler,
				TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				GSceneRenderTargets.GetLightAttenuationTexture());
		}

		// if we are in a postprocessing pass
		if(View.RenderingCompositePassContext)
		{
			PostprocessParameter.Set(ShaderRHI, *View.RenderingCompositePassContext, TStaticSamplerState<>::GetRHI());
		}

		//Use of the eye adaptation texture here is experimental and potentially dangerous as it can introduce a feedback loop. May be removed.
		if(EyeAdaptation.IsBound())
		{
			FTextureRHIRef& EyeAdaptationTex = GetEyeAdaptation(View);
			SetTextureParameter(ShaderRHI, EyeAdaptation, EyeAdaptationTex);
		}

		if (PerlinNoiseGradientTexture.IsBound() && IsValidRef(GSystemTextures.PerlinNoiseGradient))
		{
			const FTexture2DRHIRef& Texture = (FTexture2DRHIRef&)GSystemTextures.PerlinNoiseGradient->GetRenderTargetItem().ShaderResourceTexture;
			// Bind the PerlinNoiseGradientTexture as a texture
			SetTextureParameter(
				ShaderRHI,
				PerlinNoiseGradientTexture,
				PerlinNoiseGradientTextureSampler,
				TStaticSamplerState<SF_Point,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
				Texture
				);
		}

		if (PerlinNoise3DTexture.IsBound() && IsValidRef(GSystemTextures.PerlinNoise3D))
		{
			const FTexture3DRHIRef& Texture = (FTexture3DRHIRef&)GSystemTextures.PerlinNoise3D->GetRenderTargetItem().ShaderResourceTexture;
			// Bind the PerlinNoise3DTexture as a texture
			SetTextureParameter(
				ShaderRHI,
				PerlinNoise3DTexture,
				PerlinNoise3DTextureSampler,
				TStaticSamplerState<SF_Bilinear,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
				Texture
				);
		}
	}

	FTextureRHIRef& GetEyeAdaptation(const FSceneView& View);

	// FShader interface.
	virtual bool Serialize(FArchive& Ar);
	virtual uint32 GetAllocatedSize() const OVERRIDE;

private:

	FShaderUniformBufferParameter MaterialUniformBuffer;
	TArray<FShaderUniformBufferParameter> ParameterCollectionUniformBuffers;
	TArray<TUniformParameter<FShaderResourceParameter> > Uniform2DTextureShaderResourceParameters;
	TArray<TUniformParameter<FShaderResourceParameter> > Uniform2DTextureSamplerShaderResourceParameters;
	TArray<TUniformParameter<FShaderResourceParameter> > UniformCubeTextureShaderResourceParameters;
	TArray<TUniformParameter<FShaderResourceParameter> > UniformCubeTextureSamplerShaderResourceParameters;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter LightAttenuation;
	FShaderResourceParameter LightAttenuationSampler;

	// For materials using atmospheric fog color 
	FAtmosphereShaderTextureParameters AtmosphericFogTextureParameters;
	FPostProcessPassParameters PostprocessParameter;

//Use of the eye adaptation texture here is experimental and potentially dangerous as it can introduce a feedback loop. May be removed.
	FShaderResourceParameter EyeAdaptation;

	/** The PerlinNoiseGradientTexture parameter for materials that use GradientNoise */
	FShaderResourceParameter PerlinNoiseGradientTexture;
	FShaderResourceParameter PerlinNoiseGradientTextureSampler;
	/** The PerlinNoise3DTexture parameter for materials that use GradientNoise */
	FShaderResourceParameter PerlinNoise3DTexture;
	FShaderResourceParameter PerlinNoise3DTextureSampler;

	FDebugUniformExpressionSet DebugUniformExpressionSet;
	FString DebugDescription;

	/** If true, cached uniform expressions are allowed. */
	static int32 bAllowCachedUniformExpressions;
	/** Console variable ref to toggle cached uniform expressions. */
	static FAutoConsoleVariableRef CVarAllowCachedUniformExpressions;
};

/** Base class of all shaders that need material and vertex factory parameters. */
class FMeshMaterialShader : public FMaterialShader
{
public:
	FMeshMaterialShader() {}

	FMeshMaterialShader(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer)
		:	FMaterialShader(Initializer)
		,	VertexFactoryParameters(Initializer.VertexFactoryType,Initializer.ParameterMap,(EShaderFrequency)Initializer.Target.Frequency)
	{}

	template<typename ShaderRHIParamRef>
	void SetParameters(const ShaderRHIParamRef ShaderRHI,const FMaterialRenderProxy* MaterialRenderProxy,const FMaterial& Material,const FSceneView& View,ESceneRenderTargetsMode::Type TextureMode)
	{
		FMaterialShader::SetParameters(ShaderRHI,MaterialRenderProxy,Material,View,false,TextureMode);
	}

	void SetVFParametersOnly(const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement)
	{
		VertexFactoryParameters.SetMesh(this,VertexFactory,View,BatchElement, 0);
	}

	template<typename ShaderRHIParamRef>
	void SetMesh(const ShaderRHIParamRef ShaderRHI,const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement,uint32 DataFlags = 0)
	{
		// Set the mesh for the vertex factory
		VertexFactoryParameters.SetMesh(this,VertexFactory,View,BatchElement, DataFlags);
		
		if(IsValidRef(BatchElement.PrimitiveUniformBuffer))
		{
			SetUniformBufferParameter(ShaderRHI,GetUniformBufferParameter<FPrimitiveUniformShaderParameters>(),BatchElement.PrimitiveUniformBuffer);
		}
		else
		{
			check(BatchElement.PrimitiveUniformBufferResource);
			SetUniformBufferParameter(ShaderRHI,GetUniformBufferParameter<FPrimitiveUniformShaderParameters>(),*BatchElement.PrimitiveUniformBufferResource);
		}

		TShaderUniformBufferParameter<FDistanceCullFadeUniformShaderParameters> LODParameter = GetUniformBufferParameter<FDistanceCullFadeUniformShaderParameters>();
		if( LODParameter.IsBound() )
		{
			SetUniformBufferParameter( ShaderRHI,LODParameter,GetPrimitiveFadeUniformBufferParameter(View, Proxy));
		}
	}

	/**
	 * Retrieves the fade uniform buffer parameter from a FSceneViewState for the primitive
	 * This code was moved from SetMesh() to work around the template first-use vs first-seen differences between MSVC and others
	 */
	FUniformBufferRHIParamRef GetPrimitiveFadeUniformBufferParameter(const FSceneView& View, const FPrimitiveSceneProxy* Proxy);


	// FShader interface.
	virtual const FVertexFactoryParameterRef* GetVertexFactoryParameterRef() const { return &VertexFactoryParameters; }
	virtual bool Serialize(FArchive& Ar);
	virtual uint32 GetAllocatedSize() const OVERRIDE;

private:
	FVertexFactoryParameterRef VertexFactoryParameters;
};

/** Base Hull shader for drawing policy rendering */
class FBaseHS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FBaseHS,MeshMaterial);
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		if (!RHISupportsTessellation(Platform))
		{
			return false;
		}

		if (VertexFactoryType && !VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return false;	
		}

		if (!Material || Material->GetTessellationMode() == MTM_NoTessellation) 
		{
			// Material controls use of tessellation
			return false;	
		}

		return true;
	}

	FBaseHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
	}

	FBaseHS() {}

	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView& View)
	{
		FMeshMaterialShader::SetParameters(GetHullShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::SetTextures);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetHullShader(),VertexFactory,View,Proxy,BatchElement);
	}
};

/** Base Domain shader for drawing policy rendering */
class FBaseDS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FBaseDS,MeshMaterial);
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		if (!RHISupportsTessellation(Platform))
		{
			return false;
		}

		if (VertexFactoryType && !VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return false;	
		}

		if (!Material || Material->GetTessellationMode() == MTM_NoTessellation) 
		{
			// Material controls use of tessellation
			return false;	
		}

		return true;		
	}

	FBaseDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
	}

	FBaseDS() {}

	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView& View)
	{
		FMeshMaterialShader::SetParameters(GetDomainShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::SetTextures);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetDomainShader(),VertexFactory,View,Proxy,BatchElement);
	}
};

