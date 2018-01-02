// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessEyeAdaptation.cpp: Post processing eye adaptation implementation.
=============================================================================*/

#include "PostProcess/PostProcessEyeAdaptation.h"
#include "PostProcess/SceneFilterRendering.h"
#include "PostProcess/PostProcessing.h"
#include "ClearQuad.h"
#include "PipelineStateCache.h"
#include "UnrealMathUtility.h"
#include "ScenePrivate.h"

SHADERCORE_API bool UsePreExposure(EShaderPlatform Platform);

// Initialize the static CVar
TAutoConsoleVariable<float> CVarEyeAdaptationPreExposureOverride(
	TEXT("r.EyeAdaptation.PreExposureOverride"),
	0,
	TEXT("Overide the scene pre-exposure by a custom value. \n")
	TEXT("= 0 : No override\n")
	TEXT("> 0 : Override PreExposure\n"),
	ECVF_RenderThreadSafe);

// Initialize the static CVar
TAutoConsoleVariable<int32> CVarEyeAdaptationMethodOverride(
	TEXT("r.EyeAdaptation.MethodOveride"),
	-1,
	TEXT("Overide the camera metering method set in post processing volumes\n")
	TEXT("-2: override with custom settings (for testing Basic Mode)\n")
	TEXT("-1: no override\n")
	TEXT(" 1: Auto Histogram-based\n")
	TEXT(" 2: Auto Basic\n")
	TEXT(" 3: Manual"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

// Initialize the static CVar used in computing the weighting focus in basic eye-adaptation
TAutoConsoleVariable<float> CVarEyeAdaptationFocus(
	TEXT("r.EyeAdaptation.Focus"),
	1.0f,
	TEXT("Applies to basic adapation mode only\n")
	TEXT(" 0: Uniform weighting\n")
	TEXT(">0: Center focus, 1 is a good number (default)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

/**
 *   Shared functionality used in computing the eye-adaptation parameters
 *   Compute the parameters used for eye-adaptation.  These will default to values
 *   that disable eye-adaptation if the hardware doesn't support the minimum feature level
 */
inline static void ComputeEyeAdaptationValues(const ERHIFeatureLevel::Type MinFeatureLevel, const FViewInfo& View, FVector4 Out[EYE_ADAPTATION_PARAMS_SIZE])
{
	const FPostProcessSettings& Settings = View.FinalPostProcessSettings;
	const FEngineShowFlags& EngineShowFlags = View.Family->EngineShowFlags;

	float LowPercent = FMath::Clamp(Settings.AutoExposureLowPercent, 1.0f, 99.0f) * 0.01f;
	float HighPercent = FMath::Clamp(Settings.AutoExposureHighPercent, 1.0f, 99.0f) * 0.01f;

	float MinAverageLuminance = 1;
	float MaxAverageLuminance = 1;

	// This scales the average luminance after it gets clamped, affecting the exposure value directly.
	float LocalExposureMultipler = View.Family->ExposureSettings.bFixed ? 1.f : FMath::Pow(2.0f, Settings.AutoExposureBias);

	// This is used in the basic mode as a premultiplier before clamping between [MinLuminance, MaxLuminance]
	// Because this happens before clamping, the calibration constant has no impact on fixed EV100 modes.
	const float CalibrationConstant = FMath::Clamp<float>(Settings.AutoExposureCalibrationConstant, 1.f, 100.f) * 0.01f;
	float AverageLuminanceScale = Settings.AutoExposureMethod == EAutoExposureMethod::AEM_Basic ? (1.f / CalibrationConstant) : 1.f;

	// example min/max: -8 .. 4   means a range from 1/256 to 4  pow(2,-8) .. pow(2,4)
	float HistogramLogMin = Settings.HistogramLogMin;
	float HistogramLogMax = Settings.HistogramLogMax;
	HistogramLogMin = FMath::Min<float>(HistogramLogMin, HistogramLogMax - 1);

	// Eye adaptation is disabled except for highend right now because the histogram is not computed.
	if (EngineShowFlags.Lighting && EngineShowFlags.EyeAdaptation && View.GetFeatureLevel() >= MinFeatureLevel)
	{
		if (View.Family->ExposureSettings.bFixed || Settings.AutoExposureMethod == EAutoExposureMethod::AEM_Manual)
		{
			float FixedEV100 = View.Family->ExposureSettings.bFixed ? 
									View.Family->ExposureSettings.FixedEV100 : 
									FMath::Log2(FMath::Square(Settings.DepthOfFieldFstop) * Settings.CameraShutterSpeed * 100 / FMath::Max(1.f, Settings.CameraISO));

			MinAverageLuminance = MaxAverageLuminance = 1.2f * FMath::Pow(2.0f, FixedEV100);
		}
		else
		{
			MinAverageLuminance = Settings.AutoExposureMinBrightness;
			MaxAverageLuminance = Settings.AutoExposureMaxBrightness;
		}
	}

	// ----------

	LowPercent = FMath::Min<float>(LowPercent, HighPercent);
	MinAverageLuminance = FMath::Min<float>(MinAverageLuminance, MaxAverageLuminance);

	Out[0] = FVector4(LowPercent, HighPercent, MinAverageLuminance, MaxAverageLuminance);

	// ----------

	Out[1] = FVector4(LocalExposureMultipler, View.Family->DeltaWorldTime, Settings.AutoExposureSpeedUp, Settings.AutoExposureSpeedDown);

	// ----------

	float DeltaLog = HistogramLogMax - HistogramLogMin;
	float Multiply = 1.0f / DeltaLog;
	float Add = -HistogramLogMin * Multiply;
	float MinIntensity = FMath::Exp2(HistogramLogMin);
	Out[2] = FVector4(Multiply, Add, MinIntensity, 0);

	// ----------

	Out[3] = FVector4(0, AverageLuminanceScale, 0, 0);
}

// Basic AutoExposure requires at least ES3_1
static ERHIFeatureLevel::Type BasicEyeAdaptationMinFeatureLevel = ERHIFeatureLevel::ES3_1;

/** Encapsulates the histogram-based post processing eye adaptation pixel shader. */
class FPostProcessEyeAdaptationPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessEyeAdaptationPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment( Parameters, OutEnvironment );
		OutEnvironment.SetRenderTargetOutputFormat(0, PF_A32B32G32R32F);
		OutEnvironment.SetDefine(TEXT("EYE_ADAPTATION_PARAMS_SIZE"), (uint32)EYE_ADAPTATION_PARAMS_SIZE);
	}

	/** Default constructor. */
	FPostProcessEyeAdaptationPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FShaderParameter EyeAdaptationParams;
	FShaderResourceParameter EyeAdaptationTexture;

	/** Initialization constructor. */
	FPostProcessEyeAdaptationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		EyeAdaptationParams.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationParams"));
		EyeAdaptationTexture.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationTexture"));
	}

	template <typename TRHICmdList>
	void SetPS(const FRenderingCompositePassContext& Context, TRHICmdList& RHICmdList, IPooledRenderTarget* LastEyeAdaptation)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());

		if (Context.View.HasValidEyeAdaptation())
		{
			SetTextureParameter(Context.RHICmdList, ShaderRHI, EyeAdaptationTexture, LastEyeAdaptation->GetRenderTargetItem().TargetableTexture);
		}
		else // some views don't have a state, thumbnail rendering?
		{
			SetTextureParameter(Context.RHICmdList, ShaderRHI, EyeAdaptationTexture, GWhiteTexture->TextureRHI);
		}

		{
			FVector4 Temp[EYE_ADAPTATION_PARAMS_SIZE];
			FRCPassPostProcessEyeAdaptation::ComputeEyeAdaptationParamsValue(Context.View, Temp);
			SetShaderValueArray(RHICmdList, ShaderRHI, EyeAdaptationParams, Temp, EYE_ADAPTATION_PARAMS_SIZE);
		}
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << EyeAdaptationParams;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessEyeAdaptationPS,TEXT("/Engine/Private/PostProcessEyeAdaptation.usf"),TEXT("MainPS"),SF_Pixel);

