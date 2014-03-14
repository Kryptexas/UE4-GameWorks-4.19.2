// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenSpaceReflections.cpp: Post processing Screen Space Reflections implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessing.h"
#include "ScreenSpaceReflections.h"
#include "PostprocessTemporalAA.h"
#include "PostprocessAmbientOcclusion.h"

static TAutoConsoleVariable<int32> CVarSSRQuality(
	TEXT("r.SSR.Quality"),
	2,
	TEXT("Whether to use screen space reflections and at what quality setting.\n")
	TEXT("(limits the setting in the post process settings which has a different scale)\n")
	TEXT("(costs performance, adds more visual realism but the technique has limits)\n")
	TEXT(" 0: off (default)\n")
	TEXT(" 1: low (no glossy)\n")
	TEXT(" 2: high (glossy/using roughness, few samples)\n")
	TEXT(" 3: very high (likely too slow for real-time)\n")
	TEXT(" 4: reference (too slow for real-time)"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarSSRTemporal(
	TEXT("r.SSR.Temporal"),
	1,
	TEXT("Defines if we use the temporal smoothing for the screen space reflection\n")
	TEXT(" 0 is off (for debugging), 1 is on (default)"),
	ECVF_RenderThreadSafe);

bool DoScreenSpaceReflections(const FViewInfo& View)
{
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

	if(GEngine->IsStereoscopic3D())
	{
		// todo: revisit that later
		return false;
	}

	if(View.FinalPostProcessSettings.ScreenSpaceReflectionIntensity < 1.0f)
	{
		return false;
	}

	return true;
}

/** Encapsulates the post processing screen space reflections pixel shader. */
template<uint32 PrevFrame, uint32 SSRQuality >
class FPostProcessScreenSpaceReflectionsPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessScreenSpaceReflectionsPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine( TEXT("PREV_FRAME_COLOR"), PrevFrame );
		OutEnvironment.SetDefine( TEXT("SSR_QUALITY"), SSRQuality );
	}

	/** Default constructor. */
	FPostProcessScreenSpaceReflectionsPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter SSRParams;

	/** Initialization constructor. */
	FPostProcessScreenSpaceReflectionsPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		SSRParams.Bind(Initializer.ParameterMap, TEXT("SSRParams"));
	}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		const FFinalPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, Context.View);

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		DeferredParameters.Set(ShaderRHI, Context.View);

		{
			float MaxRoughness = FMath::Clamp(Context.View.FinalPostProcessSettings.ScreenSpaceReflectionMaxRoughness, 0.01f, 1.0f);
			float RoughnessScale = FMath::Clamp(Context.View.FinalPostProcessSettings.ScreenSpaceReflectionRoughnessScale, 0.01f, 1.0f);

			// f(x) = x * Mul + Add
			// f(MaxRoughness) = 0
			// f(0) = 1
			// -> Add = 1
			float RoughnessMaskMul = -1 / MaxRoughness;

			FLinearColor Value(
				FMath::Clamp(Context.View.FinalPostProcessSettings.ScreenSpaceReflectionIntensity * 0.01f, 0.0f, 1.0f), 
				RoughnessMaskMul,
				RoughnessScale, 
				0);

			SetShaderValue(ShaderRHI, SSRParams, Value);
		}
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << SSRParams;
		return bShaderHasOutdatedParameters;
	}
};

// Typedef is necessary because the C preprocessor thinks the comma in the template parameter list is a comma in the macro parameter list.
#define IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(A, B) \
	typedef FPostProcessScreenSpaceReflectionsPS<A,B> FPostProcessScreenSpaceReflectionsPS##A##B; \
	IMPLEMENT_SHADER_TYPE(template<>,FPostProcessScreenSpaceReflectionsPS##A##B,TEXT("ScreenSpaceReflections"),TEXT("ScreenSpaceReflectionsPS"),SF_Pixel)

IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(0,1);
IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(1,1);
IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(0,2);
IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(1,2);
IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(0,3);
IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(1,3);
IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(0,4);
IMPLEMENT_REFLECTION_PIXELSHADER_TYPE(1,4);

// --------------------------------------------------------


// @param Quality usually in 0..100 range, default is 50
// @return see CVarSSRQuality, never 0
static int32 ComputeSSRQuality(float Quality)
{
	int32 Ret;

	if(Quality >= 50.0f)
	{
		Ret = (Quality >= 75.0f) ? 4 : 3;
	}
	else
	{
		Ret = (Quality >= 25.0f) ? 2 : 1;
	}

	int SSRQualityCVar = FMath::Max(0, CVarSSRQuality.GetValueOnRenderThread());

	return FMath::Min(Ret, SSRQualityCVar);
}

void FRCPassPostProcessScreenSpaceReflections::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(ScreenSpaceReflections, DEC_SCENE_ITEMS);

	const FSceneView& View = Context.View;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());
	RHIClear(true, FLinearColor(0, 0, 0, 0), false, 1.0f, false, 0, FIntRect());
	Context.SetViewportAndCallRHI(View.ViewRect);

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	int SSRQuality = ComputeSSRQuality(View.FinalPostProcessSettings.ScreenSpaceReflectionQuality);
	
	if(SSRQuality > 1 && View.FinalPostProcessSettings.ScreenSpaceReflectionRoughnessScale <= 0.01f)
	{
		// no glossy SSR needed
		SSRQuality = 1;
	}

	SSRQuality = FMath::Clamp(SSRQuality, 1, 4);

	#define CASE(A, B) \
		case (A + 2 * (B + 3 * 0 )): \
		{ \
			TShaderMapRef< FPostProcessVS > VertexShader(GetGlobalShaderMap()); \
			TShaderMapRef< FPostProcessScreenSpaceReflectionsPS<A, B> > PixelShader(GetGlobalShaderMap()); \
			static FGlobalBoundShaderState BoundShaderState; \
			SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader); \
			VertexShader->SetParameters(Context); \
			PixelShader->SetParameters(Context); \
		}; \
		break

	switch( bPrevFrame + 2 * (SSRQuality + 3 * 0 ) )
	{
		CASE(0,1);	CASE(1,1);
		CASE(0,2);	CASE(1,2);
		CASE(0,3);	CASE(1,3);
		CASE(0,4);	CASE(1,4);
	}
	#undef CASE


	// Draw a quad mapping scene color to the view's render target
	DrawRectangle( 
		0, 0,
		View.ViewRect.Width(), View.ViewRect.Height(),
		View.ViewRect.Min.X, View.ViewRect.Min.Y, 
		View.ViewRect.Width(), View.ViewRect.Height(),
		View.ViewRect.Size(),
		GSceneRenderTargets.GetBufferSizeXY(),
		EDRF_UseTriangleOptimization);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessScreenSpaceReflections::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret(FPooledRenderTargetDesc::Create2DDesc(GSceneRenderTargets.GetBufferSizeXY(), PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable, false));

	Ret.DebugName = TEXT("ScreenSpaceReflections");

	return Ret;
}

