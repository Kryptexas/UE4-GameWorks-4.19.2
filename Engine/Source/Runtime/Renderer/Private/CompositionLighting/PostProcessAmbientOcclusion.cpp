// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessAmbientOcclusion.cpp: Post processing ambient occlusion implementation.
=============================================================================*/

#include "CompositionLighting/PostProcessAmbientOcclusion.h"
#include "StaticBoundShaderState.h"
#include "SceneUtils.h"
#include "PostProcess/SceneRenderTargets.h"
#include "SceneRenderTargetParameters.h"
#include "ScenePrivate.h"
#include "PostProcess/SceneFilterRendering.h"
#include "PostProcess/PostProcessing.h"
#include "PipelineStateCache.h"

// Tile size for the AmbientOcclusion compute shader, tweaked for 680 GTX. */
// see GCN Performance Tip 21 http://developer.amd.com/wordpress/media/2013/05/GCNPerformanceTweets.pdf 
const int32 GAmbientOcclusionTileSizeX = 16;
const int32 GAmbientOcclusionTileSizeY = 16;

static TAutoConsoleVariable<int32> CVarAmbientOcclusionCompute(
	TEXT("r.AmbientOcclusion.Compute"),
	0,
	TEXT("If SSAO should use ComputeShader (not available on all platforms) or PixelShader.\n")
	TEXT("The [Async] Compute Shader version is WIP, not optimized, requires hardware support (not mobile/DX10/OpenGL3),\n")
	TEXT("does not use normals which allows it to run right after EarlyZPass (better performance when used with AyncCompute)\n")
	TEXT("AyncCompute is currently only functional on PS4.\n")
	TEXT(" 0: PixelShader (default)\n")
	TEXT(" 1: (WIP) Use ComputeShader if possible, otherwise fall back to '0'\n")
	TEXT(" 2: (WIP) Use AsyncCompute if efficient, otherwise fall back to '1'\n")
	TEXT(" 3: (WIP) Use AsyncCompute if possible, otherwise fall back to '1'"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarAmbientOcclusionMaxQuality(
	TEXT("r.AmbientOcclusionMaxQuality"),
	100.0f,
	TEXT("Defines the max clamping value from the post process volume's quality level for ScreenSpace Ambient Occlusion\n")
	TEXT("     100: don't override quality level from the post process volume (default)\n")
	TEXT("   0..99: clamp down quality level from the post process volume to the maximum set by this cvar\n")
	TEXT(" -100..0: Enforces a different quality (the absolute value) even if the postprocessvolume asks for a lower quality."),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarAmbientOcclusionStepMipLevelFactor(
	TEXT("r.AmbientOcclusionMipLevelFactor"),
	0.5f,
	TEXT("Controls mipmap level according to the SSAO step id\n")
	TEXT(" 0: always look into the HZB mipmap level 0 (memory cache trashing)\n")
	TEXT(" 0.5: sample count depends on post process settings (default)\n")
	TEXT(" 1: Go into higher mipmap level (quality loss)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarAmbientOcclusionLevels(
	TEXT("r.AmbientOcclusionLevels"),
	-1,
	TEXT("Defines how many mip levels are using during the ambient occlusion calculation. This is useful when tweaking the algorithm.\n")
	TEXT("<0: decide based on the quality setting in the postprocess settings/volume and r.AmbientOcclusionMaxQuality (default)\n")
	TEXT(" 0: none (disable AmbientOcclusion)\n")
	TEXT(" 1: one\n")
	TEXT(" 2: two (costs extra performance, soft addition)\n")
	TEXT(" 3: three (larger radius cost less but can flicker)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarAmbientOcclusionAsyncComputeBudget(
	TEXT("r.AmbientOcclusion.AsyncComputeBudget"),
	1,
	TEXT("Defines which level of EAsyncComputeBudget to use for balancing AsyncCompute work against Gfx work.\n")
	TEXT("Only matters if the compute version of SSAO is active (requires CS support, enabled by cvar, single pass, no normals)\n")
	TEXT("This is a low level developer tweak to get best performance on hardware that supports AsyncCompute.\n")
	TEXT(" 0: least AsyncCompute\n")
	TEXT(" 1: .. (default)\n")
	TEXT(" 2: .. \n")
	TEXT(" 3: .. \n")
	TEXT(" 4: most AsyncCompute"),
	ECVF_RenderThreadSafe);

float FSSAOHelper::GetAmbientOcclusionQualityRT(const FSceneView& View)
{
	float CVarValue = CVarAmbientOcclusionMaxQuality.GetValueOnRenderThread();

	if (CVarValue < 0)
	{
		return FMath::Clamp(-CVarValue, 0.0f, 100.0f);
	}
	else
	{
		return FMath::Min(CVarValue, View.FinalPostProcessSettings.AmbientOcclusionQuality);
	}
}

int32 FSSAOHelper::GetAmbientOcclusionShaderLevel(const FSceneView& View)
{
	float QualityPercent = GetAmbientOcclusionQualityRT(View);

	return	(QualityPercent > 75.0f) +
		(QualityPercent > 55.0f) +
		(QualityPercent > 25.0f) +
		(QualityPercent > 5.0f);
}

bool FSSAOHelper::IsAmbientOcclusionCompute(const FSceneView& View)
{
	return View.GetFeatureLevel() >= ERHIFeatureLevel::SM5 && (CVarAmbientOcclusionCompute.GetValueOnRenderThread() >= 1);
}

int32 FSSAOHelper::GetNumAmbientOcclusionLevels()
{
	return CVarAmbientOcclusionLevels.GetValueOnRenderThread();
}

float FSSAOHelper::GetAmbientOcclusionStepMipLevelFactor()
{
	return CVarAmbientOcclusionStepMipLevelFactor.GetValueOnRenderThread();
}

EAsyncComputeBudget FSSAOHelper::GetAmbientOcclusionAsyncComputeBudget()
{
	int32 RawBudget = CVarAmbientOcclusionAsyncComputeBudget.GetValueOnRenderThread();

	return (EAsyncComputeBudget)FMath::Clamp(RawBudget, (int32)EAsyncComputeBudget::ELeast_0, (int32)EAsyncComputeBudget::EAll_4);
}

bool FSSAOHelper::IsBasePassAmbientOcclusionRequired(const FViewInfo& View)
{
	// the BaseAO pass is only worth with some AO
	return (View.FinalPostProcessSettings.AmbientOcclusionStaticFraction >= 1 / 100.0f) && !IsAnyForwardShadingEnabled(View.GetShaderPlatform());
}

bool FSSAOHelper::IsAmbientOcclusionAsyncCompute(const FViewInfo& View, uint32 AOPassCount)
{
	// if AsyncCompute is feasible
	if(IsAmbientOcclusionCompute(View) && (AOPassCount > 0))
	{
		int32 ComputeCVar = CVarAmbientOcclusionCompute.GetValueOnRenderThread();

		if(ComputeCVar >= 2)
		{
			// we might want AsyncCompute

			if(ComputeCVar == 3)
			{
				// enforced, no matter if efficient hardware support
				return true;
			}

			// depends on efficient hardware support
			return GSupportsEfficientAsyncCompute;
		}
	}

	return false;
}


/** Shader parameters needed for screen space AmbientOcclusion passes. */
class FScreenSpaceAOParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		ScreenSpaceAOParams.Bind(ParameterMap, TEXT("ScreenSpaceAOParams"));
	}

	//@param TRHICmdList could be async compute or compute dispatch, so template on commandlist type.
	template<typename ShaderRHIParamRef, typename TRHICmdList>
	void Set(TRHICmdList& RHICmdList, const FViewInfo& View, const ShaderRHIParamRef ShaderRHI, FIntPoint InputTextureSize) const
	{
		const FFinalPostProcessSettings& Settings = View.FinalPostProcessSettings;

		FIntPoint SSAORandomizationSize = GSystemTextures.SSAORandomization->GetDesc().Extent;
		FVector2D ViewportUVToRandomUV(InputTextureSize.X / (float)SSAORandomizationSize.X, InputTextureSize.Y / (float)SSAORandomizationSize.Y);

		// e.g. 4 means the input texture is 4x smaller than the buffer size
		uint32 ScaleToFullRes = FSceneRenderTargets::Get(RHICmdList).GetBufferSizeXY().X / InputTextureSize.X;

		FIntRect ViewRect = FIntRect::DivideAndRoundUp(View.ViewRect, ScaleToFullRes);

		float AORadiusInShader = Settings.AmbientOcclusionRadius;
		float ScaleRadiusInWorldSpace = 1.0f;

		if(!Settings.AmbientOcclusionRadiusInWS)
		{
			// radius is defined in view space in 400 units
			AORadiusInShader /= 400.0f;
			ScaleRadiusInWorldSpace = 0.0f;
		}

		// /4 is an adjustment for usage with multiple mips
		float f = FMath::Log2(ScaleToFullRes);
		float g = pow(Settings.AmbientOcclusionMipScale, f);
		AORadiusInShader *= pow(Settings.AmbientOcclusionMipScale, FMath::Log2(ScaleToFullRes)) / 4.0f;

		float Ratio = View.UnscaledViewRect.Width() / (float)View.UnscaledViewRect.Height();

		// Grab this and pass into shader so we can negate the fov influence of projection on the screen pos.
		float InvTanHalfFov = View.ViewMatrices.GetProjectionMatrix().M[0][0];

		FVector4 Value[6];

		float StaticFraction = FMath::Clamp(Settings.AmbientOcclusionStaticFraction, 0.0f, 1.0f);

		// clamp to prevent user error
		float FadeRadius = FMath::Max(1.0f, Settings.AmbientOcclusionFadeRadius);
		float InvFadeRadius = 1.0f / FadeRadius;

		FVector2D TemporalOffset(0.0f, 0.0f);
		
		if(View.State)
		{
			TemporalOffset = (View.State->GetCurrentTemporalAASampleIndex() % 8) * FVector2D(2.48f, 7.52f) / 64.0f;
		}
		const float HzbStepMipLevelFactorValue = FMath::Clamp(FSSAOHelper::GetAmbientOcclusionStepMipLevelFactor(), 0.0f, 100.0f);

		// /1000 to be able to define the value in that distance
		Value[0] = FVector4(Settings.AmbientOcclusionPower, Settings.AmbientOcclusionBias / 1000.0f, 1.0f / Settings.AmbientOcclusionDistance_DEPRECATED, Settings.AmbientOcclusionIntensity);
		Value[1] = FVector4(ViewportUVToRandomUV.X, ViewportUVToRandomUV.Y, AORadiusInShader, Ratio);
		Value[2] = FVector4(ScaleToFullRes, Settings.AmbientOcclusionMipThreshold / ScaleToFullRes, ScaleRadiusInWorldSpace, Settings.AmbientOcclusionMipBlend);
		Value[3] = FVector4(TemporalOffset.X, TemporalOffset.Y, StaticFraction, InvTanHalfFov);
		Value[4] = FVector4(InvFadeRadius, -(Settings.AmbientOcclusionFadeDistance - FadeRadius) * InvFadeRadius, HzbStepMipLevelFactorValue, 0);
		Value[5] = FVector4(View.ViewRect.Width(), View.ViewRect.Height(), ViewRect.Min.X, ViewRect.Min.Y);

		SetShaderValueArray(RHICmdList, ShaderRHI, ScreenSpaceAOParams, Value, 6);
	}

	friend FArchive& operator<<(FArchive& Ar, FScreenSpaceAOParameters& This);

private:
	FShaderParameter ScreenSpaceAOParams;
};

/** Encapsulates the post processing ambient occlusion pixel shader. */
template <uint32 bInitialPass>
class FPostProcessAmbientOcclusionSetupPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessAmbientOcclusionSetupPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("INITIAL_PASS"), bInitialPass);
	}

	/** Default constructor. */
	FPostProcessAmbientOcclusionSetupPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter AmbientOcclusionSetupParams;

	/** Initialization constructor. */
	FPostProcessAmbientOcclusionSetupPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		AmbientOcclusionSetupParams.Bind(Initializer.ParameterMap, TEXT("AmbientOcclusionSetupParams"));
	}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		const FFinalPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		PostprocessParameter.SetPS(Context.RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
		DeferredParameters.Set(Context.RHICmdList, ShaderRHI, Context.View, MD_PostProcess);

		// e.g. 4 means the input texture is 4x smaller than the buffer size
		uint32 ScaleToFullRes = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY().X / Context.Pass->GetOutput(ePId_Output0)->RenderTargetDesc.Extent.X;

		// /1000 to be able to define the value in that distance
		FVector4 AmbientOcclusionSetupParamsValue = FVector4(ScaleToFullRes, Settings.AmbientOcclusionMipThreshold / ScaleToFullRes, Context.View.ViewRect.Width(), Context.View.ViewRect.Height());
		SetShaderValue(Context.RHICmdList, ShaderRHI, AmbientOcclusionSetupParams, AmbientOcclusionSetupParamsValue);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << AmbientOcclusionSetupParams;
		return bShaderHasOutdatedParameters;
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("/Engine/Private/PostProcessAmbientOcclusion.usf");
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("MainSetupPS");
	}
};


