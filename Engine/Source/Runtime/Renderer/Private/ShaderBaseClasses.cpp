// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderBaseClasses.cpp: Shader base classes
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"

/** If true, cached uniform expressions are allowed. */
int32 FMaterialShader::bAllowCachedUniformExpressions = true;

/** Console variable ref to toggle cached uniform expressions. */
FAutoConsoleVariableRef FMaterialShader::CVarAllowCachedUniformExpressions(
	TEXT("AllowCachedUniformExpressions"),
	bAllowCachedUniformExpressions,
	TEXT("Allow uniform expressions to be cached."),
	ECVF_RenderThreadSafe);

FMaterialShader::FMaterialShader(const FMaterialShaderType::CompiledShaderInitializerType& Initializer)
:	FShader(Initializer)
,	DebugUniformExpressionSet(Initializer.UniformExpressionSet)
,	DebugDescription(Initializer.DebugDescription)
{
	check(!DebugDescription.IsEmpty());

	// Bind the material uniform buffer parameter.
	MaterialUniformBuffer.Bind(Initializer.ParameterMap,TEXT("Material"));

	for (int32 CollectionIndex = 0; CollectionIndex < Initializer.UniformExpressionSet.ParameterCollections.Num(); CollectionIndex++)
	{
		FShaderUniformBufferParameter CollectionParameter;
		CollectionParameter.Bind(Initializer.ParameterMap,*FString::Printf(TEXT("MaterialCollection%u"), CollectionIndex));
		ParameterCollectionUniformBuffers.Add(CollectionParameter);
	}

	// Bind uniform 2D texture parameters.
	for(int32 ParameterIndex = 0;ParameterIndex < Initializer.UniformExpressionSet.Uniform2DTextureExpressions.Num();ParameterIndex++)
	{
		FShaderResourceParameter TextureShaderParameter;
		FString TextureParameterName = FString::Printf(TEXT("MaterialTexture2D_%u"), ParameterIndex);
		TextureShaderParameter.Bind(Initializer.ParameterMap,*TextureParameterName);

		FShaderResourceParameter SamplerShaderParameter;
		FString SamplerParameterName = FString::Printf(TEXT("MaterialTexture2D_%uSampler"), ParameterIndex);
		SamplerShaderParameter.Bind(Initializer.ParameterMap,*SamplerParameterName);

		if(TextureShaderParameter.IsBound() && SamplerShaderParameter.IsBound())
		{
			TUniformParameter<FShaderResourceParameter>* UniformParameter = new(Uniform2DTextureShaderResourceParameters) TUniformParameter<FShaderResourceParameter>();
			UniformParameter->Index = ParameterIndex;
			UniformParameter->ShaderParameter = TextureShaderParameter;

			UniformParameter = new(Uniform2DTextureSamplerShaderResourceParameters) TUniformParameter<FShaderResourceParameter>();
			UniformParameter->Index = ParameterIndex;
			UniformParameter->ShaderParameter = SamplerShaderParameter;
		}
	}
	// Bind uniform cube texture parameters.
	for(int32 ParameterIndex = 0;ParameterIndex < Initializer.UniformExpressionSet.UniformCubeTextureExpressions.Num();ParameterIndex++)
	{
		FShaderResourceParameter TextureShaderParameter;
		FString TextureParameterName = FString::Printf(TEXT("MaterialTextureCube_%u"), ParameterIndex);
		TextureShaderParameter.Bind(Initializer.ParameterMap,*TextureParameterName);

		FShaderResourceParameter SamplerShaderParameter;
		FString SamplerParameterName = FString::Printf(TEXT("MaterialTextureCube_%uSampler"), ParameterIndex);
		SamplerShaderParameter.Bind(Initializer.ParameterMap,*SamplerParameterName);

		if(TextureShaderParameter.IsBound() && SamplerShaderParameter.IsBound())
		{
			TUniformParameter<FShaderResourceParameter>* UniformParameter = new(UniformCubeTextureShaderResourceParameters) TUniformParameter<FShaderResourceParameter>();
			UniformParameter->Index = ParameterIndex;
			UniformParameter->ShaderParameter = TextureShaderParameter;

			UniformParameter = new(UniformCubeTextureSamplerShaderResourceParameters) TUniformParameter<FShaderResourceParameter>();
			UniformParameter->Index = ParameterIndex;
			UniformParameter->ShaderParameter = SamplerShaderParameter;
		}
	}

	DeferredParameters.Bind(Initializer.ParameterMap);
	LightAttenuation.Bind(Initializer.ParameterMap, TEXT("LightAttenuationTexture"));
	LightAttenuationSampler.Bind(Initializer.ParameterMap, TEXT("LightAttenuationTextureSampler"));
	AtmosphericFogTextureParameters.Bind(Initializer.ParameterMap);
	PostprocessParameter.Bind(Initializer.ParameterMap);
	EyeAdaptation.Bind(Initializer.ParameterMap, TEXT("EyeAdaptation"));
	// only used it Material has expression that requires PerlinNoiseGradientTexture
	PerlinNoiseGradientTexture.Bind(Initializer.ParameterMap,TEXT("PerlinNoiseGradientTexture"));
	PerlinNoiseGradientTextureSampler.Bind(Initializer.ParameterMap,TEXT("PerlinNoiseGradientTextureSampler"));
	// only used it Material has expression that requires PerlinNoise3DTexture
	PerlinNoise3DTexture.Bind(Initializer.ParameterMap,TEXT("PerlinNoise3DTexture"));
	PerlinNoise3DTextureSampler.Bind(Initializer.ParameterMap,TEXT("PerlinNoise3DTextureSampler"));
}

