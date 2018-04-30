// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenSpaceReflections.cpp: Post processing Screen Space Reflections implementation.
=============================================================================*/

#include "PostProcess/ScreenSpaceReflections.h"
#include "StaticBoundShaderState.h"
#include "SceneUtils.h"
#include "PostProcess/SceneRenderTargets.h"
#include "SceneRenderTargetParameters.h"
#include "ScenePrivate.h"
#include "PostProcess/SceneFilterRendering.h"
#include "PostProcess/PostProcessInput.h"
#include "PostProcess/PostProcessOutput.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessTemporalAA.h"
#include "PostProcess/PostProcessHierarchical.h"
#include "ClearQuad.h"
#include "PipelineStateCache.h"

static TAutoConsoleVariable<int32> CVarSSRQuality(
	TEXT("r.SSR.Quality"),
	3,
	TEXT("Whether to use screen space reflections and at what quality setting.\n")
	TEXT("(limits the setting in the post process settings which has a different scale)\n")
	TEXT("(costs performance, adds more visual realism but the technique has limits)\n")
	TEXT(" 0: off (default)\n")
	TEXT(" 1: low (no glossy)\n")
	TEXT(" 2: medium (no glossy)\n")
	TEXT(" 3: high (glossy/using roughness, few samples)\n")
	TEXT(" 4: very high (likely too slow for real-time)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarSSRTemporal(
	TEXT("r.SSR.Temporal"),
	0,
	TEXT("Defines if we use the temporal smoothing for the screen space reflection\n")
	TEXT(" 0 is off (for debugging), 1 is on (default)"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarSSRStencil(
	TEXT("r.SSR.Stencil"),
	0,
	TEXT("Defines if we use the stencil prepass for the screen space reflection\n")
	TEXT(" 0 is off (default), 1 is on"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarSSRCone(
	TEXT("r.SSR.Cone"),
	0,
	TEXT("Defines if we use cone traced screen space reflection\n")
	TEXT(" 0 is off (default), 1 is on"),
	ECVF_RenderThreadSafe);

// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
static TAutoConsoleVariable<int32> CVarCombineVXGISpecularWithSSR(
	TEXT("r.VXGI.CombineSpecularWithSSR"),
	0,
	TEXT("Defines if VXGI specular tracing fills is combined with SSR or replaces it\n")
	TEXT(" 0 is replace, 1 is combine"),
	ECVF_RenderThreadSafe);
#endif
// NVCHANGE_END: Add VXGI

DECLARE_GPU_STAT_NAMED(ScreenSpaceReflections, TEXT("ScreenSpace Reflections"));

bool ShouldRenderScreenSpaceReflections(const FViewInfo& View)
{
	// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI
	if (View.FinalPostProcessSettings.VxgiSpecularTracingEnabled && !CVarCombineVXGISpecularWithSSR.GetValueOnRenderThread())
	{
		return false;
	}
#endif
	// NVCHANGE_END: Add VXGI

	if(!View.Family->EngineShowFlags.ScreenSpaceReflections)
	{
		return false;
	}

	if(!View.State)
	{
		// not view state (e.g. thumbnail rendering?), no HZB (no screen space reflections or occlusion culling)
		return false;
	}

	int SSRQuality = CVarSSRQuality.GetValueOnRenderThread();

	if(SSRQuality <= 0)
	{
		return false;
	}

	if(View.FinalPostProcessSettings.ScreenSpaceReflectionIntensity < 1.0f)
	{
		return false;
	}

	if (IsAnyForwardShadingEnabled(View.GetShaderPlatform()))
	{
		return false;
	}

	return true;
}

bool IsSSRTemporalPassRequired(const FViewInfo& View, bool bCheckSSREnabled)
{
	if (bCheckSSREnabled && !ShouldRenderScreenSpaceReflections(View))
	{
		return false;
	}
	if (!View.State)
	{
		return false;
	}
	return View.AntiAliasingMethod != AAM_TemporalAA || CVarSSRTemporal.GetValueOnRenderThread() != 0;
}


static float ComputeRoughnessMaskScale(const FRenderingCompositePassContext& Context, uint32 SSRQuality)
{
	float MaxRoughness = FMath::Clamp(Context.View.FinalPostProcessSettings.ScreenSpaceReflectionMaxRoughness, 0.01f, 1.0f);

	// f(x) = x * Scale + Bias
	// f(MaxRoughness) = 0
	// f(MaxRoughness/2) = 1

	float RoughnessMaskScale = -2.0f / MaxRoughness;
	return RoughnessMaskScale * (SSRQuality < 3 ? 2.0f : 1.0f);
}

FLinearColor ComputeSSRParams(const FRenderingCompositePassContext& Context, uint32 SSRQuality, bool bEnableDiscard)
{
	float RoughnessMaskScale = ComputeRoughnessMaskScale(Context, SSRQuality);

	float FrameRandom = 0;

	if(Context.ViewState)
	{
		bool bTemporalAAIsOn = Context.View.AntiAliasingMethod == AAM_TemporalAA;

		if(bTemporalAAIsOn)
		{
			// usually this number is in the 0..7 range but it depends on the TemporalAA quality
			FrameRandom = Context.ViewState->GetCurrentTemporalAASampleIndex() * 1551;
		}
		else
		{
			// 8 aligns with the temporal smoothing, larger number will do more flickering (power of two for best performance)
			FrameRandom = Context.ViewState->GetFrameIndexMod8() * 1551;
		}
	}

	return FLinearColor(
		FMath::Clamp(Context.View.FinalPostProcessSettings.ScreenSpaceReflectionIntensity * 0.01f, 0.0f, 1.0f), 
		RoughnessMaskScale,
		(float)bEnableDiscard,	// TODO 
		FrameRandom);
}

/**
 * Encapsulates the post processing screen space reflections pixel shader stencil pass.
 */
class FPostProcessScreenSpaceReflectionsStencilPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FPostProcessScreenSpaceReflectionsStencilPS);

	using FPermutationDomain = FShaderPermutationNone;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine( TEXT("PREV_FRAME_COLOR"), uint32(0) );
		OutEnvironment.SetDefine( TEXT("SSR_QUALITY"), uint32(0) );
	}

	/** Default constructor. */
	FPostProcessScreenSpaceReflectionsStencilPS() {}

	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter SSRParams;

	/** Initialization constructor. */
	FPostProcessScreenSpaceReflectionsStencilPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		SSRParams.Bind(Initializer.ParameterMap, TEXT("SSRParams"));
	}

	template <typename TRHICmdList>
	void SetParameters(TRHICmdList& RHICmdList, const FRenderingCompositePassContext& Context, uint32 SSRQuality, bool EnableDiscard)
	{
		const FFinalPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View, MD_PostProcess);

		{
			FLinearColor Value = ComputeSSRParams(Context, SSRQuality, EnableDiscard);

			SetShaderValue(RHICmdList, ShaderRHI, SSRParams, Value);
		}
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << SSRParams;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_GLOBAL_SHADER(FPostProcessScreenSpaceReflectionsStencilPS, "/Engine/Private/ScreenSpaceReflections.usf", "ScreenSpaceReflectionsStencilPS", SF_Pixel);

namespace
{
static const uint32 SSRConeQuality = 5;
static constexpr int32 QualityCount = 6;

class FSSRQualityDim : SHADER_PERMUTATION_INT("SSR_QUALITY", QualityCount);
class FSSRPrevFrameColorDim : SHADER_PERMUTATION_BOOL("PREV_FRAME_COLOR");
class FSSRPrevFrameColorDim2 : SHADER_PERMUTATION_BOOL("PREV_FRAME_COLOR");
}





class FPostProcessScreenSpaceReflectionsPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FPostProcessScreenSpaceReflectionsPS);

	using FPermutationDomain = TShaderPermutationDomain<FSSRQualityDim, FSSRPrevFrameColorDim>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		FPermutationDomain PermutationVector(Parameters.PermutationId);
		if ((PermutationVector.Get<FSSRQualityDim>() == 0) && PermutationVector.Get<FSSRPrevFrameColorDim>())
		{
			return false;
	}
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FPostProcessScreenSpaceReflectionsPS() {}

	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter SSRParams;
	FShaderParameter HZBUvFactorAndInvFactor;
	FShaderParameter PrevScreenPositionScaleBias;
	FShaderParameter PrevSceneColorPreExposureCorrection;

	/** Initialization constructor. */
	FPostProcessScreenSpaceReflectionsPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		SSRParams.Bind(Initializer.ParameterMap, TEXT("SSRParams"));
		HZBUvFactorAndInvFactor.Bind(Initializer.ParameterMap, TEXT("HZBUvFactorAndInvFactor"));
		PrevScreenPositionScaleBias.Bind(Initializer.ParameterMap, TEXT("PrevScreenPositionScaleBias"));
		PrevSceneColorPreExposureCorrection.Bind(Initializer.ParameterMap, TEXT("PrevSceneColorPreExposureCorrection"));
	}

	template <typename TRHICmdList>
	void SetParameters(TRHICmdList& RHICmdList, const FRenderingCompositePassContext& Context, int32 SSRQuality)
	{
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

		const FFinalPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View, MD_PostProcess);

		{
			FLinearColor Value = ComputeSSRParams(Context, SSRQuality, false);

			SetShaderValue(RHICmdList, ShaderRHI, SSRParams, Value);
		}

		{
			const FVector2D HZBUvFactor(
				float(Context.View.ViewRect.Width()) / float(2 * Context.View.HZBMipmap0Size.X),
				float(Context.View.ViewRect.Height()) / float(2 * Context.View.HZBMipmap0Size.Y)
				);
			const FVector4 HZBUvFactorAndInvFactorValue(
				HZBUvFactor.X,
				HZBUvFactor.Y,
				1.0f / HZBUvFactor.X,
				1.0f / HZBUvFactor.Y
				);
			
			SetShaderValue(RHICmdList, ShaderRHI, HZBUvFactorAndInvFactor, HZBUvFactorAndInvFactorValue);
		}

		{
			FIntPoint ViewportOffset = Context.View.ViewRect.Min;
			FIntPoint ViewportExtent = Context.View.ViewRect.Size();
			FIntPoint BufferSize = SceneContext.GetBufferSizeXY();

			if (Context.View.PrevViewInfo.TemporalAAHistory.IsValid())
			{
				ViewportOffset = Context.View.PrevViewInfo.TemporalAAHistory.ViewportRect.Min;
				ViewportExtent = Context.View.PrevViewInfo.TemporalAAHistory.ViewportRect.Size();
				BufferSize = Context.View.PrevViewInfo.TemporalAAHistory.ReferenceBufferSize;
	}

			FVector2D InvBufferSize(1.0f / float(BufferSize.X), 1.0f / float(BufferSize.Y));

			FVector4 ScreenPosToPixelValue(
				ViewportExtent.X * 0.5f * InvBufferSize.X,
				-ViewportExtent.Y * 0.5f * InvBufferSize.Y,
				(ViewportExtent.X * 0.5f + ViewportOffset.X) * InvBufferSize.X,
				(ViewportExtent.Y * 0.5f + ViewportOffset.Y) * InvBufferSize.Y);
			SetShaderValue(Context.RHICmdList, ShaderRHI, PrevScreenPositionScaleBias, ScreenPosToPixelValue);
		}

		{
			float PrevSceneColorPreExposureCorrectionValue = 1.0f;

			if (Context.View.PrevViewInfo.TemporalAAHistory.IsValid())
			{
				PrevSceneColorPreExposureCorrectionValue = Context.View.PreExposure / Context.View.PrevViewInfo.TemporalAAHistory.SceneColorPreExposure;
			}

			SetShaderValue(RHICmdList, ShaderRHI, PrevSceneColorPreExposureCorrection, PrevSceneColorPreExposureCorrectionValue);
		}
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << SSRParams << HZBUvFactorAndInvFactor << PrevScreenPositionScaleBias << PrevSceneColorPreExposureCorrection;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_GLOBAL_SHADER(FPostProcessScreenSpaceReflectionsPS, "/Engine/Private/ScreenSpaceReflections.usf", "ScreenSpaceReflectionsPS", SF_Pixel);