// #define avoids a lot of code duplication
#define VARIATION1(A) typedef FPostProcessAmbientOcclusionSetupPS<A> FPostProcessAmbientOcclusionSetupPS##A; \
	IMPLEMENT_SHADER_TYPE2(FPostProcessAmbientOcclusionSetupPS##A, SF_Pixel);

	VARIATION1(0)			VARIATION1(1)
#undef VARIATION1

// --------------------------------------------------------
void FRCPassPostProcessAmbientOcclusionSetup::Process(FRenderingCompositePassContext& Context)
{
	const FViewInfo& View = Context.View;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY().X / DestSize.X;

	FIntRect SrcRect = View.ViewRect;
	FIntRect DestRect = SrcRect  / ScaleFactor;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, AmbientOcclusionSetup, TEXT("AmbientOcclusionSetup %dx%d"), DestRect.Width(), DestRect.Height());

	// Set the view family's render target/viewport.
	SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIParamRef());

	Context.SetViewportAndCallRHI(DestRect);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());

	if(IsInitialPass())
	{
		TShaderMapRef<FPostProcessAmbientOcclusionSetupPS<1> > PixelShader(Context.GetShaderMap());

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

		PixelShader->SetParameters(Context);
	}
	else
	{
		TShaderMapRef<FPostProcessAmbientOcclusionSetupPS<0> > PixelShader(Context.GetShaderMap());

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

		PixelShader->SetParameters(Context);
	}

	VertexShader->SetParameters(Context);
	DrawPostProcessPass(
		Context.RHICmdList,
		0, 0,
		DestRect.Width(), DestRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y, 
		SrcRect.Width(), SrcRect.Height(),
		DestRect.Size(),
		FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY(),
		*VertexShader,
		View.StereoPass,
		Context.HasHmdMesh(),
		EDRF_UseTriangleOptimization);

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessAmbientOcclusionSetup::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret;
	
	if(IsInitialPass())
	{
		Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
	}
	else
	{
		Ret = GetInput(ePId_Input1)->GetOutput()->RenderTargetDesc;
	}

	Ret.Reset();
	Ret.Format = PF_FloatRGBA;
	Ret.ClearValue = FClearValueBinding::None;
	Ret.TargetableFlags &= ~TexCreate_DepthStencilTargetable;
	Ret.TargetableFlags |= TexCreate_RenderTargetable;
	Ret.Extent = FIntPoint::DivideAndRoundUp(Ret.Extent, 2);

	Ret.DebugName = TEXT("AmbientOcclusionSetup");
	
	return Ret;
}

