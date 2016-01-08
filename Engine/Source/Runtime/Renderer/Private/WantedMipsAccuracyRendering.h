// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
WantedMipsAccuracyRendering.h: Declarations used for the viewmode.
=============================================================================*/

#pragma once

static const int32 NumStreamingAccuracyColors = 5;
static const int32 MaxStreamingAccuracyMips = 11;

/**
* Pixel shader that renders texture streamer wanted mips accuracy.
*/
class FWantedMipsAccuracyPS : public FDebugViewModePS
{
	DECLARE_SHADER_TYPE(FWantedMipsAccuracyPS,Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return AllowDebugViewModeShader(Platform);
	}

	FWantedMipsAccuracyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FDebugViewModePS(Initializer)
	{
		StreamingAccuracyColorsParameter.Bind(Initializer.ParameterMap,TEXT("StreamingAccuracyColors"));
		CPUWantedMipsParameter.Bind(Initializer.ParameterMap,TEXT("CPUWantedMips"));
	}

	FWantedMipsAccuracyPS() {}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << StreamingAccuracyColorsParameter;
		Ar << CPUWantedMipsParameter;
		return bShaderHasOutdatedParameters;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("MAX_MIPS"), MaxStreamingAccuracyMips);
	}

	virtual void SetParameters(
		FRHICommandList& RHICmdList, 
		const FShader* OriginalVS, 
		const FShader* OriginalPS, 
		const FSceneView& View
		) override;

	virtual void SetMesh(
		FRHICommandList& RHICmdList, 
		const FVertexFactory* VertexFactory,
		const FSceneView& View,
		const FPrimitiveSceneProxy* Proxy,
		const FMeshBatchElement& BatchElement, 
		const FMeshDrawingRenderState& DrawRenderState
		) override;

	virtual void SetMesh(FRHICommandList& RHICmdList, const FSceneView& View) override;

private:

	FShaderParameter StreamingAccuracyColorsParameter;
	FShaderParameter CPUWantedMipsParameter;
};
