// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*=============================================================================
	SceneOcclusion.h
=============================================================================*/

/**
* A vertex shader for rendering a texture on a simple element.
*/
class FOcclusionQueryVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FOcclusionQueryVS,Global);
public:
	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3); }

	FOcclusionQueryVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
			StencilingGeometryParameters.Bind(Initializer.ParameterMap);
	}

	FOcclusionQueryVS() {}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	}

	void SetParametersWithBoundingSphere(const FSceneView& View, const FSphere& BoundingSphere)
	{
		FGlobalShader::SetParameters(GetVertexShader(), View);

		FVector4 StencilingSpherePosAndScale;
		StencilingGeometry::CalcTransform(StencilingSpherePosAndScale, BoundingSphere, View.ViewMatrices.PreViewTranslation);
		StencilingGeometryParameters.Set(this, StencilingSpherePosAndScale);
	}

	void SetParameters(const FSceneView& View)
	{
		FGlobalShader::SetParameters(GetVertexShader(),View);

		// Don't transform if rendering frustum
		StencilingGeometryParameters.Set(this, FVector4(0,0,0,1));
	}

	// Begin FShader interface
	virtual bool Serialize(FArchive& Ar) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << StencilingGeometryParameters;
		return bShaderHasOutdatedParameters;
	}
	//  End FShader interface 

private:
	FStencilingGeometryShaderParameters StencilingGeometryParameters;
};