/** Encapsulates the histogram-based post processing eye adaptation compute shader. */
class FPostProcessEyeAdaptationCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessEyeAdaptationCS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetRenderTargetOutputFormat(0, PF_A32B32G32R32F);
		OutEnvironment.SetDefine(TEXT("EYE_ADAPTATION_PARAMS_SIZE"), (uint32)EYE_ADAPTATION_PARAMS_SIZE);
	}

	/** Default constructor. */
	FPostProcessEyeAdaptationCS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FRWShaderParameter OutComputeTex;
	FShaderParameter EyeAdaptationParams;

	/** Initialization constructor. */
	FPostProcessEyeAdaptationCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		OutComputeTex.Bind(Initializer.ParameterMap, TEXT("OutComputeTex"));
		EyeAdaptationParams.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationParams"));
	}

	template <typename TRHICmdList>
	void SetParameters(TRHICmdList& RHICmdList, const FRenderingCompositePassContext& Context, FUnorderedAccessViewRHIParamRef DestUAV, IPooledRenderTarget* LastEyeAdaptation)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		// CS params
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);
		PostprocessParameter.SetCS(ShaderRHI, Context, RHICmdList, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		OutComputeTex.SetTexture(RHICmdList, ShaderRHI, nullptr, DestUAV);		
		
		// PS params
		FVector4 EyeAdaptationParamValues[EYE_ADAPTATION_PARAMS_SIZE];
		FRCPassPostProcessEyeAdaptation::ComputeEyeAdaptationParamsValue(Context.View, EyeAdaptationParamValues);
		SetShaderValueArray(RHICmdList, ShaderRHI, EyeAdaptationParams, EyeAdaptationParamValues, EYE_ADAPTATION_PARAMS_SIZE);
	}

	template <typename TRHICmdList>
	void UnsetParameters(TRHICmdList& RHICmdList)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		OutComputeTex.UnsetUAV(RHICmdList, ShaderRHI);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << OutComputeTex << EyeAdaptationParams;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessEyeAdaptationCS,TEXT("/Engine/Private/PostProcessEyeAdaptation.usf"),TEXT("MainCS"),SF_Compute);

