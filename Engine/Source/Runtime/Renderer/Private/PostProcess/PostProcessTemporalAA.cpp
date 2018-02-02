// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessTemporalAA.cpp: Post process MotionBlur implementation.
=============================================================================*/

#include "PostProcess/PostProcessTemporalAA.h"
#include "StaticBoundShaderState.h"
#include "SceneUtils.h"
#include "PostProcess/SceneRenderTargets.h"
#include "SceneRenderTargetParameters.h"
#include "SceneRendering.h"
#include "ScenePrivate.h"
#include "PostProcess/SceneFilterRendering.h"
#include "CompositionLighting/PostProcessAmbientOcclusion.h"
#include "PostProcess/PostProcessTonemap.h"
#include "ClearQuad.h"
#include "PipelineStateCache.h"
#include "PostProcessing.h"

const int32 GTemporalAATileSizeX = 8;
const int32 GTemporalAATileSizeY = 8;

static TAutoConsoleVariable<float> CVarTemporalAAFilterSize(
	TEXT("r.TemporalAAFilterSize"),
	1.0f,
	TEXT("Size of the filter kernel. (1.0 = smoother, 0.0 = sharper but aliased)."),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarTemporalAACatmullRom(
	TEXT("r.TemporalAACatmullRom"),
	0,
	TEXT("Whether to use a Catmull-Rom filter kernel. Should be a bit sharper than Gaussian."),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarTemporalAAPauseCorrect(
	TEXT("r.TemporalAAPauseCorrect"),
	1,
	TEXT("Correct temporal AA in pause. This holds onto render targets longer preventing reuse and consumes more memory."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarTemporalAACurrentFrameWeight(
	TEXT("r.TemporalAACurrentFrameWeight"),
	.04f,
	TEXT("Weight of current frame's contribution to the history.  Low values cause blurriness and ghosting, high values fail to hide jittering."),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static float CatmullRom( float x )
{
	float ax = FMath::Abs(x);
	if( ax > 1.0f )
		return ( ( -0.5f * ax + 2.5f ) * ax - 4.0f ) *ax + 2.0f;
	else
		return ( 1.5f * ax - 2.5f ) * ax*ax + 1.0f;
}


struct FTemporalAAParameters
{
public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter SampleWeights;
	FShaderParameter PlusWeights;
	FShaderParameter DitherScale;
	FShaderParameter VelocityScaling;
	FShaderParameter CurrentFrameWeight;
	FShaderParameter ScreenPosAbsMax;
	FShaderParameter ScreenPosToHistoryBufferUV;
	FShaderParameter HistoryBufferUVMinMax;
	FShaderParameter MaxViewportUVAndSvPositionToViewportUV;
	FShaderParameter OneOverHistoryPreExposure;

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		PostprocessParameter.Bind(ParameterMap);
		DeferredParameters.Bind(ParameterMap);
		SampleWeights.Bind(ParameterMap, TEXT("SampleWeights"));
		PlusWeights.Bind(ParameterMap, TEXT("PlusWeights"));
		DitherScale.Bind(ParameterMap, TEXT("DitherScale"));
		VelocityScaling.Bind(ParameterMap, TEXT("VelocityScaling"));
		CurrentFrameWeight.Bind(ParameterMap, TEXT("CurrentFrameWeight"));
		ScreenPosAbsMax.Bind(ParameterMap, TEXT("ScreenPosAbsMax"));
		ScreenPosToHistoryBufferUV.Bind(ParameterMap, TEXT("ScreenPosToHistoryBufferUV"));
		HistoryBufferUVMinMax.Bind(ParameterMap, TEXT("HistoryBufferUVMinMax"));
		MaxViewportUVAndSvPositionToViewportUV.Bind(ParameterMap, TEXT("MaxViewportUVAndSvPositionToViewportUV"));
		OneOverHistoryPreExposure.Bind(ParameterMap, TEXT("OneOverHistoryPreExposure"));
	}

	void Serialize(FArchive& Ar)
	{
		Ar << PostprocessParameter << DeferredParameters ;
		Ar << SampleWeights << PlusWeights << DitherScale << VelocityScaling << CurrentFrameWeight << ScreenPosAbsMax << ScreenPosToHistoryBufferUV << HistoryBufferUVMinMax << MaxViewportUVAndSvPositionToViewportUV << OneOverHistoryPreExposure;
	}

	template <typename TRHICmdList, typename TShaderRHIParamRef>
	void SetParameters(
		TRHICmdList& RHICmdList,
		const TShaderRHIParamRef ShaderRHI,
		const FRenderingCompositePassContext& Context,
		const FTemporalAAHistory& InputHistory,
		bool bUseDither,
		int32 ScaleFactor)
	{
		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View, MD_PostProcess);

		// PS params
		{
			const float JitterX = Context.View.TemporalJitterPixels.X;
			const float JitterY = Context.View.TemporalJitterPixels.Y;

			static const float SampleOffsets[9][2] =
			{
				{ -1.0f, -1.0f },
				{  0.0f, -1.0f },
				{  1.0f, -1.0f },
				{ -1.0f,  0.0f },
				{  0.0f,  0.0f },
				{  1.0f,  0.0f },
				{ -1.0f,  1.0f },
				{  0.0f,  1.0f },
				{  1.0f,  1.0f },
			};

			float FilterSize = CVarTemporalAAFilterSize.GetValueOnRenderThread();
			int32 bCatmullRom = CVarTemporalAACatmullRom.GetValueOnRenderThread();

			float Weights[9];
			float WeightsPlus[5];
			float TotalWeight = 0.0f;
			float TotalWeightLow = 0.0f;
			float TotalWeightPlus = 0.0f;
			for (int32 i = 0; i < 9; i++)
			{
				float PixelOffsetX = SampleOffsets[i][0] - JitterX;
				float PixelOffsetY = SampleOffsets[i][1] - JitterY;

				PixelOffsetX /= FilterSize;
				PixelOffsetY /= FilterSize;

				if (bCatmullRom)
				{
					Weights[i] = CatmullRom(PixelOffsetX) * CatmullRom(PixelOffsetY);
					TotalWeight += Weights[i];
				}
				else
				{
					// Normal distribution, Sigma = 0.47
					Weights[i] = FMath::Exp(-2.29f * (PixelOffsetX * PixelOffsetX + PixelOffsetY * PixelOffsetY));
					TotalWeight += Weights[i];
				}
			}

			WeightsPlus[0] = Weights[1];
			WeightsPlus[1] = Weights[3];
			WeightsPlus[2] = Weights[4];
			WeightsPlus[3] = Weights[5];
			WeightsPlus[4] = Weights[7];
			TotalWeightPlus = Weights[1] + Weights[3] + Weights[4] + Weights[5] + Weights[7];

			for (int32 i = 0; i < 9; i++)
			{
				SetShaderValue(RHICmdList, ShaderRHI, SampleWeights, Weights[i] / TotalWeight, i);
			}

			for (int32 i = 0; i < 5; i++)
			{
				SetShaderValue(RHICmdList, ShaderRHI, PlusWeights, WeightsPlus[i] / TotalWeightPlus, i);
			}
		}

		SetShaderValue(RHICmdList, ShaderRHI, DitherScale, bUseDither ? 1.0f : 0.0f);

		const bool bIgnoreVelocity = (Context.View.ViewState && Context.View.ViewState->bSequencerIsPaused);
		SetShaderValue(RHICmdList, ShaderRHI, VelocityScaling, bIgnoreVelocity ? 0.0f : 1.0f);

		SetShaderValue(RHICmdList, ShaderRHI, CurrentFrameWeight, CVarTemporalAACurrentFrameWeight.GetValueOnRenderThread());


		{
			FIntPoint ViewportOffset = InputHistory.ViewportRect.Min;
			FIntPoint ViewportExtent = InputHistory.ViewportRect.Size();
			FIntPoint ReferenceBufferSize = InputHistory.ReferenceBufferSize;

			float InBufferSizeX = 1.f / float(InputHistory.ReferenceBufferSize.X);
			float InBufferSizeY = 1.f / float(InputHistory.ReferenceBufferSize.Y);

			FVector4 ScreenPosToPixelValue(
				ViewportExtent.X * 0.5f * InBufferSizeX,
				-ViewportExtent.Y * 0.5f * InBufferSizeY,
				(ViewportExtent.X * 0.5f + ViewportOffset.X) * InBufferSizeX,
				(ViewportExtent.Y * 0.5f + ViewportOffset.Y) * InBufferSizeY);
			SetShaderValue(RHICmdList, ShaderRHI, ScreenPosToHistoryBufferUV, ScreenPosToPixelValue);

			FVector2D ScreenPosAbsMaxValue(1.0f - 1.0f / float(ViewportExtent.X), 1.0f - 1.0f / float(ViewportExtent.Y));
			SetShaderValue(RHICmdList, ShaderRHI, ScreenPosAbsMax, ScreenPosAbsMaxValue);

			FVector4 HistoryBufferUVMinMaxValue(
				(ViewportOffset.X + 0.5f) * InBufferSizeX,
				(ViewportOffset.Y + 0.5f) * InBufferSizeY,
				(ViewportOffset.X + ViewportExtent.X - 0.5f) * InBufferSizeX,
				(ViewportOffset.Y + ViewportExtent.Y - 0.5f) * InBufferSizeY);

			SetShaderValue(RHICmdList, ShaderRHI, HistoryBufferUVMinMax, HistoryBufferUVMinMaxValue);
		}

		{
			FVector4 MaxViewportUVAndSvPositionToViewportUVValue(
				(Context.View.ViewRect.Width() - 0.5f) / float(Context.View.ViewRect.Width()),
				(Context.View.ViewRect.Height() - 0.5f) / float(Context.View.ViewRect.Height()),
				float(ScaleFactor) / float(Context.View.ViewRect.Width()),
				float(ScaleFactor) / float(Context.View.ViewRect.Height()));

			SetShaderValue(RHICmdList, ShaderRHI, MaxViewportUVAndSvPositionToViewportUV, MaxViewportUVAndSvPositionToViewportUVValue);
		}

		const float OneOverHistoryPreExposureValue = 1.f / FMath::Max<float>(SMALL_NUMBER, InputHistory.IsValid() ? InputHistory.SceneColorPreExposure : Context.View.PreExposure);
		SetShaderValue(RHICmdList, ShaderRHI, OneOverHistoryPreExposure, OneOverHistoryPreExposureValue);
	}
};


/** Encapsulates a TemporalAA pixel shader. */
template< uint32 PassConfig, uint32 Responsive >
class FPostProcessTemporalAAPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessTemporalAAPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		// PassConfig == 4 is MainFast.
		// Responsive == 2 is camera cut.
		OutEnvironment.SetDefine(TEXT("TAA_PASS_CONFIG"), PassConfig == 4 ? 1 : PassConfig);
		OutEnvironment.SetDefine(TEXT("TAA_FAST"), PassConfig == 4 ? 1 : 0);
		OutEnvironment.SetDefine(TEXT("TAA_RESPONSIVE"), Responsive > 0 ? 1 : 0);
		OutEnvironment.SetDefine(TEXT("TAA_CAMERA_CUT"), Responsive == 2 ? 1 : 0);
	}

	/** Default constructor. */
	FPostProcessTemporalAAPS() {}

public:
	FTemporalAAParameters Parameter;

	/** Initialization constructor. */
	FPostProcessTemporalAAPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Parameter.Bind(Initializer.ParameterMap);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Parameter.Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	template <typename TRHICmdList>
	void SetParameters(
		TRHICmdList& RHICmdList,
		const FRenderingCompositePassContext& Context,
		const FTemporalAAHistory& InputHistory,
		bool bUseDither,
		int32 ScaleFactor)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);
		
		FSamplerStateRHIParamRef FilterTable[3];
		FilterTable[0] = TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		FilterTable[1] = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		FilterTable[2] = FilterTable[0];

		Parameter.PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, 0, eFC_0000, FilterTable);

		Parameter.SetParameters(RHICmdList, ShaderRHI, Context, InputHistory, bUseDither, ScaleFactor);
	}
};

