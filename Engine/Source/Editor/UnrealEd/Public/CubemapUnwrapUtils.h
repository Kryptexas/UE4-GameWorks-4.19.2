// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CubemapUnwapUtils.h: Pixel and Vertex shader to render a cube map as 2D texture
=============================================================================*/

#pragma once

#include "GlobalShader.h" 
#include "SimpleElementShaders.h" 

namespace CubemapHelpers
{
	/**
	* Creates an unwrapped 2D image of the cube map ( longitude/latitude )
	* @param	CubeTexture	Source UTextureCube object.
	* @param	BitsOUT	Raw bits of the 2D image bitmap.
	* @param	SizeXOUT	Filled with the X dimension of the output bitmap.
	* @param	SizeYOUT	Filled with the Y dimension of the output bitmap.
	* @param	FormatOUT	Filled with the pixel format of the output bitmap.
	* @return	true on success.
	*/
	UNREALED_API bool GenerateLongLatUnwrap(const UTextureCube* CubeTexture, TArray<uint8>& BitsOUT, FIntPoint& SizeOUT, EPixelFormat& FormatOUT);

	/**
	* Creates an unwrapped 2D image of the cube map ( longitude/latitude )
	* @param	CubeTarget	Source UTextureRenderTargetCube object.
	* @param	BitsOUT	Raw bits of the 2D image bitmap.
	* @param	SizeXOUT	Filled with the X dimension of the output bitmap.
	* @param	SizeYOUT	Filled with the Y dimension of the output bitmap.
	* @param	FormatOUT	Filled with the pixel format of the output bitmap.
	* @return	true on success.
	*/
	UNREALED_API bool GenerateLongLatUnwrap(const UTextureRenderTargetCube* CubeTarget, TArray<uint8>& BitsOUT, FIntPoint& SizeOUT, EPixelFormat& FormatOUT);
}

/**
 * A vertex shader for rendering a texture on a simple element.
 */
class FCubemapTexturePropertiesVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCubemapTexturePropertiesVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FCubemapTexturePropertiesVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		Transform.Bind(Initializer.ParameterMap,TEXT("Transform"), SPF_Mandatory);
	}
	FCubemapTexturePropertiesVS() {}

	void SetParameters(const FMatrix& TransformValue)
	{
		SetShaderValue(GetVertexShader(), Transform, TransformValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Transform;

		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter Transform;
};

/**
 * Simple pixel shader reads from a cube map texture and unwraps it in the LongitudeLatitude form.
 */
template<bool bHDROutput>
class FCubemapTexturePropertiesPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCubemapTexturePropertiesPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FCubemapTexturePropertiesPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		CubeTexture.Bind(Initializer.ParameterMap,TEXT("CubeTexture"));
		CubeTextureSampler.Bind(Initializer.ParameterMap,TEXT("CubeTextureSampler"));
		ColorWeights.Bind(Initializer.ParameterMap,TEXT("ColorWeights"));
		PackedProperties0.Bind(Initializer.ParameterMap,TEXT("PackedProperties0"));
		Gamma.Bind(Initializer.ParameterMap,TEXT("Gamma"));
	}
	FCubemapTexturePropertiesPS() {}

	void SetParameters(const FTexture* Texture, const FMatrix& ColorWeightsValue, float MipLevel, float GammaValue)
	{
		SetTextureParameter(GetPixelShader(),CubeTexture,CubeTextureSampler,Texture);

		FVector4 PackedProperties0Value(MipLevel, 0, 0, 0);
		SetShaderValue(GetPixelShader(), PackedProperties0, PackedProperties0Value);
		SetShaderValue(GetPixelShader(), ColorWeights, ColorWeightsValue);
		SetShaderValue(GetPixelShader(), Gamma, GammaValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CubeTexture << CubeTextureSampler << PackedProperties0 << ColorWeights << Gamma;
		return bShaderHasOutdatedParameters;
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("HDR_OUTPUT"), bHDROutput ? TEXT("1") : TEXT("0"));
	}