void FRCPassPostProcessEyeAdaptation::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENTF(Context.RHICmdList, PostProcessEyeAdaptation, TEXT("PostProcessEyeAdaptation%s"), bIsComputePass?TEXT("Compute"):TEXT(""));
	AsyncEndFence = FComputeFenceRHIRef();

	const FViewInfo& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	// Get the custom 1x1 target used to store exposure value and Toggle the two render targets used to store new and old.
	Context.View.SwapEyeAdaptationRTs(Context.RHICmdList);

	IPooledRenderTarget* LastEyeAdaptation = Context.View.GetLastEyeAdaptationRT(Context.RHICmdList);
	IPooledRenderTarget* EyeAdaptation = Context.View.GetEyeAdaptation(Context.RHICmdList);
	check(EyeAdaptation);

	FIntPoint DestSize = EyeAdaptation->GetDesc().Extent;

	const FSceneRenderTargetItem& DestRenderTarget = EyeAdaptation->GetRenderTargetItem();

	static auto* RenderPassCVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.RHIRenderPasses"));
	if (bIsComputePass)
	{
		FIntRect DestRect(0, 0, DestSize.X, DestSize.Y);

		// Common setup
		SetRenderTarget(Context.RHICmdList, nullptr, nullptr);
		Context.SetViewportAndCallRHI(DestRect, 0.0f, 1.0f);
		
		static FName AsyncEndFenceName(TEXT("AsyncEyeAdaptationEndFence"));
		AsyncEndFence = Context.RHICmdList.CreateComputeFence(AsyncEndFenceName);

		if (IsAsyncComputePass())
		{
			// Async path
			FRHIAsyncComputeCommandListImmediate& RHICmdListComputeImmediate = FRHICommandListExecutor::GetImmediateAsyncComputeCommandList();
			{
				SCOPED_COMPUTE_EVENT(RHICmdListComputeImmediate, AsyncEyeAdaptation);
				WaitForInputPassComputeFences(RHICmdListComputeImmediate);
				RHICmdListComputeImmediate.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget.UAV);
				DispatchCS(RHICmdListComputeImmediate, Context, DestRenderTarget.UAV, LastEyeAdaptation);
				RHICmdListComputeImmediate.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, DestRenderTarget.UAV, AsyncEndFence);
			}
			FRHIAsyncComputeCommandListImmediate::ImmediateDispatch(RHICmdListComputeImmediate);
		}
		else
		{
			// Direct path
			WaitForInputPassComputeFences(Context.RHICmdList);
			Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);

			Context.RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget.UAV);
			DispatchCS(Context.RHICmdList, Context, DestRenderTarget.UAV, LastEyeAdaptation);
			Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, DestRenderTarget.UAV, AsyncEndFence);

			Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);
		}
	}
	else
	{
		// Inform MultiGPU systems that we're starting to update this texture for this frame
		Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);

		// we render to our own output render target, not the intermediate one created by the compositing system
		// Set the view family's render target/viewport.
		SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef(), true);
		Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

		TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
		TShaderMapRef<FPostProcessEyeAdaptationPS> PixelShader(Context.GetShaderMap());

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

		PixelShader->SetPS(Context, Context.RHICmdList, LastEyeAdaptation);

		// Draw a quad mapping scene color to the view's render target
		DrawRectangle(
			Context.RHICmdList,
			0, 0,
			DestSize.X, DestSize.Y,
			0, 0,
			DestSize.X, DestSize.Y,
			DestSize,
			DestSize,
			*VertexShader,
			EDRF_UseTriangleOptimization);

		Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());

		// Inform MultiGPU systems that we've finished updating this texture for this frame
		Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);
	}

	Context.View.SetValidEyeAdaptation();
}