// Typedef is necessary because the C preprocessor thinks the comma in the template parameter list is a comma in the macro parameter list.
#define IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(A, B) \
	typedef FPostProcessTemporalAAPS<A,B> FPostProcessTemporalAAPS##A##B; \
	IMPLEMENT_SHADER_TYPE(template<>,FPostProcessTemporalAAPS##A##B, \
		TEXT("/Engine/Private/PostProcessTemporalAA.usf"), TEXT("MainPS"),SF_Pixel);

IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(0, 0);
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(1, 0);
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(1, 1);
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(1, 2);
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(2, 0);
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(3, 0);
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(4, 0);
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(4, 1);
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(4, 2);

/** Encapsulates the post processing depth of field setup pixel shader. */
template<uint32 PassConfig, uint32 UseFast, uint32 ScreenPercentageRange, uint32 CameraCut>
class FPostProcessTemporalAACS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessTemporalAACS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("TAA_PASS_CONFIG"), PassConfig);
		OutEnvironment.SetDefine(TEXT("TAA_FAST"), UseFast);
		OutEnvironment.SetDefine(TEXT("TAA_SCREEN_PERCENTAGE_RANGE"), ScreenPercentageRange);
		OutEnvironment.SetDefine(TEXT("TAA_CAMERA_CUT"), CameraCut);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GTemporalAATileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GTemporalAATileSizeY);
	}

	/** Default constructor. */
	FPostProcessTemporalAACS() {}

