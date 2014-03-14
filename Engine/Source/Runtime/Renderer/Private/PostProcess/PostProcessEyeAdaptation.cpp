// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessEyeAdaptation.cpp: Post processing eye adaptation implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessEyeAdaptation.h"
#include "PostProcessing.h"

/** Encapsulates the post processing eye adaptation pixel shader. */
class FPostProcessEyeAdaptationPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessEyeAdaptationPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FPostProcessEyeAdaptationPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FShaderParameter EyeAdaptationParams;

	/** Initialization constructor. */
	FPostProcessEyeAdaptationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		EyeAdaptationParams.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationParams"));
	}

	void SetPS(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, Context.View);

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());

		{
			FVector4 Temp[3];

			FRCPassPostProcessEyeAdaptation::ComputeEyeAdaptationParamsValue(Context.View, Temp);
			SetShaderValueArray(ShaderRHI, EyeAdaptationParams, Temp, 3);
		}
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << EyeAdaptationParams;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessEyeAdaptationPS,TEXT("PostProcessEyeAdaptation"),TEXT("MainPS"),SF_Pixel);

void FRCPassPostProcessEyeAdaptation::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessEyeAdaptation, DEC_SCENE_ITEMS);

	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	IPooledRenderTarget* EyeAdaptation = Context.View.GetEyeAdaptation();
	check(EyeAdaptation);

	FIntPoint DestSize = EyeAdaptation->GetDesc().Extent;

	// we render to our own output render target, not the intermediate one created by the compositing system

	// Set the view family's render target/viewport.
	RHISetRenderTarget(EyeAdaptation->GetRenderTargetItem().TargetableTexture, FTextureRHIRef());	
	Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f );

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FPostProcessEyeAdaptationPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetPS(Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		DestSize.X, DestSize.Y,
		0, 0,
		DestSize.X, DestSize.Y,
		DestSize,
		DestSize,
		EDRF_UseTriangleOptimization);

	RHICopyToResolveTarget(EyeAdaptation->GetRenderTargetItem().TargetableTexture, EyeAdaptation->GetRenderTargetItem().ShaderResourceTexture, false, FResolveParams());
}

void FRCPassPostProcessEyeAdaptation::ComputeEyeAdaptationParamsValue(const FViewInfo& View, FVector4 Out[3])
{
	const FPostProcessSettings& Settings = View.FinalPostProcessSettings;
	const FEngineShowFlags& EngineShowFlags = View.Family->EngineShowFlags;

	float EyeAdaptationMin = Settings.AutoExposureMinBrightness;
	float EyeAdaptationMax = Settings.AutoExposureMaxBrightness;

	// FLT_MAX means no override
	float LocalOverrideExposure = FLT_MAX;

	// Eye adaptation is disabled except for highend right now because the histogram is not computed.
	if(!EngineShowFlags.EyeAdaptation || GRHIFeatureLevel != ERHIFeatureLevel::SM5)
	{
		LocalOverrideExposure = 0;
	}


	float LocalExposureMultipler = pow(2, Settings.AutoExposureBias);

	if(View.Family->ExposureSettings.bFixed)
	{
		// editor wants to override the setting with it's own fixed setting
		LocalOverrideExposure = View.Family->ExposureSettings.LogOffset;
		LocalExposureMultipler = 1;
	}

	if(LocalOverrideExposure != FLT_MAX)
	{
		// set the eye adaptation to a fixed value
		EyeAdaptationMin = EyeAdaptationMax = FMath::Pow(2.0f, -LocalOverrideExposure);
	}

	if(EyeAdaptationMin > EyeAdaptationMax)
	{
		EyeAdaptationMin = EyeAdaptationMax;
	}

	float LowPercent = FMath::Clamp(Settings.AutoExposureLowPercent, 1.0f, 99.0f) * 0.01f;
	float HighPercent = FMath::Clamp(Settings.AutoExposureHighPercent, 1.0f, 99.0f) * 0.01f;

	if(LowPercent > HighPercent)
	{
		LowPercent = HighPercent;
	}

	Out[0] = FVector4(LowPercent, HighPercent, EyeAdaptationMin, EyeAdaptationMax);

	// ----------

	Out[1] = FVector4(LocalExposureMultipler, View.Family->DeltaWorldTime, Settings.AutoExposureSpeedUp, Settings.AutoExposureSpeedDown);

	// ----------

	// example min/max: -8 .. 4   means a range from 1/256 to 4  pow(2,-8) .. pow(2,4)
	float HistogramLogMin = Settings.HistogramLogMin;
	float HistogramLogMax = Settings.HistogramLogMax;

	float DeltaLog = HistogramLogMax - HistogramLogMin;
	float Multiply = 1.0f / DeltaLog;
	float Add = - HistogramLogMin * Multiply;
	Out[2] = FVector4(Multiply, Add, 0, 0);
}

float FRCPassPostProcessEyeAdaptation::ComputeExposureScaleValue(const FViewInfo& View)
{
	FVector4 EyeAdaptationParams[3];

	ComputeEyeAdaptationParamsValue(View, EyeAdaptationParams);
	
	// like in PostProcessEyeAdaptation.usf
	float Exposure = (EyeAdaptationParams[0].Z + EyeAdaptationParams[0].W) * 0.5f;
	float ExposureScale = 1.0f / FMath::Max(0.0001f, Exposure);

	float ExposureOffsetMultipler = EyeAdaptationParams[1].X;

	return ExposureScale * ExposureOffsetMultipler;
}

FPooledRenderTargetDesc FRCPassPostProcessEyeAdaptation::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	// Specify invalid description to avoid getting intermediate rendertargets created.
	// We want to use ViewState->GetEyeAdaptation() instead
	FPooledRenderTargetDesc Ret;

	Ret.DebugName = TEXT("EyeAdaptation");

	return Ret;
}