bool FRCPassPostProcessAmbientOcclusionSetup::IsInitialPass() const
{
	const FPooledRenderTargetDesc* InputDesc0 = GetInputDesc(ePId_Input0);
	const FPooledRenderTargetDesc* InputDesc1 = GetInputDesc(ePId_Input1);

	if(!InputDesc0 && InputDesc1)
	{
		return false;
	}
	if(InputDesc0 && !InputDesc1)
	{
		return true;
	}
	// internal error, SetInput() was done wrong
	check(0);
	return false;
}

// --------------------------------------------------------

FArchive& operator<<(FArchive& Ar, FScreenSpaceAOParameters& This)
{
	Ar << This.ScreenSpaceAOParams;

	return Ar;
}

// --------------------------------------------------------

/**
 * Encapsulates the post processing ambient occlusion pixel shader.
 * @param bAOSetupAsInput true:use AO setup instead of full resolution depth and normal
 * @param bDoUpsample true:we have lower resolution pass data we need to upsample, false otherwise
 * @param ShaderQuality 0..4, 0:low 4:high
 */
template<uint32 bTAOSetupAsInput, uint32 bDoUpsample, uint32 ShaderQuality, uint32 bComputeShader>
class FPostProcessAmbientOcclusionPSandCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessAmbientOcclusionPSandCS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		if(bComputeShader)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}
		else
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
		}
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters,OutEnvironment);

		OutEnvironment.SetDefine(TEXT("USE_UPSAMPLE"), bDoUpsample);
		OutEnvironment.SetDefine(TEXT("USE_AO_SETUP_AS_INPUT"), bTAOSetupAsInput);
		OutEnvironment.SetDefine(TEXT("SHADER_QUALITY"), ShaderQuality);
		OutEnvironment.SetDefine(TEXT("COMPUTE_SHADER"), bComputeShader);

		if(bComputeShader)
		{
			OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GAmbientOcclusionTileSizeX);
			OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GAmbientOcclusionTileSizeY);
		}
	}

	/** Default constructor. */
	FPostProcessAmbientOcclusionPSandCS() {}