public:
	FTemporalAAParameters Parameter;
	FShaderResourceParameter EyeAdaptation;
	FShaderParameter TemporalAAComputeParams;
	FShaderParameter OutComputeTex;
	FShaderParameter TemporalJitterPixels;
	FShaderParameter ScreenPercentageAndUpscaleFactor;

	/** Initialization constructor. */
	FPostProcessTemporalAACS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Parameter.Bind(Initializer.ParameterMap);

		EyeAdaptation.Bind(Initializer.ParameterMap, TEXT("EyeAdaptation"));
		TemporalAAComputeParams.Bind(Initializer.ParameterMap, TEXT("TemporalAAComputeParams"));
		OutComputeTex.Bind(Initializer.ParameterMap, TEXT("OutComputeTex"));
		TemporalJitterPixels.Bind(Initializer.ParameterMap, TEXT("TemporalJitterPixels"));
		ScreenPercentageAndUpscaleFactor.Bind(Initializer.ParameterMap, TEXT("ScreenPercentageAndUpscaleFactor"));
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Parameter.Serialize(Ar);
		Ar << EyeAdaptation << TemporalAAComputeParams << OutComputeTex << TemporalJitterPixels << ScreenPercentageAndUpscaleFactor;
		return bShaderHasOutdatedParameters;
	}

	template <typename TRHICmdList>
	void SetParameters(
		TRHICmdList& RHICmdList,
		const FRenderingCompositePassContext& Context,
		const FTemporalAAHistory& InputHistory,
		const FIntPoint& DestSize,
		FUnorderedAccessViewRHIParamRef DestUAV,
		bool bUseDither,
		FTextureRHIParamRef EyeAdaptationTex,
		int32 ScaleFactor)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		// CS params
		FSamplerStateRHIParamRef FilterTable[3];
		{
			FilterTable[0] = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			FilterTable[1] = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
			FilterTable[2] = FilterTable[0];
		}
		Parameter.PostprocessParameter.SetCS(ShaderRHI, Context, RHICmdList, 0, eFC_0000, FilterTable);

		RHICmdList.SetUAVParameter(ShaderRHI, OutComputeTex.GetBaseIndex(), DestUAV);

		FVector4 TemporalAAComputeValues(0, 0, 1.f / (float)DestSize.X, 1.f / (float)DestSize.Y);
		SetShaderValue(RHICmdList, ShaderRHI, TemporalAAComputeParams, TemporalAAComputeValues);

		// VS params		
		SetTextureParameter(RHICmdList, ShaderRHI, EyeAdaptation, EyeAdaptationTex);

		Parameter.SetParameters(RHICmdList, ShaderRHI, Context, InputHistory, bUseDither, ScaleFactor);

		// Temporal AA upscale specific params.
		{
			FIntPoint SecondaryViewSize = Context.View.GetSecondaryViewRectSize();
			SetShaderValue(RHICmdList, ShaderRHI, TemporalJitterPixels, Context.View.TemporalJitterPixels);
			SetShaderValue(RHICmdList, ShaderRHI, ScreenPercentageAndUpscaleFactor, FVector2D(
				float(Context.View.ViewRect.Size().X) / float(SecondaryViewSize.X),
				float(SecondaryViewSize.X) / float(Context.View.ViewRect.Size().X)));
		}
	}

	template <typename TRHICmdList>
	void UnsetParameters(TRHICmdList& RHICmdList)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		RHICmdList.SetUAVParameter(ShaderRHI, OutComputeTex.GetBaseIndex(), NULL);
	}
};