// --------------------------------------------------------


// @param Quality usually in 0..100 range, default is 50
// @return see CVarSSRQuality, never 0
static int32 ComputeSSRQuality(float Quality)
{
	int32 Ret;

	if(Quality >= 60.0f)
	{
		Ret = (Quality >= 80.0f) ? 4 : 3;
	}
	else
	{
		Ret = (Quality >= 40.0f) ? 2 : 1;
	}

	int SSRQualityCVar = FMath::Clamp(CVarSSRQuality.GetValueOnRenderThread(), 0, QualityCount - 1);

	return FMath::Min(Ret, SSRQualityCVar);
}

void FRCPassPostProcessScreenSpaceReflections::Process(FRenderingCompositePassContext& Context)
{
	auto& RHICmdList = Context.RHICmdList;
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	const FViewInfo& View = Context.View;
	const auto FeatureLevel = Context.GetFeatureLevel();

	int32 SSRQuality = ComputeSSRQuality(View.FinalPostProcessSettings.ScreenSpaceReflectionQuality);

	SSRQuality = FMath::Clamp(SSRQuality, 1, 4);
	
	const bool VisualizeSSR = View.Family->EngineShowFlags.VisualizeSSR;
	const bool SSRStencilPrePass = CVarSSRStencil.GetValueOnRenderThread() != 0 && !VisualizeSSR;

	FRenderingCompositeOutputRef* Input2 = GetInput(ePId_Input2);

	const bool SSRConeTracing = Input2 && Input2->GetOutput();
	
	if (VisualizeSSR)
	{
		SSRQuality = 0;
	}
	else if (SSRConeTracing)
	{
		SSRQuality = SSRConeQuality;
	}
	
	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	SCOPED_GPU_STAT(RHICmdList, ScreenSpaceReflections);
	
	if (SSRStencilPrePass)
	{ // ScreenSpaceReflectionsStencil draw event
		SCOPED_DRAW_EVENTF(Context.RHICmdList, ScreenSpaceReflectionsStencil, TEXT("ScreenSpaceReflectionsStencil %dx%d"), View.ViewRect.Width(), View.ViewRect.Height());

		TShaderMapRef< FPostProcessVS > VertexShader(Context.GetShaderMap());
		TShaderMapRef< FPostProcessScreenSpaceReflectionsStencilPS > PixelShader(Context.GetShaderMap());
		
		// bind the dest render target and the depth stencil render target
		SetRenderTarget(RHICmdList, DestRenderTarget.TargetableTexture, SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EUninitializedColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);
		Context.SetViewportAndCallRHI(View.ViewRect);

		// Clear stencil to 0
		DrawClearQuad(RHICmdList, false, FLinearColor(), false, 0, true, 0, PassOutputs[0].RenderTargetDesc.Extent, View.ViewRect);
	
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

		// Clobers the stencil to pixel that should not compute SSR
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always, true, CF_Always, SO_Replace, SO_Replace, SO_Replace>::GetRHI();

		// Set rasterizer state to solid
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();

		// disable blend mode
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();

		// bind shader
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);
		RHICmdList.SetStencilRef(0x80);

		VertexShader->SetParameters(Context);
		PixelShader->SetParameters(Context.RHICmdList, Context, SSRQuality, true);
	
		DrawPostProcessPass(
			Context.RHICmdList,
			0, 0,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Min.X, View.ViewRect.Min.Y,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Size(),
			SceneContext.GetBufferSizeXY(),
			*VertexShader,
			View.StereoPass,
			Context.HasHmdMesh(),
			EDRF_UseTriangleOptimization);

	} // ScreenSpaceReflectionsStencil draw event

	{ // ScreenSpaceReflections draw event
		SCOPED_DRAW_EVENTF(Context.RHICmdList, ScreenSpaceReflections, TEXT("ScreenSpaceReflections %dx%d"), View.ViewRect.Width(), View.ViewRect.Height());

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		if (SSRStencilPrePass)
		{
			// set up the stencil test to match 0, meaning FPostProcessScreenSpaceReflectionsStencilPS has been discarded
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always, true, CF_Equal, SO_Keep, SO_Keep, SO_Keep>::GetRHI();
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		}
		else
		{
			ERenderTargetLoadAction LoadAction = Context.GetLoadActionForRenderTarget(DestRenderTarget);

			FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, LoadAction);
			FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
			Context.RHICmdList.SetRenderTargetsAndClear(Info);
			Context.SetViewportAndCallRHI(View.ViewRect);

			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		}

		// set the state
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		TShaderMapRef< FPostProcessVS > VertexShader(Context.GetShaderMap());

		FPostProcessScreenSpaceReflectionsPS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FSSRPrevFrameColorDim>(bPrevFrame && SSRQuality != 0);
		PermutationVector.Set<FSSRQualityDim>(SSRQuality);

		TShaderMapRef<FPostProcessScreenSpaceReflectionsPS> PixelShader(Context.GetShaderMap(), PermutationVector);
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);
		VertexShader->SetParameters(Context);
		PixelShader->SetParameters(RHICmdList, Context, SSRQuality);

		DrawPostProcessPass(
			RHICmdList,
			0, 0,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Min.X, View.ViewRect.Min.Y,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Size(),
			FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY(),
			*VertexShader,
			View.StereoPass,
			Context.HasHmdMesh(),
			EDRF_UseTriangleOptimization);
	
		RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
	} // ScreenSpaceReflections
}

