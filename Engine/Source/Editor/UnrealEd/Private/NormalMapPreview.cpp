// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	NormalMapPreview.h: Implementation for previewing normal maps.
==============================================================================*/

#include "UnrealEd.h"
#include "NormalMapPreview.h"
#include "SimpleElementShaders.h"
#include "GlobalShader.h"
#include "ShaderParameters.h"

/*------------------------------------------------------------------------------
	Batched element shaders for previewing normal maps.
------------------------------------------------------------------------------*/

/**
 * Simple pixel shader that reconstructs a normal for the purposes of visualization.
 */
class FSimpleElementNormalMapPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleElementNormalMapPS,Global);
public:

	/** Should the shader be cached? Always. */
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	/** Default constructor. */
	FSimpleElementNormalMapPS() {}

	/**
	 * Initialization constructor.
	 * @param Initializer - Shader initialization container.
	 */
	FSimpleElementNormalMapPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Texture.Bind(Initializer.ParameterMap,TEXT("NormalMapTexture"));
		TextureSampler.Bind(Initializer.ParameterMap,TEXT("NormalMapTextureSampler"));
	}

	/**
	 * Set shader parameters.
	 * @param NormalMapTexture - The normal map texture to sample.
	 */
	void SetParameters(const FTexture* NormalMapTexture)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetTextureParameter(PixelShaderRHI,Texture,TextureSampler,NormalMapTexture);
	}

	/**
	 * Serialization.
	 * @param Ar - The archive with which to serialize.
	 */
	virtual bool Serialize(FArchive& Ar) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Texture;
		Ar << TextureSampler;
		return bShaderHasOutdatedParameters;
	}

private:
	/** The texture to sample. */
	FShaderResourceParameter Texture;
	FShaderResourceParameter TextureSampler;
};
IMPLEMENT_SHADER_TYPE(,FSimpleElementNormalMapPS,TEXT("SimpleElementNormalMapPixelShader"),TEXT("Main"),SF_Pixel);

/** Binds vertex and pixel shaders for this element */
void FNormalMapBatchedElementParameters::BindShaders_RenderThread(
	const FMatrix& InTransform,
	const float InGamma,
	const FMatrix& ColorWeights,
	const FTexture* Texture)
{
	TShaderMapRef<FSimpleElementVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FSimpleElementNormalMapPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(BoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	VertexShader->SetParameters(InTransform);
	PixelShader->SetParameters(Texture);

	RHISetBlendState(TStaticBlendState<>::GetRHI());
}