#define IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(A, B, C, D) \
	typedef FPostProcessTemporalAACS<A, B, C, D> FPostProcessTemporalAACS##A##B##C##D; \
	IMPLEMENT_SHADER_TYPE(template<>,FPostProcessTemporalAACS##A##B##C##D, \
		TEXT("/Engine/Private/PostProcessTemporalAA.usf"),TEXT("MainCS"),SF_Compute);

IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(0, 0, 0, 0);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(1, 0, 0, 0);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(1, 1, 0, 0);

IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(0, 0, 0, 1);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(1, 0, 0, 1);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(1, 1, 0, 1);

// Upsampling compute shaders.
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 0, 0, 0);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 1, 0, 0);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 0, 1, 0);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 1, 1, 0);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 0, 2, 0);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 1, 2, 0);

IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 0, 0, 1);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 1, 0, 1);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 0, 1, 1);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 1, 1, 1);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 0, 2, 1);
IMPLEMENT_TEMPORALAA_COMPUTESHADER_TYPE(4, 1, 2, 1);

template <typename TPixelShader>
void DrawPixelPassTemplate(
	FRenderingCompositePassContext& Context,
	const FIntPoint SrcSize,
	const FIntRect ViewRect,
	const FTemporalAAHistory& InputHistory,
	const bool bUseDither,
	int32 ScaleFactor,
	FDepthStencilStateRHIParamRef DepthStencilState)
{
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = DepthStencilState;

	TShaderMapRef< FPostProcessTonemapVS > VertexShader(Context.GetShaderMap());
	TShaderMapRef< TPixelShader > PixelShader(Context.GetShaderMap());

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

	VertexShader->SetVS(Context);
	PixelShader->SetParameters(Context.RHICmdList, Context, InputHistory, bUseDither, ScaleFactor);

	DrawRectangle(
		Context.RHICmdList,
		0, 0,
		ViewRect.Width(), ViewRect.Height(),
		ViewRect.Min.X, ViewRect.Min.Y,
		ViewRect.Width(), ViewRect.Height(),
		ViewRect.Size(),
		SrcSize,
		*VertexShader,
		EDRF_UseTriangleOptimization);
}

template <typename Type, typename TRHICmdList>
void DispatchCSTemplate(
	TRHICmdList& RHICmdList,
	FRenderingCompositePassContext& Context,
	const FTemporalAAHistory& InputHistory,
	const FIntRect& DestRect,
	FUnorderedAccessViewRHIParamRef DestUAV,
	const bool bUseDither,
	FTextureRHIParamRef EyeAdaptationTex,
	int32 ScaleFactor)
{
	auto ShaderMap = Context.GetShaderMap();
	TShaderMapRef<Type> ComputeShader(ShaderMap);

	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	FIntPoint DestSize(DestRect.Width(), DestRect.Height());
	ComputeShader->SetParameters(RHICmdList, Context, InputHistory, DestSize, DestUAV, bUseDither, EyeAdaptationTex, ScaleFactor);

	uint32 GroupSizeX = FMath::DivideAndRoundUp(DestSize.X, GTemporalAATileSizeX);
	uint32 GroupSizeY = FMath::DivideAndRoundUp(DestSize.Y, GTemporalAATileSizeY);
	DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

	ComputeShader->UnsetParameters(RHICmdList);
}

void FRCPassPostProcessSSRTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(Context.RHICmdList, SSRTemporalAA);

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);
	FIntPoint SrcSize = InputDesc->Extent;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);
	FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, ERenderTargetLoadAction::ENoAction);
	FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
	Context.RHICmdList.SetRenderTargetsAndClear(Info);
	Context.SetViewportAndCallRHI(Context.View.ViewRect);

	DrawPixelPassTemplate<FPostProcessTemporalAAPS<2, 0>>(
		Context, SrcSize, Context.View.ViewRect,
		InputHistory, /* bUseDither = */ false, /* ScaleFactor = */ 1,
		TStaticDepthStencilState<false, CF_Always>::GetRHI());

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());

	OutputHistory->SafeRelease();
	OutputHistory->RT[0] = PassOutputs[0].PooledRenderTarget;
	OutputHistory->ViewportRect = Context.View.ViewRect;
	OutputHistory->ReferenceBufferSize = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY();
	OutputHistory->SceneColorPreExposure = Context.View.PreExposure;
}