public:
	FShaderParameter HZBRemapping;
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FScreenSpaceAOParameters ScreenSpaceAOParams;
	FShaderResourceParameter RandomNormalTexture;
	FShaderResourceParameter RandomNormalTextureSampler;
	FShaderParameter OutTexture;

	/** Initialization constructor. */
	FPostProcessAmbientOcclusionPSandCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		ScreenSpaceAOParams.Bind(Initializer.ParameterMap);
		RandomNormalTexture.Bind(Initializer.ParameterMap, TEXT("RandomNormalTexture"));
		RandomNormalTextureSampler.Bind(Initializer.ParameterMap, TEXT("RandomNormalTextureSampler"));
		HZBRemapping.Bind(Initializer.ParameterMap, TEXT("HZBRemapping"));
		OutTexture.Bind(Initializer.ParameterMap, TEXT("OutTexture"));
	}

	FVector4 GetHZBValue(const FViewInfo& View)
	{
		const FVector2D HZBScaleFactor(
			float(View.ViewRect.Width()) / float(2 * View.HZBMipmap0Size.X),
			float(View.ViewRect.Height()) / float(2 * View.HZBMipmap0Size.Y));

		// from -1..1 to UV 0..1*HZBScaleFactor
		// .xy:mul, zw:add
		const FVector4 HZBRemappingValue(
			0.5f * HZBScaleFactor.X,
			-0.5f * HZBScaleFactor.Y,
			0.5f * HZBScaleFactor.X,
			0.5f * HZBScaleFactor.Y);

		return HZBRemappingValue;
	}
	
	template <typename TRHICmdList>
	void SetParametersCompute(TRHICmdList& RHICmdList, const FRenderingCompositePassContext& Context, FIntPoint InputTextureSize, FUnorderedAccessViewRHIParamRef OutUAV)
	{
		const FViewInfo& View = Context.View;
		const FVector4 HZBRemappingValue = GetHZBValue(View);				
		const FSceneRenderTargetItem& SSAORandomization = GSystemTextures.SSAORandomization->GetRenderTargetItem();

		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, View.ViewUniformBuffer);			
		RHICmdList.SetUAVParameter(ShaderRHI, OutTexture.GetBaseIndex(), OutUAV);

		// SF_Point is better than bilinear to avoid halos around objects
		DeferredParameters.Set(RHICmdList, ShaderRHI, View, MD_PostProcess);
		PostprocessParameter.SetCS(ShaderRHI, Context, RHICmdList, TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());

		SetTextureParameter(RHICmdList, ShaderRHI, RandomNormalTexture, RandomNormalTextureSampler, TStaticSamplerState<SF_Point, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI(), SSAORandomization.ShaderResourceTexture);
		ScreenSpaceAOParams.Set(RHICmdList, View, ShaderRHI, InputTextureSize);
		SetShaderValue(RHICmdList, ShaderRHI, HZBRemapping, HZBRemappingValue);			
	}

	
	void SetParametersGfx(FRHICommandList& RHICmdList, const FRenderingCompositePassContext& Context, FIntPoint InputTextureSize, FUnorderedAccessViewRHIParamRef OutUAV)
	{
		const FViewInfo& View = Context.View;
		const FVector4 HZBRemappingValue = GetHZBValue(View);
		const FSceneRenderTargetItem& SSAORandomization = GSystemTextures.SSAORandomization->GetRenderTargetItem();

		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, View.ViewUniformBuffer);

		// SF_Point is better than bilinear to avoid halos around objects
		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
		DeferredParameters.Set(RHICmdList, ShaderRHI, View, MD_PostProcess);

		SetTextureParameter(RHICmdList, ShaderRHI, RandomNormalTexture, RandomNormalTextureSampler, TStaticSamplerState<SF_Point, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI(), SSAORandomization.ShaderResourceTexture);
		ScreenSpaceAOParams.Set(RHICmdList, View, ShaderRHI, InputTextureSize);
		SetShaderValue(RHICmdList, ShaderRHI, HZBRemapping, HZBRemappingValue);
	}

	template <typename TRHICmdList>
	void UnsetParameters(TRHICmdList& RHICmdList)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		RHICmdList.SetUAVParameter(ShaderRHI, OutTexture.GetBaseIndex(), NULL);
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << HZBRemapping << PostprocessParameter << DeferredParameters << ScreenSpaceAOParams << RandomNormalTexture << RandomNormalTextureSampler << OutTexture;
		return bShaderHasOutdatedParameters;
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("/Engine/Private/PostProcessAmbientOcclusion.usf");
	}

	static const TCHAR* GetFunctionName()
	{
		return bComputeShader ? TEXT("MainCS") : TEXT("MainPS");
	}
};


