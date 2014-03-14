// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "Engine.h"
#include "GlobalShader.h"
#include "ShaderParameters.h"

class FClearReplacementVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FClearReplacementVS,Global);
public:
	FClearReplacementVS() {}
	FClearReplacementVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}
};

class FClearReplacementPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FClearReplacementPS,Global);
public:
	FClearReplacementPS() {}
	FClearReplacementPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
		ClearColor.Bind(Initializer.ParameterMap, TEXT("ClearColor"), SPF_Mandatory);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ClearColor;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

protected:
	FShaderParameter ClearColor;
};

class FClearTexture2DReplacementCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FClearTexture2DReplacementCS,Global);
public:
	FClearTexture2DReplacementCS() {}
	FClearTexture2DReplacementCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
		ClearColor.Bind(Initializer.ParameterMap, TEXT("ClearColor"), SPF_Mandatory);
		ClearTextureRW.Bind(Initializer.ParameterMap, TEXT("ClearTextureRW"), SPF_Mandatory);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ClearColor << ClearTextureRW;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FUnorderedAccessViewRHIParamRef TextureRW, FLinearColor Value)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		SetShaderValue(ComputeShaderRHI, ClearColor, Value);
		RHISetUAVParameter(ComputeShaderRHI, ClearTextureRW.GetBaseIndex(), TextureRW);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

protected:
	FShaderParameter ClearColor;
	FShaderResourceParameter ClearTextureRW;
};

class FClearBufferReplacementCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FClearBufferReplacementCS,Global);
public:
	FClearBufferReplacementCS() {}
	FClearBufferReplacementCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader( Initializer )
	{
		ClearDword.Bind(Initializer.ParameterMap, TEXT("ClearDword"), SPF_Mandatory);
		ClearBufferRW.Bind(Initializer.ParameterMap, TEXT("ClearBufferRW"), SPF_Mandatory);
	}

	void SetParameters(FUnorderedAccessViewRHIParamRef BufferRW, uint32 Dword)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		SetShaderValue(ComputeShaderRHI, ClearDword, Dword);
		RHISetUAVParameter(ComputeShaderRHI, ClearBufferRW.GetBaseIndex(), BufferRW);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ClearDword << ClearBufferRW;
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

//protected:
	FShaderParameter ClearDword;
	FShaderResourceParameter ClearBufferRW;
};