FPooledRenderTargetDesc FRCPassPostProcessSSRTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.DebugName = TEXT("SSRTemporalAA");
	Ret.AutoWritable = false;
	return Ret;
}

void FRCPassPostProcessDOFTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	AsyncEndFence = FComputeFenceRHIRef();

	const FViewInfo& View = Context.View;

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);
	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	int32 ScaleFactor = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY().X / SrcSize.X;
	FIntRect ViewRect;
	ViewRect.Min = View.ViewRect.Min / ScaleFactor;
	ensure(ViewRect.Min * ScaleFactor == View.ViewRect.Min);
	ViewRect.Max.X = FMath::DivideAndRoundUp(View.ViewRect.Max.X, ScaleFactor);
	ViewRect.Max.Y = FMath::DivideAndRoundUp(View.ViewRect.Max.Y, ScaleFactor);

	SCOPED_DRAW_EVENTF(Context.RHICmdList, DOFTemporalAA, TEXT("DOFTemporalAA%s %dx%d"), bIsComputePass ? TEXT("Compute") : TEXT(""), ViewRect.Width(), ViewRect.Height());

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	if (bIsComputePass)
	{
		// Common setup
		SetRenderTarget(Context.RHICmdList, nullptr, nullptr);
		Context.SetViewportAndCallRHI(ViewRect, 0.0f, 1.0f);
		
		static FName AsyncEndFenceName(TEXT("AsyncDOFTemporalAAEndFence"));
		AsyncEndFence = Context.RHICmdList.CreateComputeFence(AsyncEndFenceName);

		FTextureRHIRef EyeAdaptationTex = GWhiteTexture->TextureRHI;
		if (Context.View.HasValidEyeAdaptation())
		{
			EyeAdaptationTex = Context.View.GetEyeAdaptation(Context.RHICmdList)->GetRenderTargetItem().TargetableTexture;
		}

		if (IsAsyncComputePass())
		{
			// Async path
			FRHIAsyncComputeCommandListImmediate& RHICmdListComputeImmediate = FRHICommandListExecutor::GetImmediateAsyncComputeCommandList();
			{
 				SCOPED_COMPUTE_EVENT(RHICmdListComputeImmediate, AsyncDOFTemporalAA);
				WaitForInputPassComputeFences(RHICmdListComputeImmediate);
					
				RHICmdListComputeImmediate.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget.UAV);
				DispatchCS(RHICmdListComputeImmediate, Context, ViewRect, DestRenderTarget.UAV, EyeAdaptationTex, ScaleFactor);
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
			DispatchCS(Context.RHICmdList, Context, ViewRect, DestRenderTarget.UAV, EyeAdaptationTex, ScaleFactor);
			Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, DestRenderTarget.UAV, AsyncEndFence);

			Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);
		}
	}
	else
	{
		WaitForInputPassComputeFences(Context.RHICmdList);

		// Inform MultiGPU systems that we're starting to update the texture
		Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);

		FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, ERenderTargetLoadAction::ENoAction);
		FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
		Context.RHICmdList.SetRenderTargetsAndClear(Info);
		Context.SetViewportAndCallRHI(ViewRect);

		DrawPixelPassTemplate<FPostProcessTemporalAAPS<0, 0>>(
			Context, SrcSize, ViewRect,
			InputHistory, /* bUseDither = */ false, ScaleFactor,
			TStaticDepthStencilState<false, CF_Always>::GetRHI());

		Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());

		// Inform MultiGPU systems that we've finished with this texture for this frame
		Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);
	}

	OutputHistory->SafeRelease();
	OutputHistory->RT[0] = PassOutputs[0].PooledRenderTarget;
	OutputHistory->ReferenceBufferSize = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY();
	OutputHistory->ViewportRect = View.ViewRect;
	OutputHistory->SceneColorPreExposure = Context.View.PreExposure;
}

template <typename TRHICmdList>
void FRCPassPostProcessDOFTemporalAA::DispatchCS(TRHICmdList& RHICmdList, FRenderingCompositePassContext& Context, const FIntRect& DestRect, FUnorderedAccessViewRHIParamRef DestUAV, FTextureRHIParamRef EyeAdaptationTex, int32 ScaleFactor)
{
	DispatchCSTemplate<FPostProcessTemporalAACS<0, 0, 0, 0>>(RHICmdList, Context, InputHistory, DestRect, DestUAV, false, EyeAdaptationTex, ScaleFactor);	
}


FPooledRenderTargetDesc FRCPassPostProcessDOFTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
	Ret.AutoWritable = false;
	Ret.DebugName = TEXT("BokehDOFTemporalAA");
	Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
	Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;
	return Ret;
}