// #define avoids a lot of code duplication
#define VARIATION0(C)	    VARIATION1(0, C) VARIATION1(1, C)
#define VARIATION1(A, C)	VARIATION2(A, 0, C) VARIATION2(A, 1, C)
#define VARIATION2(A, B, C) \
	typedef FPostProcessAmbientOcclusionPSandCS<A, B, C, false> FPostProcessAmbientOcclusionPS##A##B##C; \
	typedef FPostProcessAmbientOcclusionPSandCS<A, B, C, true> FPostProcessAmbientOcclusionCS##A##B##C; \
	IMPLEMENT_SHADER_TYPE2(FPostProcessAmbientOcclusionPS##A##B##C, SF_Pixel); \
	IMPLEMENT_SHADER_TYPE2(FPostProcessAmbientOcclusionCS##A##B##C, SF_Compute);

	VARIATION0(0)
	VARIATION0(1)
	VARIATION0(2)
	VARIATION0(3)
	VARIATION0(4)
	
#undef VARIATION0
#undef VARIATION1
#undef VARIATION2

// ---------------------------------

template <uint32 bTAOSetupAsInput, uint32 bDoUpsample, uint32 ShaderQuality>
FShader* FRCPassPostProcessAmbientOcclusion::SetShaderTemplPS(const FRenderingCompositePassContext& Context, FGraphicsPipelineStateInitializer& GraphicsPSOInit)
{
	TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
	TShaderMapRef<FPostProcessAmbientOcclusionPSandCS<bTAOSetupAsInput, bDoUpsample, ShaderQuality, false> > PixelShader(Context.GetShaderMap());

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

	const FPooledRenderTargetDesc* InputDesc0 = GetInputDesc(ePId_Input0);

	VertexShader->SetParameters(Context);
	PixelShader->SetParametersGfx(Context.RHICmdList, Context, InputDesc0->Extent, 0);

	return *VertexShader;
}

template <uint32 bTAOSetupAsInput, uint32 bDoUpsample, uint32 ShaderQuality, typename TRHICmdList>
void FRCPassPostProcessAmbientOcclusion::DispatchCS(TRHICmdList& RHICmdList, const FRenderingCompositePassContext& Context, const FIntPoint& TexSize, FUnorderedAccessViewRHIParamRef OutUAV)
{
	TShaderMapRef<FPostProcessAmbientOcclusionPSandCS<bTAOSetupAsInput, bDoUpsample, ShaderQuality, true> > ComputeShader(Context.GetShaderMap());

	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

	ComputeShader->SetParametersCompute(RHICmdList, Context, TexSize, OutUAV);

	uint32 ScaleToFullRes = SceneContext.GetBufferSizeXY().X / TexSize.X;

	FIntRect ViewRect = FIntRect::DivideAndRoundUp(Context.View.ViewRect, ScaleToFullRes);

	uint32 GroupSizeX = FMath::DivideAndRoundUp(ViewRect.Size().X, GAmbientOcclusionTileSizeX);
	uint32 GroupSizeY = FMath::DivideAndRoundUp(ViewRect.Size().Y, GAmbientOcclusionTileSizeY);
	DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

	ComputeShader->UnsetParameters(RHICmdList);
}



// --------------------------------------------------------

FRCPassPostProcessAmbientOcclusion::FRCPassPostProcessAmbientOcclusion(const FSceneView& View, ESSAOType InAOType, bool bInAOSetupAsInput)
	: AOType(InAOType)
	, bAOSetupAsInput(bInAOSetupAsInput)	
{
}

void FRCPassPostProcessAmbientOcclusion::ProcessCS(FRenderingCompositePassContext& Context, const FSceneRenderTargetItem* DestRenderTarget,
	const FIntRect& ViewRect, const FIntPoint& TexSize, int32 ShaderQuality, bool bDoUpsample)
{
#define SET_SHADER_CASE(RHICmdList, ShaderQualityCase) \
	case ShaderQualityCase: \
	if (bAOSetupAsInput) \
	{ \
		if (bDoUpsample) DispatchCS<1, 1, ShaderQualityCase>(RHICmdList, Context, TexSize, DestRenderTarget->UAV); \
		else DispatchCS<1, 0, ShaderQualityCase>(RHICmdList, Context, TexSize, DestRenderTarget->UAV); \
	} \
	else \
	{ \
		if (bDoUpsample) DispatchCS<0, 1, ShaderQualityCase>(RHICmdList, Context, TexSize, DestRenderTarget->UAV); \
		else DispatchCS<0, 0, ShaderQualityCase>(RHICmdList, Context, TexSize, DestRenderTarget->UAV); \
	} \
	break

	SetRenderTarget(Context.RHICmdList, nullptr, nullptr);
	Context.SetViewportAndCallRHI(ViewRect, 0.0f, 1.0f);

	//for async compute we need to set up a fence to make sure the resource is ready before we start.
	if (AOType == ESSAOType::EAsyncCS)
	{
		//Grab the async compute commandlist.
		FRHIAsyncComputeCommandListImmediate& RHICmdListComputeImmediate = FRHICommandListExecutor::GetImmediateAsyncComputeCommandList();

		static FName AsyncStartFenceName(TEXT("AsyncStartFence"));
		FComputeFenceRHIRef AsyncStartFence = Context.RHICmdList.CreateComputeFence(AsyncStartFenceName);

		//Fence to let us know when the Gfx pipe is done with the RT we want to write to.
		Context.RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget->UAV, AsyncStartFence);

		SCOPED_COMPUTE_EVENT(RHICmdListComputeImmediate, AsyncSSAO);
		//Async compute must wait for Gfx to be done with our dest target before we can dispatch anything.
		RHICmdListComputeImmediate.WaitComputeFence(AsyncStartFence);

		switch (ShaderQuality)
		{
			SET_SHADER_CASE(RHICmdListComputeImmediate, 0);
			SET_SHADER_CASE(RHICmdListComputeImmediate, 1);
			SET_SHADER_CASE(RHICmdListComputeImmediate, 2);
			SET_SHADER_CASE(RHICmdListComputeImmediate, 3);
			SET_SHADER_CASE(RHICmdListComputeImmediate, 4);
		default:
			break;
		};
	}
	else
	{
		//no fence necessary for inline compute.
		Context.RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget->UAV, nullptr);
		switch (ShaderQuality)
		{
			SET_SHADER_CASE(Context.RHICmdList, 0);
			SET_SHADER_CASE(Context.RHICmdList, 1);
			SET_SHADER_CASE(Context.RHICmdList, 2);
			SET_SHADER_CASE(Context.RHICmdList, 3);
			SET_SHADER_CASE(Context.RHICmdList, 4);
		default:
			break;
		};
	}
	Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, DestRenderTarget->TargetableTexture);
