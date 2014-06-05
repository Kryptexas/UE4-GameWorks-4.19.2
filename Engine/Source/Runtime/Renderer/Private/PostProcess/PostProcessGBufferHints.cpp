// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessGBufferHints.cpp: Post processing GBufferHints implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessGBufferHints.h"
#include "PostProcessing.h"
#include "PostProcessHistogram.h"
#include "PostProcessEyeAdaptation.h"

/** Encapsulates the post processing eye adaptation pixel shader. */
class FPostProcessGBufferHintsPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessGBufferHintsPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		FDeferredPixelShaderParameters::ModifyCompilationEnvironment(Platform,OutEnvironment);
	}

	/** Default constructor. */
	FPostProcessGBufferHintsPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter EyeAdaptationParams;
	FShaderResourceParameter MiniFontTexture;

	/** Initialization constructor. */
	FPostProcessGBufferHintsPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		EyeAdaptationParams.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationParams"));
		MiniFontTexture.Bind(Initializer.ParameterMap, TEXT("MiniFontTexture"));
	}

	void SetPS(FRHICommandList* RHICmdList, const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, Context.View);

		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View);

		{
			FVector4 Temp[3];

			FRCPassPostProcessEyeAdaptation::ComputeEyeAdaptationParamsValue(Context.View, Temp);
			SetShaderValueArray(RHICmdList, ShaderRHI, EyeAdaptationParams, Temp, 3);
		}

		SetTextureParameter(RHICmdList, ShaderRHI, MiniFontTexture, GEngine->MiniFontTexture ? GEngine->MiniFontTexture->Resource->TextureRHI : GSystemTextures.WhiteDummy->GetRenderTargetItem().TargetableTexture);
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << EyeAdaptationParams << MiniFontTexture;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessGBufferHintsPS,TEXT("PostProcessGBufferHints"),TEXT("MainPS"),SF_Pixel);

void FRCPassPostProcessGBufferHints::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(GBufferHints, DEC_SCENE_ITEMS);
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);
	
	FIntRect SrcRect = View.ViewRect;
	FIntRect DestRect = View.ViewRect;
	FIntPoint SrcSize = InputDesc->Extent;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	//@todo-rco: RHIPacketList
	FRHICommandList* RHICmdList = nullptr;

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());	
	Context.SetViewportAndCallRHI(DestRect);

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FPostProcessGBufferHintsPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetPS(RHICmdList, Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		DestRect.Width(), DestRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y,
		SrcRect.Width(), SrcRect.Height(),
		DestRect.Size(),
		SrcSize,
		*VertexShader,
		EDRF_UseTriangleOptimization);


	// this is a helper class for FCanvas to be able to get screen size
	class FRenderTargetTemp : public FRenderTarget
	{
	public:
		const FSceneView& View;
		const FTexture2DRHIRef Texture;

		FRenderTargetTemp(const FSceneView& InView, const FTexture2DRHIRef InTexture)
			: View(InView), Texture(InTexture)
		{
		}
		virtual FIntPoint GetSizeXY() const
		{
			return View.ViewRect.Size();
		};
		virtual const FTexture2DRHIRef& GetRenderTargetTexture() const
		{
			return Texture;
		}
	} TempRenderTarget(View, (const FTexture2DRHIRef&)DestRenderTarget.TargetableTexture);

	FCanvas Canvas(&TempRenderTarget, NULL, ViewFamily.CurrentRealTime, ViewFamily.CurrentWorldTime, ViewFamily.DeltaWorldTime);

	float X = 30;
	float Y = 8;
	const float YStep = 14;
	const float ColumnWidth = 250;

	FString Line;

	Line = FString::Printf(TEXT("GBufferHints"));
	Canvas.DrawShadowedString( X, Y += YStep, *Line, GetStatsFont(), FLinearColor(1, 1, 1));

	Y += YStep;
	
	Line = FString::Printf(TEXT("Yellow: Unrealistic material (In nature even black materials reflect quite some light)"));
	Canvas.DrawShadowedString( X, Y += YStep, *Line, GetStatsFont(), FLinearColor(0.8f, 0.8f, 0));

	Line = FString::Printf(TEXT("Red: Impossive material (this material emits more light than it receives)"));
	Canvas.DrawShadowedString( X, Y += YStep, *Line, GetStatsFont(), FLinearColor(1, 0, 0));

	Canvas.Flush();

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessGBufferHints::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.Reset();
	Ret.DebugName = TEXT("GBufferHints");

	return Ret;
}