void FRCPassPostProcessDOFTemporalAANear::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(Context.RHICmdList, DOFTemporalAANear);

	const FViewInfo& View = Context.View;
	FSceneViewState* ViewState = Context.ViewState;

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);
	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	int32 ScaleFactor = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY().X / SrcSize.X;
	FIntRect ViewRect;
	ViewRect.Min = View.ViewRect.Min / ScaleFactor;
	ensure(ViewRect.Min * ScaleFactor == View.ViewRect.Min);
	ViewRect.Max.X = FMath::DivideAndRoundUp(View.ViewRect.Max.X, ScaleFactor);
	ViewRect.Max.Y = FMath::DivideAndRoundUp(View.ViewRect.Max.Y, ScaleFactor);

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Inform MultiGPU systems that we're starting to update this texture for this frame
	Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);

	FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, ERenderTargetLoadAction::ENoAction);
	FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
	Context.RHICmdList.SetRenderTargetsAndClear(Info);
	Context.SetViewportAndCallRHI(ViewRect);

	DrawPixelPassTemplate<FPostProcessTemporalAAPS<0, 0>>(
		Context, SrcSize, ViewRect,
		InputHistory, /* bUseDither = */ false, ScaleFactor,
		TStaticDepthStencilState<false, CF_Always>::GetRHI());

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());

	// Inform MultiGPU systems that we've finished with this texture for this frame
	Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);

	OutputHistory->SafeRelease();
	OutputHistory->RT[0] = PassOutputs[0].PooledRenderTarget;
	OutputHistory->ReferenceBufferSize = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY();
	OutputHistory->ViewportRect = View.ViewRect;
	OutputHistory->SceneColorPreExposure = Context.View.PreExposure;
}

FPooledRenderTargetDesc FRCPassPostProcessDOFTemporalAANear::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.DebugName = TEXT("BokehDOFTemporalAANear");

	return Ret;
}



void FRCPassPostProcessLightShaftTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	const FViewInfo& View = Context.View;
	FSceneViewState* ViewState = Context.ViewState;

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);
	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	int32 ScaleFactor = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY().X / SrcSize.X;
	FIntRect ViewRect = View.ViewRect / ScaleFactor;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, TemporalAA, TEXT("LightShaftTemporalAA %dx%d"), ViewRect.Width(), ViewRect.Height());

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);
	FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, ERenderTargetLoadAction::ENoAction);
	FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
	Context.RHICmdList.SetRenderTargetsAndClear(Info);
	Context.SetViewportAndCallRHI(ViewRect);

	DrawPixelPassTemplate<FPostProcessTemporalAAPS<3, 0>>(
		Context, SrcSize, ViewRect,
		InputHistory, /* bUseDither = */ false, ScaleFactor,
		TStaticDepthStencilState<false, CF_Always>::GetRHI());

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessLightShaftTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.DebugName = TEXT("LightShaftTemporalAA");

	return Ret;
}

FRCPassPostProcessTemporalAA::FRCPassPostProcessTemporalAA(
	const FPostprocessContext& Context,
	const FTemporalAAHistory& InInputHistory,
	FTemporalAAHistory* OutOutputHistory,
	bool bInIsComputePass)
	: OutputExtent(0, 0)
	, InputHistory(InInputHistory)
	, OutputHistory(OutOutputHistory)
{
	bIsComputePass = bInIsComputePass;
	bPreferAsyncCompute = false;
	bPreferAsyncCompute &= (GNumActiveGPUsForRendering == 1); // Can't handle multi-frame updates on async pipe

	if (Context.View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale)
	{
		FIntPoint PrimaryUpscaleViewSize = Context.View.GetSecondaryViewRectSize();
		FIntPoint QuantizedPrimaryUpscaleViewSize;
		QuantizeSceneBufferSize(PrimaryUpscaleViewSize, QuantizedPrimaryUpscaleViewSize);

		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);
		OutputExtent.X = FMath::Max(SceneContext.GetBufferSizeXY().X, QuantizedPrimaryUpscaleViewSize.X);
		OutputExtent.Y = FMath::Max(SceneContext.GetBufferSizeXY().Y, QuantizedPrimaryUpscaleViewSize.Y);
		bIsComputePass = true;
	}
}

DECLARE_GPU_STAT(TemporalAA); 

void FRCPassPostProcessTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	AsyncEndFence = FComputeFenceRHIRef();

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);
	FIntPoint SrcSize = InputDesc->Extent;
	
	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PostProcessAAQuality")); 
	uint32 Quality = FMath::Clamp(CVar->GetValueOnRenderThread(), 1, 6);
	bool bUseFast = Quality == 3;

	// Only use dithering if we are outputting to a low precision format
	const bool bUseDither = PassOutputs[0].RenderTargetDesc.Format != PF_FloatRGBA;

	FIntRect SrcRect = Context.View.ViewRect;
	FIntRect DestRect = SrcRect;

	SCOPED_GPU_STAT(Context.RHICmdList, TemporalAA);
	if (bIsComputePass)
	{
		const TCHAR* PassName = TEXT("AA");

		if (Context.View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale)
		{
			// Outputs the scene color in top left corner, because the output extent is
			// max(View.UnscaledViewRect, SceneContext.GetBufferSizeXY())
			DestRect.Min = FIntPoint(0, 0);
			DestRect.Max = Context.View.GetSecondaryViewRectSize();
			PassName = TEXT("Upsample");
		}

		SCOPED_DRAW_EVENTF(Context.RHICmdList, TemporalAA, TEXT("Temporal%sCompute%s %dx%d -> %dx%d"),
			PassName, bUseFast ? TEXT("Fast") : TEXT(""), SrcRect.Width(), SrcRect.Height(), DestRect.Width(), DestRect.Height());

		// Common setup
		SetRenderTarget(Context.RHICmdList, nullptr, nullptr);
		Context.SetViewportAndCallRHI(DestRect, 0.0f, 1.0f);
		
		static FName AsyncEndFenceName(TEXT("AsyncTemporalAAEndFence"));
		AsyncEndFence = Context.RHICmdList.CreateComputeFence(AsyncEndFenceName);

		FTextureRHIRef EyeAdaptationTex = GWhiteTexture->TextureRHI;
		if (Context.View.HasValidEyeAdaptation())
		{
			EyeAdaptationTex = Context.View.GetEyeAdaptation(Context.RHICmdList)->GetRenderTargetItem().TargetableTexture;
		}

		if (IsAsyncComputePass())
		{
			// Async path
			FRHIAsyncComputeCommandListImmediate& RHICmdListComputeImmediate = FRHICommandListExecutor::GetImmediateAsyncComputeCommandList();
			{
 				SCOPED_COMPUTE_EVENT(RHICmdListComputeImmediate, AsyncTemporalAA);
				WaitForInputPassComputeFences(RHICmdListComputeImmediate);
					
				RHICmdListComputeImmediate.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget.UAV);
				DispatchCS(RHICmdListComputeImmediate, Context, DestRect, DestRenderTarget.UAV, bUseFast, bUseDither, EyeAdaptationTex);
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
			DispatchCS(Context.RHICmdList, Context, DestRect, DestRenderTarget.UAV, bUseFast, bUseDither, EyeAdaptationTex);
			Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, DestRenderTarget.UAV, AsyncEndFence);

			Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);
		}
	}
	else
	{
		checkf(Context.View.PrimaryScreenPercentageMethod != EPrimaryScreenPercentageMethod::TemporalUpscale,
			TEXT("Temporal upsample is only available with TemporalAA's compute shader."));

		FIntRect ViewRect = DestRect;
		FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

		SCOPED_DRAW_EVENTF(Context.RHICmdList, TemporalAA, TEXT("TemporalAA%s %dx%d"), bUseFast ? TEXT("Fast") : TEXT(""), ViewRect.Width(), ViewRect.Height());

		WaitForInputPassComputeFences(Context.RHICmdList);

		// Inform MultiGPU systems that we're starting to update this resource
		Context.RHICmdList.BeginUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);

		SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, SceneContext.GetSceneDepthTexture(), ESimpleRenderTargetMode::EUninitializedColorExistingDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);
		Context.SetViewportAndCallRHI(ViewRect);

		if(!Context.View.PrevViewInfo.TemporalAAHistory.IsValid())
		{
			// On camera cut this turns on responsive everywhere.
			if (bUseFast)
			{
				DrawPixelPassTemplate<FPostProcessTemporalAAPS<4, 2>>(
					Context, SrcSize, ViewRect,
					InputHistory, bUseDither, /* ScaleFactor = */ 1,
					TStaticDepthStencilState<false, CF_Always>::GetRHI());
			}
			else
			{
				DrawPixelPassTemplate<FPostProcessTemporalAAPS<1, 2>>(
					Context, SrcSize, ViewRect,
					InputHistory, bUseDither, /* ScaleFactor = */ 1,
					TStaticDepthStencilState<false, CF_Always>::GetRHI());
			}
		}
		else
		{
			{	
				// Normal temporal feedback
				// Draw to pixels where stencil == 0
				FDepthStencilStateRHIParamRef DepthStencilState = TStaticDepthStencilState<
					false, CF_Always,
					true, CF_Equal, SO_Keep, SO_Keep, SO_Keep,
					false, CF_Always, SO_Keep, SO_Keep, SO_Keep,
					STENCIL_TEMPORAL_RESPONSIVE_AA_MASK, STENCIL_TEMPORAL_RESPONSIVE_AA_MASK
				>::GetRHI();
	
				if (bUseFast)
				{
					DrawPixelPassTemplate<FPostProcessTemporalAAPS<4, 0>>(
						Context, SrcSize, ViewRect,
						InputHistory, bUseDither, /* ScaleFactor = */ 1,
						DepthStencilState);
				}
				else
				{
					DrawPixelPassTemplate<FPostProcessTemporalAAPS<1, 0>>(
						Context, SrcSize, ViewRect,
						InputHistory, bUseDither, /* ScaleFactor = */ 1,
						DepthStencilState);
				}
			}
	
			{	// Responsive feedback for tagged pixels
				// Draw to pixels where stencil != 0
				FDepthStencilStateRHIParamRef DepthStencilState = TStaticDepthStencilState<
					false, CF_Always,
					true, CF_NotEqual, SO_Keep, SO_Keep, SO_Keep,
					false, CF_Always, SO_Keep, SO_Keep, SO_Keep,
					STENCIL_TEMPORAL_RESPONSIVE_AA_MASK, STENCIL_TEMPORAL_RESPONSIVE_AA_MASK
				>::GetRHI();
			
				if(bUseFast)
				{
					DrawPixelPassTemplate<FPostProcessTemporalAAPS<4, 1>>(
						Context, SrcSize, ViewRect,
						InputHistory, bUseDither, 1,
						DepthStencilState);
				}
				else
				{
					DrawPixelPassTemplate<FPostProcessTemporalAAPS<1, 1>>(
						Context, SrcSize, ViewRect,
						InputHistory, bUseDither, 1,
						DepthStencilState);
				}
			}
		}

		Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());

		// Inform MultiGPU systems that we've finished with this texture for this frame
		Context.RHICmdList.EndUpdateMultiFrameResource(DestRenderTarget.ShaderResourceTexture);
	}

	OutputHistory->SafeRelease();
	OutputHistory->RT[0] = PassOutputs[0].PooledRenderTarget;
	OutputHistory->ViewportRect = DestRect;
	OutputHistory->ReferenceBufferSize = SrcSize;
	OutputHistory->SceneColorPreExposure = Context.View.PreExposure;

	// Changes the view rectangle of the scene color and reference buffer size when doing temporal upsample for the
	// following passes to still work.
	if (Context.View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale)
	{
		Context.SceneColorViewRect = OutputHistory->ViewportRect;
		Context.ReferenceBufferSize = OutputExtent;

		OutputHistory->ReferenceBufferSize = OutputExtent;
	}
}