#undef SET_SHADER_CASE	
}

void FRCPassPostProcessAmbientOcclusion::ProcessPS(FRenderingCompositePassContext& Context, const FSceneRenderTargetItem* DestRenderTarget,
	const FIntRect& ViewRect, const FIntPoint& TexSize, int32 ShaderQuality, bool bDoUpsample)
{
	// Set the view family's render target/viewport.
	SetRenderTarget(Context.RHICmdList, DestRenderTarget->TargetableTexture, FTextureRHIRef());
	Context.SetViewportAndCallRHI(ViewRect);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	// set the state
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	FShader* VertexShader = 0;

#define SET_SHADER_CASE(ShaderQualityCase) \
		case ShaderQualityCase: \
	if (bAOSetupAsInput) \
	{ \
		if (bDoUpsample) VertexShader = SetShaderTemplPS<1, 1, ShaderQualityCase>(Context, GraphicsPSOInit); \
		else VertexShader = SetShaderTemplPS<1, 0, ShaderQualityCase>(Context, GraphicsPSOInit); \
	} \
	else \
	{ \
		if (bDoUpsample) VertexShader = SetShaderTemplPS<0, 1, ShaderQualityCase>(Context, GraphicsPSOInit); \
		else VertexShader = SetShaderTemplPS<0, 0, ShaderQualityCase>(Context, GraphicsPSOInit); \
	} \
	break

	switch (ShaderQuality)
	{
		SET_SHADER_CASE(0);
		SET_SHADER_CASE(1);
		SET_SHADER_CASE(2);
		SET_SHADER_CASE(3);
		SET_SHADER_CASE(4);
	default:
		break;
	};
#undef SET_SHADER_CASE

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		Context.RHICmdList,
		0, 0,
		ViewRect.Width(), ViewRect.Height(),
		ViewRect.Min.X, ViewRect.Min.Y,
		ViewRect.Width(), ViewRect.Height(),
		ViewRect.Size(),
		TexSize,
		VertexShader,
		EDRF_UseTriangleOptimization);

	Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, DestRenderTarget->TargetableTexture);
}

