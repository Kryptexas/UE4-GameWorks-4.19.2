// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AmbientCubemapParameters.h: Shared shader parameters for ambient cubemap
=============================================================================*/

#pragma once

/** Pixel shader parameters needed for deferred passes. */
class FCubemapShaderParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap);

	void SetParameters(const FPixelShaderRHIParamRef ShaderRHI, const FFinalPostProcessSettings::FCubemapEntry& Entry) const;
	void SetParameters(const FComputeShaderRHIParamRef ShaderRHI, const FFinalPostProcessSettings::FCubemapEntry& Entry) const;

	friend FArchive& operator<<(FArchive& Ar, FCubemapShaderParameters& P);

private:
	FShaderParameter AmbientCubemapColor;
	FShaderParameter AmbientCubemapMipAdjust;
	FShaderResourceParameter AmbientCubemap;
	FShaderResourceParameter AmbientCubemapSampler;

	template<typename TShaderRHIRef>
	void SetParametersTemplate(const TShaderRHIRef& ShaderRHI, const FFinalPostProcessSettings::FCubemapEntry& Entry) const;
};