template <typename TRHICmdList>
void FRCPassPostProcessEyeAdaptation::DispatchCS(TRHICmdList& RHICmdList, FRenderingCompositePassContext& Context, FUnorderedAccessViewRHIParamRef DestUAV, IPooledRenderTarget* LastEyeAdaptation)
{
	auto ShaderMap = Context.GetShaderMap();
	TShaderMapRef<FPostProcessEyeAdaptationCS> ComputeShader(ShaderMap);
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	ComputeShader->SetParameters(RHICmdList, Context, DestUAV, LastEyeAdaptation);
	DispatchComputeShader(RHICmdList, *ComputeShader, 1, 1, 1);
	ComputeShader->UnsetParameters(RHICmdList);
}

void FRCPassPostProcessEyeAdaptation::ComputeEyeAdaptationParamsValue(const FViewInfo& View, FVector4 Out[EYE_ADAPTATION_PARAMS_SIZE])
{
	ComputeEyeAdaptationValues(ERHIFeatureLevel::SM5, View, Out);
}

float FRCPassPostProcessEyeAdaptation::GetFixedExposure(const FViewInfo& View)
{
	FVector4 EyeAdaptationParams[EYE_ADAPTATION_PARAMS_SIZE];
	FRCPassPostProcessEyeAdaptation::ComputeEyeAdaptationParamsValue(View, EyeAdaptationParams);
	
	// like in PostProcessEyeAdaptation.usf (EyeAdaptationParams[0].ZW : Min/Max Intensity)
	const float Exposure = (EyeAdaptationParams[0].Z + EyeAdaptationParams[0].W) * 0.5f;
	const float ExposureOffsetMultipler = EyeAdaptationParams[1].X;

	const float ExposureScale = 1.0f / FMath::Max(0.0001f, Exposure);
	return ExposureScale * ExposureOffsetMultipler;
}

void FSceneViewState::UpdatePreExposure(FViewInfo& View)
{
	PreExposure = 1.f;
	bUpdateLastExposure = false;

	if (IsMobilePlatform(View.GetShaderPlatform()))
	{
		if (!IsMobileHDR())
		{
			// In gamma space, the exposure is fully applied in the pre-exposure (no post-exposure compensation)
			PreExposure = FRCPassPostProcessEyeAdaptation::GetFixedExposure(View);
		}
	}
	else if (UsePreExposure(View.GetShaderPlatform()))
	{
		const FSceneViewFamily& ViewFamily = *View.Family;

		if (!IsRichView(ViewFamily) && !ViewFamily.EngineShowFlags.VisualizeHDR && !ViewFamily.EngineShowFlags.VisualizeBloom && ViewFamily.EngineShowFlags.PostProcessing && ViewFamily.bResolveScene)
		{
			const float PreExposureOverride = CVarEyeAdaptationPreExposureOverride.GetValueOnRenderThread();
			const float LastExposure = View.GetLastEyeAdaptationExposure();
			if (PreExposureOverride > 0)
			{
				PreExposure = PreExposureOverride;
			}
			else if (LastExposure > 0)
			{
				PreExposure = LastExposure;
			}

			bUpdateLastExposure = true;
		}
	}

	// Set up view's preexposure.
	View.PreExposure = PreExposure > 0 ? PreExposure : 1.0f;
}