void FRCPassPostProcessAmbientOcclusion::Process(FRenderingCompositePassContext& Context)
{
	const FViewInfo& View = Context.View;

	const FPooledRenderTargetDesc* InputDesc0 = GetInputDesc(ePId_Input0);
	const FPooledRenderTargetDesc* InputDesc2 = GetInputDesc(ePId_Input2);

	const FSceneRenderTargetItem* DestRenderTarget = 0;
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

	if(bAOSetupAsInput)
	{
		DestRenderTarget = &PassOutputs[0].RequestSurface(Context);
	}
	else
	{
		DestRenderTarget = &SceneContext.ScreenSpaceAO->GetRenderTargetItem();
	}

	// Compute doesn't have Input0, it runs in full resolution
	FIntPoint TexSize = InputDesc0 ? InputDesc0->Extent : SceneContext.GetBufferSizeXY();

	// usually 1, 2, 4 or 8
	uint32 ScaleToFullRes = SceneContext.GetBufferSizeXY().X / TexSize.X;

	FIntRect ViewRect = FIntRect::DivideAndRoundUp(View.ViewRect, ScaleToFullRes);

	// 0..4, 0:low 4:high
	const int32 ShaderQuality = FSSAOHelper::GetAmbientOcclusionShaderLevel(Context.View);

	bool bDoUpsample = (InputDesc2 != 0);
	
	SCOPED_DRAW_EVENTF(Context.RHICmdList, AmbientOcclusion, TEXT("AmbientOcclusion%s %dx%d SetupAsInput=%d Upsample=%d ShaderQuality=%d"), 
		(AOType == ESSAOType::EPS) ? TEXT("PS") : TEXT("CS"), ViewRect.Width(), ViewRect.Height(), bAOSetupAsInput, bDoUpsample, ShaderQuality);

	if (AOType == ESSAOType::EPS)
	{
		ProcessPS(Context, DestRenderTarget, ViewRect, TexSize, ShaderQuality, bDoUpsample);
	}
	else
	{
		ProcessCS(Context, DestRenderTarget, ViewRect, TexSize, ShaderQuality, bDoUpsample);
	}	
}

