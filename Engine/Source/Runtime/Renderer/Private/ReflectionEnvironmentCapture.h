// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Functionality for capturing the scene into reflection capture cubemaps, and prefiltering
=============================================================================*/

#pragma once

extern ENGINE_API int32 GReflectionCaptureSize;

extern void ComputeDiffuseIrradiance(FTextureRHIRef LightingSource, int32 LightingSourceMipIndex, FSHVectorRGB3* OutIrradianceEnvironmentMap);

class FDownsampleGS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDownsampleGS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FDownsampleGS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap,TEXT("CubeFace"));
	}
	FDownsampleGS() {}

	void SetParameters(int32 CubeFaceValue)
	{
		SetShaderValue(GetGeometryShader(), CubeFace, CubeFaceValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CubeFace;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter CubeFace;
};