FPooledRenderTargetDesc FRCPassPostProcessEyeAdaptation::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	// Specify invalid description to avoid getting intermediate rendertargets created.
	// We want to use ViewState->GetEyeAdaptation() instead
	FPooledRenderTargetDesc Ret;

	Ret.DebugName = TEXT("EyeAdaptation");

	return Ret;
}


/** Encapsulates the post process computation of Log2 Luminance pixel shader. */
class FPostProcessBasicEyeAdaptationSetupPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessBasicEyeAdaptationSetupPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, BasicEyeAdaptationMinFeatureLevel);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("EYE_ADAPTATION_PARAMS_SIZE"), (uint32)EYE_ADAPTATION_PARAMS_SIZE);
	}

	/** Default constructor. */
	FPostProcessBasicEyeAdaptationSetupPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FShaderParameter EyeAdaptationParams;

	/** Initialization constructor. */
	FPostProcessBasicEyeAdaptationSetupPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		EyeAdaptationParams.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationParams"));
	}

	void SetPS(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);
		PostprocessParameter.SetPS(Context.RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

		{
			FVector4 Temp[EYE_ADAPTATION_PARAMS_SIZE];
			ComputeEyeAdaptationValues(BasicEyeAdaptationMinFeatureLevel, Context.View, Temp);
			SetShaderValueArray(Context.RHICmdList, ShaderRHI, EyeAdaptationParams, Temp, EYE_ADAPTATION_PARAMS_SIZE);
		}
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << EyeAdaptationParams;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(, FPostProcessBasicEyeAdaptationSetupPS, TEXT("/Engine/Private/PostProcessEyeAdaptation.usf"), TEXT("MainBasicEyeAdaptationSetupPS"), SF_Pixel);

void FRCPassPostProcessBasicEyeAdaptationSetUp::Process(FRenderingCompositePassContext& Context)
{
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if (!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FViewInfo& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = Context.ReferenceBufferSize.X / SrcSize.X;

	FIntRect SrcRect = Context.SceneColorViewRect / ScaleFactor;
	FIntRect DestRect = SrcRect;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, PostProcessBasicEyeAdaptationSetup, TEXT("PostProcessBasicEyeAdaptationSetup %dx%d"), DestRect.Width(), DestRect.Height());

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	ERenderTargetLoadAction LoadAction = Context.GetLoadActionForRenderTarget(DestRenderTarget);

	FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, LoadAction);
	FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
	Context.RHICmdList.SetRenderTargetsAndClear(Info);
	Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
	TShaderMapRef<FPostProcessBasicEyeAdaptationSetupPS> PixelShader(Context.GetShaderMap());

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

	PixelShader->SetPS(Context);

	DrawPostProcessPass(
		Context.RHICmdList,
		DestRect.Min.X, DestRect.Min.Y,
		DestRect.Width(), DestRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y,
		SrcRect.Width(), SrcRect.Height(),
		DestSize,
		SrcSize,
		*VertexShader,
		View.StereoPass,
		Context.HasHmdMesh(),
		EDRF_UseTriangleOptimization);

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessBasicEyeAdaptationSetUp::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.Reset();
	Ret.DebugName = TEXT("EyeAdaptationBasicSetup");
	// Require alpha channel for log2 information.
	Ret.Format = PF_FloatRGBA;
	Ret.Flags |= GFastVRamConfig.EyeAdaptation;
	return Ret;
}



/** Encapsulates the post process computation of the exposure scale  pixel shader. */
class FPostProcessLogLuminance2ExposureScalePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessLogLuminance2ExposureScalePS, Global);



public:
	/** Default constructor. */
	FPostProcessLogLuminance2ExposureScalePS() {}

	/** Initialization constructor. */
	FPostProcessLogLuminance2ExposureScalePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		EyeAdaptationTexture.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationTexture"));
		EyeAdaptationParams.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationParams"));
		EyeAdaptationSrcRect.Bind(Initializer.ParameterMap, TEXT("EyeAdaptionSrcRect"));
	}