FPooledRenderTargetDesc FRCPassPostProcessScreenSpaceReflections::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret(FPooledRenderTargetDesc::Create2DDesc(FSceneRenderTargets::Get_FrameConstantsOnly().GetBufferSizeXY(), PF_FloatRGBA, FClearValueBinding::None, TexCreate_None, TexCreate_RenderTargetable, false));

	Ret.ClearValue = FClearValueBinding(FLinearColor(0, 0, 0, 0));
	Ret.DebugName = TEXT("ScreenSpaceReflections");
	Ret.AutoWritable = false;
	return Ret;
}

void RenderScreenSpaceReflections(FRHICommandListImmediate& RHICmdList, FViewInfo& View, TRefCountPtr<IPooledRenderTarget>& SSROutput, TRefCountPtr<IPooledRenderTarget>& VelocityRT)
{
	FRenderingCompositePassContext CompositeContext(RHICmdList, View);	
	FPostprocessContext Context(RHICmdList, CompositeContext.Graph, View );

	FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

	FRenderingCompositePass* SceneColorInput = Context.Graph.RegisterPass( new FRCPassPostProcessInput( FSceneRenderTargets::Get(RHICmdList).GetSceneColor() ) );
	FRenderingCompositePass* HZBInput = Context.Graph.RegisterPass( new FRCPassPostProcessInput( View.HZB ) );
	FRenderingCompositePass* HCBInput = nullptr;

	bool bSamplePrevFrame = false;
	if (View.PrevViewInfo.CustomSSRInput.IsValid())
	{
		SceneColorInput = Context.Graph.RegisterPass(new FRCPassPostProcessInput(View.PrevViewInfo.CustomSSRInput));
		bSamplePrevFrame = true;
	}
	else if (View.PrevViewInfo.TemporalAAHistory.IsValid())
	{
		SceneColorInput = Context.Graph.RegisterPass(new FRCPassPostProcessInput(View.PrevViewInfo.TemporalAAHistory.RT[0]));
		bSamplePrevFrame = true;
	}

	FRenderingCompositeOutputRef VelocityInput;
	if ( VelocityRT && !Context.View.bCameraCut )
	{
		VelocityInput = Context.Graph.RegisterPass(new FRCPassPostProcessInput(VelocityRT));
	}
	else
	{
		// No velocity, use black
		VelocityInput = Context.Graph.RegisterPass(new FRCPassPostProcessInput(GSystemTextures.BlackDummy));
	}
	
	if (CVarSSRCone.GetValueOnRenderThread())
	{
		HCBInput = Context.Graph.RegisterPass( new FRCPassPostProcessBuildHCB() );
		HCBInput->SetInput( ePId_Input0, SceneColorInput );
	}

	{
		FRenderingCompositePass* TracePass = Context.Graph.RegisterPass(
			new FRCPassPostProcessScreenSpaceReflections( bSamplePrevFrame ) );
		TracePass->SetInput( ePId_Input0, SceneColorInput );
		TracePass->SetInput( ePId_Input1, HZBInput );
		TracePass->SetInput( ePId_Input2, HCBInput );
		TracePass->SetInput( ePId_Input3, VelocityInput );

		Context.FinalOutput = FRenderingCompositeOutputRef( TracePass );
	}

	const bool bTemporalFilter = IsSSRTemporalPassRequired(View, false);

	if( ViewState && bTemporalFilter )
	{
		{
			FRenderingCompositeOutputRef HistoryInput;
			if( ViewState->SSRHistory.IsValid() && !Context.View.bCameraCut )
			{
				HistoryInput = Context.Graph.RegisterPass( new FRCPassPostProcessInput( ViewState->SSRHistory.RT[0] ) );
			}
			else
			{
				// No history, use black
				HistoryInput = Context.Graph.RegisterPass(new FRCPassPostProcessInput(GSystemTextures.BlackDummy));
			}

			FRenderingCompositePass* TemporalAAPass = Context.Graph.RegisterPass( new FRCPassPostProcessSSRTemporalAA(ViewState->SSRHistory, &ViewState->SSRHistory) );
			TemporalAAPass->SetInput( ePId_Input0, Context.FinalOutput );
			TemporalAAPass->SetInput( ePId_Input1, HistoryInput );
			TemporalAAPass->SetInput( ePId_Input2, VelocityInput );

			Context.FinalOutput = FRenderingCompositeOutputRef( TemporalAAPass );
		}

		FRenderingCompositePass* HistoryOutput = Context.Graph.RegisterPass( new FRCPassPostProcessOutput( &ViewState->SSRHistory.RT[0] ) );
		HistoryOutput->SetInput( ePId_Input0, Context.FinalOutput );

		Context.FinalOutput = FRenderingCompositeOutputRef( HistoryOutput );
	}

	{
		FRenderingCompositePass* ReflectionOutput = Context.Graph.RegisterPass( new FRCPassPostProcessOutput( &SSROutput ) );
		ReflectionOutput->SetInput( ePId_Input0, Context.FinalOutput );

		Context.FinalOutput = FRenderingCompositeOutputRef( ReflectionOutput );
	}

	CompositeContext.Process(Context.FinalOutput.GetPass(), TEXT("ReflectionEnvironments"));
}
