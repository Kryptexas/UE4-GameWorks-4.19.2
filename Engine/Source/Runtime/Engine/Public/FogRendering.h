// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FogRendering.h: 
=============================================================================*/

#pragma once

/** Parameters needed to render exponential height fog. */
class FExponentialHeightFogShaderParameters
{
public:

	/** Binds the parameters. */
	void Bind(const FShaderParameterMap& ParameterMap);

	/** 
	 * Sets exponential height fog shader parameters.
	 */
	template<typename ShaderRHIParamRef>
	void Set(ShaderRHIParamRef Shader, const FSceneView* View) const
	{
		const FViewInfo* ViewInfo = static_cast<const FViewInfo*>(View);
		SetShaderValue(Shader, ExponentialFogParameters, ViewInfo->ExponentialFogParameters);
		SetShaderValue(Shader, ExponentialFogColorParameter, FVector4(ViewInfo->ExponentialFogColor, 1.0f - ViewInfo->FogMaxOpacity));
		SetShaderValue(Shader, InscatteringLightDirection, FVector4(ViewInfo->InscatteringLightDirection, ViewInfo->bUseDirectionalInscattering ? 1 : 0));
		SetShaderValue(Shader, DirectionalInscatteringColor, FVector4(FVector(ViewInfo->DirectionalInscatteringColor), ViewInfo->DirectionalInscatteringExponent));
		SetShaderValue(Shader, DirectionalInscatteringStartDistance, ViewInfo->DirectionalInscatteringStartDistance);
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FExponentialHeightFogShaderParameters& P);

	FShaderParameter ExponentialFogParameters;
	FShaderParameter ExponentialFogColorParameter;
	FShaderParameter InscatteringLightDirection;
	FShaderParameter DirectionalInscatteringColor;
	FShaderParameter DirectionalInscatteringStartDistance;
};

/** Encapsulates parameters needed to calculate height fog in a vertex shader. */
class FHeightFogShaderParameters
{
public:

	/** Binds the parameters. */
	void Bind(const FShaderParameterMap& ParameterMap);

	/** 
	 * Sets height fog shader parameters.
	 */
	template<typename ShaderRHIParamRef>
	void Set(ShaderRHIParamRef Shader, const FSceneView* View,const bool bAllowGlobalFog) const
	{
		// Set the fog constants.
		if (bAllowGlobalFog)
		{
			ExponentialParameters.Set(Shader, View);
		}
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FHeightFogShaderParameters& P);

private:

	FExponentialHeightFogShaderParameters ExponentialParameters;
};

extern bool ShouldRenderFog(const FSceneViewFamily& Family);