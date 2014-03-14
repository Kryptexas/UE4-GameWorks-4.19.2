// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
ShaderComplexityRendering.cpp: Contains definitions for rendering the shader complexity viewmode.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessVisualizeComplexity.h"

/**
 * Gets the maximum shader complexity count from the ini settings.
 */
float GetMaxShaderComplexityCount()
{
	return GRHIFeatureLevel == ERHIFeatureLevel::ES2 ? GEngine->MaxES2PixelShaderAdditiveComplexityCount : GEngine->MaxPixelShaderAdditiveComplexityCount;
}

void FShaderComplexityAccumulatePS::SetParameters(
	uint32 NumVertexInstructions, 
	uint32 NumPixelInstructions)
{
	//normalize the complexity so we can fit it in a low precision scene color which is necessary on some platforms
	const float NormalizedComplexityValue = float(NumPixelInstructions) / GetMaxShaderComplexityCount();

	SetShaderValue( GetPixelShader(), NormalizedComplexity, NormalizedComplexityValue);
}

IMPLEMENT_SHADER_TYPE(,FShaderComplexityAccumulatePS,TEXT("ShaderComplexityAccumulatePixelShader"),TEXT("Main"),SF_Pixel);

/**
* The number of shader complexity colors from the engine ini that will be passed to the shader. 
* Changing this requires a recompile of the FShaderComplexityApplyPS.
*/
const uint32 NumShaderComplexityColors = 9;

/**
* Vertex shader used for combining LDR translucency with scene color when floating point blending isn't supported
*/
class FShaderComplexityApplyVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShaderComplexityApplyVS,Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		//this is used for shader complexity, so needs to compile for all platforms
		return true;
	}

	/** Default constructor. */
	FShaderComplexityApplyVS() {}

public:

	/** Initialization constructor. */
	FShaderComplexityApplyVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
	}
};

IMPLEMENT_SHADER_TYPE(,FShaderComplexityApplyVS,TEXT("ShaderComplexityApplyVertexShader"),TEXT("Main"),SF_Vertex);

/**
* Pixel shader that is used to map shader complexity stored in scene color into color.
*/
class FShaderComplexityApplyPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShaderComplexityApplyPS,Global);
public:

	/** 
	* Constructor - binds all shader params
	* @param Initializer - init data from shader compiler
	*/
	FShaderComplexityApplyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
		SceneTextureParams.Bind(Initializer.ParameterMap);
		ShaderComplexityColors.Bind(Initializer.ParameterMap,TEXT("ShaderComplexityColors"));
	}

	FShaderComplexityApplyPS() 
	{
	}

	/**
	* Sets the current pixel shader params
	* @param View - current view
	* @param ShadowInfo - projected shadow info for a single light
	*/
	virtual void SetParameters(
		const FSceneView* View,
		const TArray<FLinearColor>& Colors
		)
	{
		FGlobalShader::SetParameters(GetPixelShader(),*View);

		SceneTextureParams.Set(GetPixelShader());

		//Make sure there are at least NumShaderComplexityColors colors specified in the ini.
		//If there are more than NumShaderComplexityColors they will be ignored.
		check(Colors.Num() >= NumShaderComplexityColors);

		//pass the complexity -> color mapping into the pixel shader
		for(int32 ColorIndex = 0; ColorIndex < NumShaderComplexityColors; ColorIndex ++)
		{
			SetShaderValue(
				GetPixelShader(),
				ShaderComplexityColors,
				Colors[ColorIndex],
				ColorIndex
				);
		}

	}

	/**
	* @param Platform - hardware platform
	* @return true if this shader should be cached
	*/
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("NUM_COMPLEXITY_COLORS"), NumShaderComplexityColors);
	}

	/**
	* Serialize shader parameters for this shader
	* @param Ar - archive to serialize with
	*/
	bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SceneTextureParams;
		Ar << ShaderComplexityColors;
		return bShaderHasOutdatedParameters;
	}

private:

	FSceneTextureShaderParameters SceneTextureParams;
	FShaderParameter ShaderComplexityColors;
};

IMPLEMENT_SHADER_TYPE(,FShaderComplexityApplyPS,TEXT("ShaderComplexityApplyPixelShader"),TEXT("Main"),SF_Pixel);

//reuse the generic filter vertex declaration
extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

void FRCPassPostProcessVisualizeComplexity::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessVisualizeComplexity, DEC_SCENE_ITEMS);
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);
	
	FIntRect SrcRect = View.ViewRect;
	FIntRect DestRect = View.UnscaledViewRect;
	FIntPoint SrcSize = InputDesc->Extent;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());	
	Context.SetViewportAndCallRHI(DestRect);

	// turn off culling and blending
	RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	// turn off depth reads/writes
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	//reuse this generic vertex shader
	TShaderMapRef<FShaderComplexityApplyVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FShaderComplexityApplyPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState ShaderComplexityBoundShaderState;
	SetGlobalBoundShaderState(ShaderComplexityBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetParameters(&View, Colors);
	
	DrawRectangle(
		0, 0,
		DestRect.Width(), DestRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y,
		SrcRect.Width(), SrcRect.Height(),
		DestRect.Size(),
		SrcSize,
		EDRF_UseTriangleOptimization);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessVisualizeComplexity::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.Reset();
	Ret.DebugName = TEXT("VisualizeComplexity");

	return Ret;
}

