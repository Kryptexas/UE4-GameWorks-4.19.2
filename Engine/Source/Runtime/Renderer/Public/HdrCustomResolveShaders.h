// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"

class FHdrCustomResolveVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHdrCustomResolveVS,Global);
public:
	FHdrCustomResolveVS() {}
	FHdrCustomResolveVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
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
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES2);
	}
};

class FHdrCustomResolve2xPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHdrCustomResolve2xPS,Global);
public:
	FHdrCustomResolve2xPS() {}
	FHdrCustomResolve2xPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
		Tex.Bind(Initializer.ParameterMap, TEXT("Tex"), SPF_Mandatory);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Tex;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES2);
	}

	void SetParameters(FRHICommandList& RHICmdList, FTextureRHIParamRef Texture2DMS)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetTextureParameter(RHICmdList, PixelShaderRHI, Tex, Texture2DMS);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("HDR_CUSTOM_RESOLVE_2X"), 1);
	}

protected:
	FShaderResourceParameter Tex;
};

class FHdrCustomResolve4xPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHdrCustomResolve4xPS,Global);
public:
	FHdrCustomResolve4xPS() {}
	FHdrCustomResolve4xPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
		Tex.Bind(Initializer.ParameterMap, TEXT("Tex"), SPF_Mandatory);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Tex;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES2);
	}

	void SetParameters(FRHICommandList& RHICmdList, FTextureRHIParamRef Texture2DMS)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetTextureParameter(RHICmdList, PixelShaderRHI, Tex, Texture2DMS);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("HDR_CUSTOM_RESOLVE_4X"), 1);
	}

protected:
	FShaderResourceParameter Tex;
};


class FHdrCustomResolve8xPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHdrCustomResolve8xPS,Global);
public:
	FHdrCustomResolve8xPS() {}
	FHdrCustomResolve8xPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
		Tex.Bind(Initializer.ParameterMap, TEXT("Tex"), SPF_Mandatory);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Tex;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES2);
	}

	void SetParameters(FRHICommandList& RHICmdList, FTextureRHIParamRef Texture2DMS)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetTextureParameter(RHICmdList, PixelShaderRHI, Tex, Texture2DMS);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("HDR_CUSTOM_RESOLVE_8X"), 1);
	}

protected:
	FShaderResourceParameter Tex;
};

class FHdrCustomResolveFMask2xPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHdrCustomResolveFMask2xPS, Global);
public:
	FHdrCustomResolveFMask2xPS() {}
	FHdrCustomResolveFMask2xPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Tex.Bind(Initializer.ParameterMap, TEXT("Tex"), SPF_Mandatory);
		FMaskTex.Bind(Initializer.ParameterMap, TEXT("FMaskTex"), SPF_Optional);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Tex;
		Ar << FMaskTex;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES2);
	}	

	void SetParameters(FRHICommandList& RHICmdList, FTextureRHIParamRef Texture2DMS, FTextureRHIParamRef FMaskTexure2D)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetTextureParameter(RHICmdList, PixelShaderRHI, Tex, Texture2DMS);
		SetTextureParameter(RHICmdList, PixelShaderRHI, FMaskTex, FMaskTexure2D);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("HDR_RESOLVE_NUM_SAMPLES"), 2);
		OutEnvironment.SetDefine(TEXT("HDR_CUSTOM_RESOLVE_USES_FMASK"), 1);
	}

protected:
	FShaderResourceParameter Tex;
	FShaderResourceParameter FMaskTex;
};

class FHdrCustomResolveFMask4xPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHdrCustomResolveFMask4xPS, Global);
public:
	FHdrCustomResolveFMask4xPS() {}
	FHdrCustomResolveFMask4xPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Tex.Bind(Initializer.ParameterMap, TEXT("Tex"), SPF_Mandatory);
		FMaskTex.Bind(Initializer.ParameterMap, TEXT("FMaskTex"), SPF_Optional);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Tex;
		Ar << FMaskTex;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES2);
	}

	void SetParameters(FRHICommandList& RHICmdList, FTextureRHIParamRef Texture2DMS, FTextureRHIParamRef FMaskTexure2D)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetTextureParameter(RHICmdList, PixelShaderRHI, Tex, Texture2DMS);
		SetTextureParameter(RHICmdList, PixelShaderRHI, FMaskTex, FMaskTexure2D);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("HDR_RESOLVE_NUM_SAMPLES"), 4);
		OutEnvironment.SetDefine(TEXT("HDR_CUSTOM_RESOLVE_USES_FMASK"), 1);
	}

protected:
	FShaderResourceParameter Tex;
	FShaderResourceParameter FMaskTex;
};


class FHdrCustomResolveFMask8xPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FHdrCustomResolveFMask8xPS, Global);
public:
	FHdrCustomResolveFMask8xPS() {}
	FHdrCustomResolveFMask8xPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Tex.Bind(Initializer.ParameterMap, TEXT("Tex"), SPF_Mandatory);
		FMaskTex.Bind(Initializer.ParameterMap, TEXT("FMaskTex"), SPF_Optional);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Tex;
		Ar << FMaskTex;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES2);
	}

	void SetParameters(FRHICommandList& RHICmdList, FTextureRHIParamRef Texture2DMS, FTextureRHIParamRef FMaskTexure2D)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetTextureParameter(RHICmdList, PixelShaderRHI, Tex, Texture2DMS);
		SetTextureParameter(RHICmdList, PixelShaderRHI, FMaskTex, FMaskTexure2D);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("HDR_RESOLVE_NUM_SAMPLES"), 8);
		OutEnvironment.SetDefine(TEXT("HDR_CUSTOM_RESOLVE_USES_FMASK"), 1);
	}

protected:
	FShaderResourceParameter Tex;
	FShaderResourceParameter FMaskTex;
};
