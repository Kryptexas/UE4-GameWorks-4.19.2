// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
ShaderComplexityRendering.h: Declarations used for the shader complexity viewmode.
=============================================================================*/

#pragma once

/**
* Pixel shader that renders normalized shader complexity.
*/
class FShaderComplexityAccumulatePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShaderComplexityAccumulatePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	FShaderComplexityAccumulatePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FGlobalShader(Initializer)
	{
		NormalizedComplexity.Bind(Initializer.ParameterMap,TEXT("NormalizedComplexity"));
	}

	FShaderComplexityAccumulatePS() {}

	void SetParameters(
		uint32 NumVertexInstructions, 
		uint32 NumPixelInstructions);

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << NormalizedComplexity;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter NormalizedComplexity;
};