FUniformBufferRHIParamRef FMaterialShader::GetParameterCollectionBuffer(const FGuid& Id, const FSceneInterface* SceneInterface) const
{
	const FScene* Scene = (const FScene*)SceneInterface;
	return Scene ? Scene->GetParameterCollectionBuffer(Id) : FUniformBufferRHIParamRef();
}

bool FMaterialShader::Serialize(FArchive& Ar)
{
	const bool bShaderHasOutdatedParameters = FShader::Serialize(Ar);
	Ar << MaterialUniformBuffer;
	Ar << ParameterCollectionUniformBuffers;
	Ar << Uniform2DTextureShaderResourceParameters;
	Ar << Uniform2DTextureSamplerShaderResourceParameters;
	Ar << UniformCubeTextureShaderResourceParameters;
	Ar << UniformCubeTextureSamplerShaderResourceParameters;
	Ar << DeferredParameters;
	Ar << LightAttenuation;
	Ar << LightAttenuationSampler;
	if (Ar.UE4Ver() >= VER_UE4_SMALLER_DEBUG_MATERIALSHADER_UNIFORM_EXPRESSIONS)
	{
		Ar << DebugUniformExpressionSet;
		Ar << DebugDescription;
	}
	else if (Ar.IsLoading() && Ar.UE4Ver() >= VER_DEBUG_MATERIALSHADER_UNIFORM_EXPRESSIONS)
	{
		FUniformExpressionSet TempExpressionSet;
		TempExpressionSet.Serialize(Ar);
		DebugUniformExpressionSet.InitFromExpressionSet(TempExpressionSet);

		Ar << DebugDescription;
	}
	Ar << AtmosphericFogTextureParameters;
	Ar << PostprocessParameter;
	Ar << EyeAdaptation;
	Ar << PerlinNoiseGradientTexture;
	Ar << PerlinNoiseGradientTextureSampler;
	Ar << PerlinNoise3DTexture;
	Ar << PerlinNoise3DTextureSampler;
	return bShaderHasOutdatedParameters;
}

FTextureRHIRef& FMaterialShader::GetEyeAdaptation(const FSceneView& View)
{
	IPooledRenderTarget* EyeAdaptationRT = NULL;
	if( View.bIsViewInfo )
	{
		const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
		EyeAdaptationRT = ViewInfo.GetEyeAdaptation();
	}

	if( EyeAdaptationRT )
	{
		return EyeAdaptationRT->GetRenderTargetItem().TargetableTexture;
	}
	
	return GWhiteTexture->TextureRHI;
}

uint32 FMaterialShader::GetAllocatedSize() const
{
	return FShader::GetAllocatedSize()
		+ ParameterCollectionUniformBuffers.GetAllocatedSize()
		+ Uniform2DTextureShaderResourceParameters.GetAllocatedSize()
		+ Uniform2DTextureSamplerShaderResourceParameters.GetAllocatedSize()
		+ UniformCubeTextureShaderResourceParameters.GetAllocatedSize()
		+ UniformCubeTextureSamplerShaderResourceParameters.GetAllocatedSize()
		+ DebugDescription.GetAllocatedSize();
}

bool FMeshMaterialShader::Serialize(FArchive& Ar)
{
	bool bShaderHasOutdatedParameters = FMaterialShader::Serialize(Ar);
	bShaderHasOutdatedParameters |= Ar << VertexFactoryParameters;
	return bShaderHasOutdatedParameters;
}

uint32 FMeshMaterialShader::GetAllocatedSize() const
{
	return FMaterialShader::GetAllocatedSize()
		+ VertexFactoryParameters.GetAllocatedSize();
}

FUniformBufferRHIParamRef FMeshMaterialShader::GetPrimitiveFadeUniformBufferParameter(const FSceneView& View, const FPrimitiveSceneProxy* Proxy)
{
	FUniformBufferRHIParamRef FadeUniformBuffer = NULL;
	if( Proxy != NULL )
	{
		const FPrimitiveSceneInfo* PrimitiveSceneInfo = Proxy->GetPrimitiveSceneInfo();
		int32 PrimitiveIndex = PrimitiveSceneInfo->GetIndex();

		// This cast should always be safe. Check it :)
		checkSlow(View.bIsViewInfo);
		const FViewInfo& ViewInfo = (const FViewInfo&)View;
		FadeUniformBuffer = ViewInfo.PrimitiveFadeUniformBuffers[PrimitiveIndex];
	}
	if (FadeUniformBuffer == NULL)
	{
		FadeUniformBuffer = GDistanceCullFadedInUniformBuffer.GetUniformBufferRHI();
	}
	return FadeUniformBuffer;
}