public:

	/** Static Shader boilerplate */
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, BasicEyeAdaptationMinFeatureLevel);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetRenderTargetOutputFormat(0, PF_A32B32G32R32F);
		OutEnvironment.SetDefine(TEXT("EYE_ADAPTATION_PARAMS_SIZE"), (uint32)EYE_ADAPTATION_PARAMS_SIZE);
	}


	void SetPS(const FRenderingCompositePassContext& Context, const FIntRect& SrcRect, IPooledRenderTarget* LastEyeAdaptation)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		PostprocessParameter.SetPS(Context.RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

		// Associate the eye adaptation buffer from the previous frame with a texture to be read in this frame. 
		
		if (Context.View.HasValidEyeAdaptation())
		{
			SetTextureParameter(Context.RHICmdList, ShaderRHI, EyeAdaptationTexture, LastEyeAdaptation->GetRenderTargetItem().TargetableTexture);
		}
		else // some views don't have a state, thumbnail rendering?
		{
			SetTextureParameter(Context.RHICmdList, ShaderRHI, EyeAdaptationTexture, GWhiteTexture->TextureRHI);
		}

		// Pack the eye adaptation parameters for the shader
		{
			FVector4 Temp[EYE_ADAPTATION_PARAMS_SIZE];
			// static computation function
			ComputeEyeAdaptationValues(BasicEyeAdaptationMinFeatureLevel, Context.View, Temp);
			// Log-based computation of the exposure scale has a built in scaling.
			//Temp[1].X *= 0.16;  
			//Encode the eye-focus slope
			// Get the focus value for the eye-focus weighting
			Temp[2].W = GetBasicAutoExposureFocus();
			SetShaderValueArray(Context.RHICmdList, ShaderRHI, EyeAdaptationParams, Temp, EYE_ADAPTATION_PARAMS_SIZE);
		}

		// Set the src extent for the shader
		SetShaderValue(Context.RHICmdList, ShaderRHI, EyeAdaptationSrcRect, SrcRect);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << EyeAdaptationTexture 
			<< EyeAdaptationParams << EyeAdaptationSrcRect;
		return bShaderHasOutdatedParameters;
	}

private:
	FPostProcessPassParameters PostprocessParameter;
	FShaderResourceParameter EyeAdaptationTexture;
	FShaderParameter EyeAdaptationParams;
	FShaderParameter EyeAdaptationSrcRect;
};

IMPLEMENT_SHADER_TYPE(, FPostProcessLogLuminance2ExposureScalePS, TEXT("/Engine/Private/PostProcessEyeAdaptation.usf"), TEXT("MainLogLuminance2ExposureScalePS"), SF_Pixel);

void FRCPassPostProcessBasicEyeAdaptation::Process(FRenderingCompositePassContext& Context)
{
	const FViewInfo& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	// Get the custom 1x1 target used to store exposure value and Toggle the two render targets used to store new and old.
	Context.View.SwapEyeAdaptationRTs(Context.RHICmdList);
	IPooledRenderTarget* EyeAdaptationThisFrameRT = Context.View.GetEyeAdaptationRT(Context.RHICmdList);
	IPooledRenderTarget* EyeAdaptationLastFrameRT = Context.View.GetLastEyeAdaptationRT(Context.RHICmdList);

	check(EyeAdaptationThisFrameRT && EyeAdaptationLastFrameRT);

	FIntPoint DestSize = EyeAdaptationThisFrameRT->GetDesc().Extent;

	// The input texture sample size.  Averaged in the pixel shader.
	FIntPoint SrcSize = GetInputDesc(ePId_Input0)->Extent;

	// Compute the region of interest in the source texture.
	uint32 ScaleFactor = FMath::DivideAndRoundUp(FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY().Y, SrcSize.Y);

	FIntRect SrcRect = View.ViewRect / ScaleFactor;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, PostProcessBasicEyeAdaptation, TEXT("PostProcessBasicEyeAdaptation %dx%d"), SrcSize.X, SrcSize.Y);

	const FSceneRenderTargetItem& DestRenderTarget = EyeAdaptationThisFrameRT->GetRenderTargetItem();

	// Inform MultiGPU systems that we're starting to update this texture for this frame
	Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);

	// we render to our own output render target, not the intermediate one created by the compositing system
	// Set the view family's render target/viewport.
	SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef(), true);
	Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
	TShaderMapRef<FPostProcessLogLuminance2ExposureScalePS> PixelShader(Context.GetShaderMap());

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

	// Set the parameters used by the pixel shader.

	PixelShader->SetPS(Context, SrcRect, EyeAdaptationLastFrameRT);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		Context.RHICmdList,
		0, 0,
		DestSize.X, DestSize.Y,
		0, 0,
		DestSize.X, DestSize.Y,
		DestSize,
		DestSize,
		*VertexShader,
		EDRF_UseTriangleOptimization);

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());

	// Inform MultiGPU systems that we've finished with this texture for this frame
	Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);

	Context.View.SetValidEyeAdaptation();
}