void BuildHZB( const FViewInfo& View );

void ScreenSpaceReflections( const FViewInfo& View, TRefCountPtr<IPooledRenderTarget>& SSROutput )
{
	BuildHZB( View );

	FRenderingCompositePassContext CompositeContext( View );
	FPostprocessContext Context( CompositeContext.Graph, View );

	FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

	FRenderingCompositePass* SceneColorInput = Context.Graph.RegisterPass( new FRCPassPostProcessInput( GSceneRenderTargets.SceneColor ) );
	FRenderingCompositePass* HZBInput = Context.Graph.RegisterPass( new FRCPassPostProcessInput( ViewState->HZB.Texture ) );

	bool bPrevFrame = 0;
	if( ViewState && ViewState->TemporalAAHistoryRT && !Context.View.bCameraCut )
	{
		SceneColorInput = Context.Graph.RegisterPass( new FRCPassPostProcessInput( ViewState->TemporalAAHistoryRT ) );
		bPrevFrame = 1;
	}

	{
		FRenderingCompositePass* TracePass = Context.Graph.RegisterPass( new FRCPassPostProcessScreenSpaceReflections( bPrevFrame ) );
		TracePass->SetInput( ePId_Input0, SceneColorInput );
		TracePass->SetInput( ePId_Input1, HZBInput );

		Context.FinalOutput = FRenderingCompositeOutputRef( TracePass );
	}

	const bool bTemporalFilter = CVarSSRTemporal.GetValueOnRenderThread() != 0;

	if( ViewState && bTemporalFilter )
	{
		{
			FRenderingCompositeOutputRef HistoryInput;
			if( ViewState && ViewState->SSRHistoryRT && !Context.View.bCameraCut )
			{
				HistoryInput = Context.Graph.RegisterPass( new FRCPassPostProcessInput( ViewState->SSRHistoryRT ) );
			}
			else
			{
				// No history, use black
				HistoryInput = Context.Graph.RegisterPass( new FRCPassPostProcessInput( GSystemTextures.BlackDummy ) );
			}

			FRenderingCompositePass* TemporalAAPass = Context.Graph.RegisterPass( new FRCPassPostProcessSSRTemporalAA );
			TemporalAAPass->SetInput( ePId_Input0, Context.FinalOutput );
			TemporalAAPass->SetInput( ePId_Input1, HistoryInput );
			//TemporalAAPass->SetInput( ePId_Input2, VelocityInput );

			Context.FinalOutput = FRenderingCompositeOutputRef( TemporalAAPass );
		}

		if( ViewState )
		{
			FRenderingCompositePass* HistoryOutput = Context.Graph.RegisterPass( new FRCPassPostProcessOutput( &ViewState->SSRHistoryRT ) );
			HistoryOutput->SetInput( ePId_Input0, Context.FinalOutput );

			Context.FinalOutput = FRenderingCompositeOutputRef( HistoryOutput );
		}
	}

	{
		FRenderingCompositePass* ReflectionOutput = Context.Graph.RegisterPass( new FRCPassPostProcessOutput( &SSROutput ) );
		ReflectionOutput->SetInput( ePId_Input0, Context.FinalOutput );

		Context.FinalOutput = FRenderingCompositeOutputRef( ReflectionOutput );
	}

	CompositeContext.Root->AddDependency( Context.FinalOutput );
	CompositeContext.Process(TEXT("ReflectionEnvironments"));
}