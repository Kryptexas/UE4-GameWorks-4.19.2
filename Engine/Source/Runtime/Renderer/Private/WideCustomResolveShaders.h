// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"

class FWideCustomResolveVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FWideCustomResolveVS,Global);
public:
	FWideCustomResolveVS() {}
	FWideCustomResolveVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) || (Parameters.Platform == SP_PCD3D_ES2);
	}
};

template <unsigned MSAASampleCount, unsigned Width, bool UseFMask>
class FWideCustomResolvePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FWideCustomResolvePS,Global);
public:
	FWideCustomResolvePS() {}
	FWideCustomResolvePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
		static_assert(Width >= 0 && Width <= 3, "Invalid width");
		static_assert(MSAASampleCount == 0 || MSAASampleCount == 2 || MSAASampleCount == 4 || MSAASampleCount == 8, "Invalid sample count");

		Tex.Bind(Initializer.ParameterMap, TEXT("Tex"), SPF_Mandatory);
		if (MSAASampleCount > 0)
		{
			FMaskTex.Bind(Initializer.ParameterMap, TEXT("FMaskTex"), SPF_Optional);
		}
		ResolveOrigin.Bind(Initializer.ParameterMap, TEXT("ResolveOrigin"));
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Tex;
		Ar << FMaskTex;
		Ar << ResolveOrigin;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void SetParameters(FRHICommandList& RHICmdList, FTextureRHIParamRef Texture2DMS, FTextureRHIParamRef FMaskTexture2D, FIntPoint Origin)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetTextureParameter(RHICmdList, PixelShaderRHI, Tex, Texture2DMS);
		if (MSAASampleCount > 0)
		{
			SetTextureParameter(RHICmdList, PixelShaderRHI, FMaskTex, FMaskTexture2D);
		}
		SetShaderValue(RHICmdList, PixelShaderRHI, ResolveOrigin, FVector2D(Origin));
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("WIDE_RESOLVE_WIDTH"), Width);
		OutEnvironment.SetDefine(TEXT("MSAA_SAMPLE_COUNT"), MSAASampleCount);
		OutEnvironment.SetDefine(TEXT("USE_FMASK"), UseFMask);
	}

protected:
	FShaderResourceParameter Tex;
	FShaderResourceParameter FMaskTex;
	FShaderParameter ResolveOrigin;
};

extern void ResolveFilterWide(
	FRHICommandList& RHICmdList,
	FGraphicsPipelineStateInitializer& GraphicsPSOInit,
	const ERHIFeatureLevel::Type CurrentFeatureLevel,
	const FTextureRHIRef& SrcTexture,
	const FTexture2DRHIRef& FMaskTexture,
	const FIntPoint& SrcOrigin,
	int32 NumSamples,
	int32 WideFilterWidth);