FPooledRenderTargetDesc FRCPassPostProcessBasicEyeAdaptation::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	// Specify invalid description to avoid getting intermediate rendertargets created.
	// We want to use ViewState->GetEyeAdaptation() instead
	FPooledRenderTargetDesc Ret;

	Ret.DebugName = TEXT("EyeAdaptationBasic");
	Ret.Flags |= GFastVRamConfig.EyeAdaptation;
	return Ret;
}

void FSceneViewState::FEyeAdaptationRTManager::SafeRelease()
{
	PooledRenderTarget[0].SafeRelease();
	PooledRenderTarget[1].SafeRelease();

	for (int32 i = 0; i < NUM_STAGING_BUFFERS; ++i)
	{
		StagingBuffers[i].SafeRelease();
	}
}

void FSceneViewState::FEyeAdaptationRTManager::SwapRTs(bool bInUpdateLastExposure)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FEyeAdaptationRTManager_SwapRTs);

	FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	if (bInUpdateLastExposure && PooledRenderTarget[CurrentBuffer].IsValid())
	{
		if (!StagingBuffers[CurrentStagingBuffer].IsValid())
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(1, 1), PF_G32R32F, FClearValueBinding::None, TexCreate_CPUReadback | TexCreate_HideInVisualizeTexture, TexCreate_None, false));
			GRenderTargetPool.FindFreeElement(RHICmdList, Desc, StagingBuffers[CurrentStagingBuffer], TEXT("EyeAdaptationCPUReadBack"), true, ERenderTargetTransience::NonTransient);
		}

		// Transfer memory GPU -> CPU
		RHICmdList.CopyToResolveTarget(PooledRenderTarget[CurrentBuffer]->GetRenderTargetItem().TargetableTexture, StagingBuffers[CurrentStagingBuffer]->GetRenderTargetItem().ShaderResourceTexture, false, FResolveParams());

		// Take the oldest buffer and read it
		const int32 NextStagingBuffer = (CurrentStagingBuffer + 1) % NUM_STAGING_BUFFERS;
		if (StagingBuffers[NextStagingBuffer].IsValid())
		{
			const float* ResultsBuffer = nullptr;
			int32 BufferWidth = 0, BufferHeight = 0;
			RHICmdList.MapStagingSurface(StagingBuffers[NextStagingBuffer]->GetRenderTargetItem().ShaderResourceTexture, *(void**)&ResultsBuffer, BufferWidth, BufferHeight);

			if (ResultsBuffer)
			{
				LastExposure = *ResultsBuffer;
			}
			RHICmdList.UnmapStagingSurface(StagingBuffers[NextStagingBuffer]->GetRenderTargetItem().ShaderResourceTexture);
		}

		// Update indices
		CurrentStagingBuffer = NextStagingBuffer;
	}

	CurrentBuffer = 1 - CurrentBuffer;
}

TRefCountPtr<IPooledRenderTarget>&  FSceneViewState::FEyeAdaptationRTManager::GetRTRef(FRHICommandList& RHICmdList, const int BufferNumber)
{
	check(BufferNumber == 0 || BufferNumber == 1);

	// Create textures if needed.
	if (!PooledRenderTarget[BufferNumber].IsValid())
	{
		// Create the texture needed for EyeAdaptation
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(1, 1), PF_G32R32F, FClearValueBinding::None, TexCreate_None, TexCreate_RenderTargetable, false));
		if (GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM5)
		{
			Desc.TargetableFlags |= TexCreate_UAV;
		}
		GRenderTargetPool.FindFreeElement(RHICmdList, Desc, PooledRenderTarget[BufferNumber], TEXT("EyeAdaptation"), true, ERenderTargetTransience::NonTransient);
	}

	return PooledRenderTarget[BufferNumber];
}