template <uint32 PassConfig, uint32 ScreenPercentageRange, typename TRHICmdList>
void DispatchMainCSTemplate(
	TRHICmdList& RHICmdList,
	FRenderingCompositePassContext& Context,
	const FTemporalAAHistory& InputHistory,
	const FIntRect& DestRect,
	FUnorderedAccessViewRHIParamRef DestUAV,
	const bool bUseFast,
	const bool bUseDither,
	FTextureRHIParamRef EyeAdaptationTex)
{
	if (!Context.View.PrevViewInfo.TemporalAAHistory.IsValid())
	{
		if (bUseFast)
		{
			DispatchCSTemplate<FPostProcessTemporalAACS<PassConfig, 1, ScreenPercentageRange, 1>>(RHICmdList, Context, InputHistory, DestRect, DestUAV, bUseDither, EyeAdaptationTex, 1);
		}
		else
		{
			DispatchCSTemplate<FPostProcessTemporalAACS<PassConfig, 0, ScreenPercentageRange, 1>>(RHICmdList, Context, InputHistory, DestRect, DestUAV, bUseDither, EyeAdaptationTex, 1);
		}
	}
	else
	{
		if (bUseFast)
		{
			DispatchCSTemplate<FPostProcessTemporalAACS<PassConfig, 1, ScreenPercentageRange, 0>>(RHICmdList, Context, InputHistory, DestRect, DestUAV, bUseDither, EyeAdaptationTex, 1);
		}
		else
		{
			DispatchCSTemplate<FPostProcessTemporalAACS<PassConfig, 0, ScreenPercentageRange, 0>>(RHICmdList, Context, InputHistory, DestRect, DestUAV, bUseDither, EyeAdaptationTex, 1);
		}
	}
}

template <typename TRHICmdList>
void FRCPassPostProcessTemporalAA::DispatchCS(TRHICmdList& RHICmdList, FRenderingCompositePassContext& Context, const FIntRect& DestRect, FUnorderedAccessViewRHIParamRef DestUAV, const bool bUseFast, const bool bUseDither, FTextureRHIParamRef EyeAdaptationTex)
{
	if (Context.View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale)
	{
		// If screen percentage > 100% on X or Y axes, then use screen percentage range = 2 shader permutation to disable LDS caching.
		if (Context.View.ViewRect.Width() > DestRect.Width() ||
			Context.View.ViewRect.Height() > DestRect.Height())
		{
			DispatchMainCSTemplate<4, 2>(RHICmdList, Context, InputHistory, DestRect, DestUAV, bUseFast, bUseDither, EyeAdaptationTex);
		}
		// If screen percentage < 71% on X and Y axes, then use screen percentage range = 1 shader permutation to have smaller LDS caching.
		else if (Context.View.ViewRect.Width() * 100 < 71 * DestRect.Width() &&
			Context.View.ViewRect.Height() * 100 < 71 * DestRect.Height())
		{
			DispatchMainCSTemplate<4, 1>(RHICmdList, Context, InputHistory, DestRect, DestUAV, bUseFast, bUseDither, EyeAdaptationTex);
		}
		else
		{
			DispatchMainCSTemplate<4, 0>(RHICmdList, Context, InputHistory, DestRect, DestUAV, bUseFast, bUseDither, EyeAdaptationTex);
		}
	}
	else
	{
		DispatchMainCSTemplate<1, 0>(RHICmdList, Context, InputHistory, DestRect, DestUAV, bUseFast, bUseDither, EyeAdaptationTex);
	}
}

FPooledRenderTargetDesc FRCPassPostProcessTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
	Ret.Flags &= ~(TexCreate_FastVRAM | TexCreate_Transient);
	Ret.Reset();
	//regardless of input type, PF_FloatRGBA is required to properly accumulate between frames for a good result.
	Ret.Format = PF_FloatRGBA;
	Ret.DebugName = TEXT("TemporalAA");
	Ret.AutoWritable = false;
	Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
	Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;

	if (OutputExtent.X > 0)
	{
		Ret.Extent = OutputExtent;
	}

	return Ret;
}