FPooledRenderTargetDesc FRCPassPostProcessAmbientOcclusion::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	if(!bAOSetupAsInput)
	{
		FPooledRenderTargetDesc Ret;

		Ret.DebugName = TEXT("AmbientOcclusionDirect");

		// we render directly to the buffer, no need for an intermediate target, we output in a single channel
		return Ret;
	}

	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.Reset();
	// R:AmbientOcclusion, GBA:used for normal
	Ret.Format = PF_B8G8R8A8;
	Ret.TargetableFlags &= ~TexCreate_DepthStencilTargetable;
	if((AOType == ESSAOType::ECS) || (AOType == ESSAOType::EAsyncCS))
	{
		Ret.TargetableFlags |= TexCreate_UAV;
		// UAV allowed format
		Ret.Format = PF_FloatRGBA;
	}
	else
	{
		Ret.TargetableFlags |= TexCreate_RenderTargetable;
	}
	Ret.DebugName = TEXT("AmbientOcclusion");

	return Ret;
}

// --------------------------------------------------------


/** Encapsulates the post processing ambient occlusion pixel shader. */
class FPostProcessBasePassAOPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessBasePassAOPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FPostProcessBasePassAOPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FScreenSpaceAOParameters ScreenSpaceAOParams;

	/** Initialization constructor. */
	FPostProcessBasePassAOPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		ScreenSpaceAOParams.Bind(Initializer.ParameterMap);
	}

	template <typename TRHICmdList>
	void SetParameters(TRHICmdList& RHICmdList, const FRenderingCompositePassContext& Context, FIntPoint InputTextureSize)
	{
		const FFinalPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);
		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View, MD_PostProcess);
		ScreenSpaceAOParams.Set(RHICmdList, Context.View, ShaderRHI, InputTextureSize);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << ScreenSpaceAOParams;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessBasePassAOPS,TEXT("/Engine/Private/PostProcessAmbientOcclusion.usf"),TEXT("BasePassAOPS"),SF_Pixel);

// --------------------------------------------------------

void FRCPassPostProcessBasePassAO::Process(FRenderingCompositePassContext& Context)
{
	const FViewInfo& View = Context.View;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, ApplyAOToBasePassSceneColor, TEXT("ApplyAOToBasePassSceneColor %dx%d"), View.ViewRect.Width(), View.ViewRect.Height());

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

	const FSceneRenderTargetItem& DestRenderTarget = SceneContext.GetSceneColor()->GetRenderTargetItem();

	// Set the view family's render target/viewport.
	Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, DestRenderTarget.TargetableTexture);
	SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture,	FTextureRHIParamRef(), ESimpleRenderTargetMode::EExistingColorAndDepth);
	Context.SetViewportAndCallRHI(View.ViewRect);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	// set the state
	GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_DestColor, BF_Zero, BO_Add, BF_DestAlpha, BF_Zero>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
	TShaderMapRef<FPostProcessBasePassAOPS> PixelShader(Context.GetShaderMap());

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);
	
	VertexShader->SetParameters(Context);
	PixelShader->SetParameters(Context.RHICmdList, Context, SceneContext.GetBufferSizeXY());

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

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessBasePassAO::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	// we assume this pass is additively blended with the scene color so this data is not needed
	FPooledRenderTargetDesc Ret;

	Ret.DebugName = TEXT("SceneColorWithAO");

	return Ret;
}