private:
	FShaderResourceParameter CubeTexture;
	FShaderResourceParameter CubeTextureSampler;
	FShaderParameter PackedProperties0;
	FShaderParameter ColorWeights;
	FShaderParameter Gamma;
};


class UNREALED_API FMipLevelBatchedElementParameters : public FBatchedElementParameters
{
public:
	FMipLevelBatchedElementParameters(float InMipLevel, bool bInHDROutput = false)
		: MipLevel(InMipLevel), bHDROutput(bInHDROutput)
	{
	}

	/** Binds vertex and pixel shaders for this element */
	virtual void BindShaders_RenderThread(const FMatrix& InTransform, const float InGamma, const FMatrix& ColorWeights, const FTexture* Texture)
	{
		if(bHDROutput)
		{
			BindShaders_RenderThread<FCubemapTexturePropertiesPS<true> >(InTransform, InGamma, ColorWeights, Texture);
		}
		else
		{
			BindShaders_RenderThread<FCubemapTexturePropertiesPS<false> >(InTransform, InGamma, ColorWeights, Texture);
		}
	}

private:
	template<typename TPixelShader> void BindShaders_RenderThread(const FMatrix& InTransform, const float InGamma, const FMatrix& ColorWeights, const FTexture* Texture)
	{
		TShaderMapRef<FCubemapTexturePropertiesVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<TPixelShader> PixelShader(GetGlobalShaderMap());

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(BoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

		VertexShader->SetParameters(InTransform);

		RHISetBlendState(TStaticBlendState<>::GetRHI());

		PixelShader->SetParameters(Texture, ColorWeights, MipLevel, InGamma);
	}

	bool bHDROutput;
	/** Parameters that need to be passed to the shader */
	float MipLevel;
};


/**
 * Simple pixel shader that renders a IES light profile for the purposes of visualization.
 */
class FIESLightProfilePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FIESLightProfilePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	FIESLightProfilePS() {}

	FIESLightProfilePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		IESTexture.Bind(Initializer.ParameterMap,TEXT("IESTexture"));
		IESTextureSampler.Bind(Initializer.ParameterMap,TEXT("IESTextureSampler"));
		BrightnessInLumens.Bind(Initializer.ParameterMap,TEXT("BrightnessInLumens"));
	}

	void SetParameters(const FTexture* Texture, float InBrightnessInLumens)
	{
		FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		SetTextureParameter(ShaderRHI, IESTexture, IESTextureSampler, Texture);

		SetShaderValue(ShaderRHI, BrightnessInLumens, InBrightnessInLumens);
	}

	virtual bool Serialize(FArchive& Ar) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << IESTexture;
		Ar << IESTextureSampler;
		Ar << BrightnessInLumens;
		return bShaderHasOutdatedParameters;
	}

private:
	/** The texture to sample. */
	FShaderResourceParameter IESTexture;
	FShaderResourceParameter IESTextureSampler;
	FShaderParameter BrightnessInLumens;
};

class UNREALED_API FIESLightProfileBatchedElementParameters : public FBatchedElementParameters
{
public:
	FIESLightProfileBatchedElementParameters(float InBrightnessInLumens) : BrightnessInLumens(InBrightnessInLumens)
	{
	}

	/** Binds vertex and pixel shaders for this element */
	virtual void BindShaders_RenderThread( const FMatrix& InTransform, const float InGamma, const FMatrix& ColorWeights, const FTexture* Texture )
	{
		TShaderMapRef<FSimpleElementVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FIESLightProfilePS> PixelShader(GetGlobalShaderMap());

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(BoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		RHISetBlendState(TStaticBlendState<>::GetRHI());

		VertexShader->SetParameters(InTransform);
		PixelShader->SetParameters(Texture, BrightnessInLumens);
	}

private:
	float BrightnessInLumens;
};