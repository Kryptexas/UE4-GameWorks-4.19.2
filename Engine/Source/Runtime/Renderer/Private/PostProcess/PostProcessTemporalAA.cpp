// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessTemporalAA.cpp: Post process MotionBlur implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessTemporalAA.h"
#include "PostProcessAmbientOcclusion.h"
#include "PostProcessTonemap.h"
#include "PostProcessing.h"

static float TemporalHalton( int32 Index, int32 Base )
{
	float Result = 0.0f;
	float InvBase = 1.0f / Base;
	float Fraction = InvBase;
	while( Index > 0 )
	{
		Result += ( Index % Base ) * Fraction;
		Index /= Base;
		Fraction *= InvBase;
	}
	return Result;
}

static void TemporalRandom(FVector2D* RESTRICT const Constant, uint32 FrameNumber)
{
	Constant->X = TemporalHalton(FrameNumber & 1023, 2);
	Constant->Y = TemporalHalton(FrameNumber & 1023, 3);
}

static TAutoConsoleVariable<float> CVarTemporalAASharpness(
	TEXT("r.TemporalAASharpness"),
	0.0f,
	TEXT("Sharpness of temporal AA (0.0 = smoother, 1.0 = sharper)."),
	ECVF_Scalability | ECVF_RenderThreadSafe);

/** Encapsulates a TemporalAA pixel shader. */
template< uint32 Type, uint32 Responsive >
class FPostProcessTemporalAAPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessTemporalAAPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		FDeferredPixelShaderParameters::ModifyCompilationEnvironment(Platform,OutEnvironment);

		OutEnvironment.SetDefine( TEXT("RESPONSIVE"), Responsive );
	}

	/** Default constructor. */
	FPostProcessTemporalAAPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter SampleWeights;
	FShaderParameter LowpassWeights;
	FCameraMotionParameters CameraMotionParams;
	FShaderParameter RandomOffset;

	/** Initialization constructor. */
	FPostProcessTemporalAAPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		SampleWeights.Bind(Initializer.ParameterMap, TEXT("SampleWeights"));
		LowpassWeights.Bind(Initializer.ParameterMap, TEXT("LowpassWeights"));
		CameraMotionParams.Bind(Initializer.ParameterMap);
		RandomOffset.Bind(Initializer.ParameterMap, TEXT("RandomOffset"));
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << SampleWeights << LowpassWeights << CameraMotionParams << RandomOffset;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FRHICommandList* RHICmdList, const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, Context.View);
		
		FSamplerStateRHIParamRef FilterTable[4];
		FilterTable[0] = TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		FilterTable[1] = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		FilterTable[2] = FilterTable[0];
		FilterTable[3] = FilterTable[0];

		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, 0, false, FilterTable);

		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View);

		FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

		float JitterX = Context.View.TemporalJitterPixelsX *  0.5f;
		float JitterY = Context.View.TemporalJitterPixelsY * -0.5f;

		{
			const FPooledRenderTargetDesc* InputDesc = Context.Pass->GetInputDesc(ePId_Input0);
			const FIntPoint ViewportSize = Context.GetViewport().Size();
			const float Width = ViewportSize.X;
			const float Height = ViewportSize.Y;

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
		
			float Sharpness = CVarTemporalAASharpness.GetValueOnRenderThread();
			// With the new temporal AA, need to hardcode a value here.
			// Going to remove this once the new temporal AA is validated.
			Sharpness = -0.25f;
			Sharpness = 1.0f + Sharpness * 0.5f;

			float Weights[9];
			float WeightsLow[9];
			float TotalWeight = 0.0f;
			float TotalWeightLow = 0.0f;
			for( int32 i = 0; i < 9; i++ )
			{
				// Exponential fit to Blackman-Harris 3.3
				float PixelOffsetX = SampleOffsets[i][0] - JitterX;
				float PixelOffsetY = SampleOffsets[i][1] - JitterY;
				PixelOffsetX *= Sharpness;
				PixelOffsetY *= Sharpness;
				Weights[i] = FMath::Exp( -2.29f * ( PixelOffsetX * PixelOffsetX + PixelOffsetY * PixelOffsetY ) );
				TotalWeight += Weights[i];

				// Lowpass.
				PixelOffsetX = SampleOffsets[i][0] - JitterX;
				PixelOffsetY = SampleOffsets[i][1] - JitterY;
				PixelOffsetX *= 0.25f;
				PixelOffsetY *= 0.25f;
				PixelOffsetX *= Sharpness;
				PixelOffsetY *= Sharpness;
				WeightsLow[i] = FMath::Exp( -2.29f * ( PixelOffsetX * PixelOffsetX + PixelOffsetY * PixelOffsetY ) );
				TotalWeightLow += WeightsLow[i];
			}
			
			for( int32 i = 0; i < 9; i++ )
			{
				SetShaderValue(RHICmdList, ShaderRHI, SampleWeights, Weights[i] / TotalWeight, i );
				SetShaderValue(RHICmdList, ShaderRHI, LowpassWeights, WeightsLow[i] / TotalWeightLow, i );
			}
			
			FVector2D RandomOffsetValue;
			TemporalRandom(&RandomOffsetValue, Context.View.FrameNumber);
			SetShaderValue(RHICmdList, ShaderRHI, RandomOffset, RandomOffsetValue);

		}

		CameraMotionParams.Set(RHICmdList, Context.View, ShaderRHI);
	}
};

// Typedef is necessary because the C preprocessor thinks the comma in the template parameter list is a comma in the macro parameter list.
#define IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(A, B, EntryName) \
	typedef FPostProcessTemporalAAPS<A,B> FPostProcessTemporalAAPS##A##B; \
	IMPLEMENT_SHADER_TYPE(template<>,FPostProcessTemporalAAPS##A##B,TEXT("PostProcessTemporalAA"),EntryName,SF_Pixel);

IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(0, 0, TEXT("DOFTemporalAAPS"));
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(1, 0, TEXT("MainTemporalAAPS"));
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(1, 1, TEXT("MainTemporalAAPS"));
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(2, 0, TEXT("SSRTemporalAAPS"));
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(3, 0, TEXT("LightShaftTemporalAAPS"));
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(4, 0, TEXT("MainFastTemporalAAPS"));
IMPLEMENT_TEMPORALAA_PIXELSHADER_TYPE(4, 1, TEXT("MainFastTemporalAAPS"));

void FRCPassPostProcessSSRTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(SSRTemporalAA, DEC_SCENE_ITEMS);

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	FViewInfo& View = const_cast< FViewInfo& >( Context.View );

	FIntPoint TexSize = InputDesc->Extent;

	// we assume the input and output is full resolution

	FIntPoint SrcSize = InputDesc->Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = GSceneRenderTargets.GetBufferSizeXY().X / SrcSize.X;

	FIntRect SrcRect = View.ViewRect / ScaleFactor;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());

	// is optimized away if possible (RT size=view size, )
	RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, SrcRect);

	Context.SetViewportAndCallRHI(SrcRect);

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef< FPostProcessTonemapVS >			VertexShader( GetGlobalShaderMap() );
	TShaderMapRef< FPostProcessTemporalAAPS<2,0> >	PixelShader( GetGlobalShaderMap() );

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	//@todo-rco: RHIPacketList
	FRHICommandList* RHICmdList = nullptr;
	VertexShader->SetVS(RHICmdList, Context);
	PixelShader->SetParameters(RHICmdList, Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y, 
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Size(),
		SrcSize,
		*VertexShader,
		EDRF_UseTriangleOptimization);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessSSRTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.DebugName = TEXT("SSRTemporalAA");

	return Ret;
}

void FRCPassPostProcessDOFTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(DOFTemporalAA, DEC_SCENE_ITEMS);

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	FViewInfo& View = const_cast< FViewInfo& >( Context.View );
	FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

	FIntPoint TexSize = InputDesc->Extent;

	// we assume the input and output is full resolution

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = GSceneRenderTargets.GetBufferSizeXY().X / SrcSize.X;

	FIntRect SrcRect = View.ViewRect / ScaleFactor;
	FIntRect DestRect = SrcRect;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());

	// is optimized away if possible (RT size=view size, )
	RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, SrcRect);

	Context.SetViewportAndCallRHI(SrcRect);

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef< FPostProcessTonemapVS >			VertexShader( GetGlobalShaderMap() );
	TShaderMapRef< FPostProcessTemporalAAPS<0,0> >	PixelShader( GetGlobalShaderMap() );

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	//@todo-rco: RHIPacketList
	FRHICommandList* RHICmdList = nullptr;
	VertexShader->SetVS(RHICmdList, Context);
	PixelShader->SetParameters(RHICmdList, Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y, 
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Size(),
		SrcSize,
		*VertexShader,
		EDRF_UseTriangleOptimization);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
	ViewState->DOFHistoryRT = PassOutputs[0].PooledRenderTarget;
	check( ViewState->DOFHistoryRT );
}

FPooledRenderTargetDesc FRCPassPostProcessDOFTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.DebugName = TEXT("BokehDOFTemporalAA");

	return Ret;
}


void FRCPassPostProcessLightShaftTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	SCOPED_DRAW_EVENT(LSTemporalAA, DEC_SCENE_ITEMS);

	FViewInfo& View = const_cast< FViewInfo& >( Context.View );
	FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

	FIntPoint TexSize = InputDesc->Extent;

	// we assume the input and output is full resolution

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = GSceneRenderTargets.GetBufferSizeXY().X / SrcSize.X;

	FIntRect SrcRect = View.ViewRect / ScaleFactor;
	FIntRect DestRect = SrcRect;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());

	// is optimized away if possible (RT size=view size, )
	RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, SrcRect);

	Context.SetViewportAndCallRHI(SrcRect);

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef< FPostProcessTonemapVS >			VertexShader( GetGlobalShaderMap() );
	TShaderMapRef< FPostProcessTemporalAAPS<3,0> >	PixelShader( GetGlobalShaderMap() );

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	//@todo-rco: RHIPacketList
	FRHICommandList* RHICmdList = nullptr;
	VertexShader->SetVS(RHICmdList, Context);
	PixelShader->SetParameters(RHICmdList, Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y, 
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Size(),
		SrcSize,
		*VertexShader,
		EDRF_UseTriangleOptimization);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessLightShaftTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.DebugName = TEXT("LightShaftTemporalAA");

	return Ret;
}


void FRCPassPostProcessTemporalAA::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(TemporalAA, DEC_SCENE_ITEMS);

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	FViewInfo& View = const_cast< FViewInfo& >( Context.View );
	FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

	FIntPoint TexSize = InputDesc->Extent;

	// we assume the input and output is full resolution

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = GSceneRenderTargets.GetBufferSizeXY().X / SrcSize.X;

	FIntRect SrcRect = View.ViewRect / ScaleFactor;
	FIntRect DestRect = SrcRect;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	//@todo-rco: RHIPacketList
	FRHICommandList* RHICmdList = nullptr;
	//RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, GSceneRenderTargets.GetSceneDepthTexture());

	// is optimized away if possible (RT size=view size, )
	RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, SrcRect);

	Context.SetViewportAndCallRHI(SrcRect);


	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PostProcessAAQuality")); 
	uint32 Quality = FMath::Clamp(CVar->GetValueOnRenderThread(), 1, 6);
	bool bUseFast = Quality == 3;


	{	
		// Normal temporal feedback
		// Draw to pixels where stencil == 0
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always,true,CF_Equal,SO_Keep,SO_Keep,SO_Keep>::GetRHI(), 0);

		TShaderMapRef< FPostProcessTonemapVS >			VertexShader(GetGlobalShaderMap());
		if (bUseFast)
		{
			TShaderMapRef< FPostProcessTemporalAAPS<4,0> >	PixelShader( GetGlobalShaderMap() );

			static FGlobalBoundShaderState BoundShaderState;

			SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			VertexShader->SetVS(RHICmdList, Context);
			PixelShader->SetParameters(RHICmdList, Context);
		}
		else
		{
			TShaderMapRef< FPostProcessTemporalAAPS<1,0> >	PixelShader( GetGlobalShaderMap() );

			static FGlobalBoundShaderState BoundShaderState;

			SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			VertexShader->SetVS(RHICmdList, Context);
			PixelShader->SetParameters(RHICmdList, Context);
		}
	
		// Draw a quad mapping scene color to the view's render target
		DrawRectangle(
			0, 0,
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Min.X, SrcRect.Min.Y, 
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Size(),
			SrcSize,
			*VertexShader,
			EDRF_UseTriangleOptimization);
	}

	{	// Responsive feedback for tagged pixels
		// Draw to pixels where stencil != 0
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always,true,CF_NotEqual,SO_Keep,SO_Keep,SO_Keep>::GetRHI(), 0);
		
		TShaderMapRef< FPostProcessTonemapVS >			VertexShader( GetGlobalShaderMap() );
		if(bUseFast)
		{
			TShaderMapRef< FPostProcessTemporalAAPS<4,1> >	PixelShader( GetGlobalShaderMap() );

			static FGlobalBoundShaderState BoundShaderState;

			SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			VertexShader->SetVS(RHICmdList, Context);
			PixelShader->SetParameters(RHICmdList, Context);
		}
		else
		{
			TShaderMapRef< FPostProcessTemporalAAPS<1,1> >	PixelShader( GetGlobalShaderMap() );

			static FGlobalBoundShaderState BoundShaderState;

			SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			VertexShader->SetVS(RHICmdList, Context);
			PixelShader->SetParameters(RHICmdList, Context);
		}

		// Draw a quad mapping scene color to the view's render target
		DrawRectangle(
			0, 0,
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Min.X, SrcRect.Min.Y, 
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Size(),
			SrcSize,
			*VertexShader,
			EDRF_UseTriangleOptimization);
	}

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
	ViewState->TemporalAAHistoryRT = PassOutputs[0].PooledRenderTarget;
	check( ViewState->TemporalAAHistoryRT );


	// TODO draw separate translucency after jitter has been removed

	// Remove jitter
	View.ViewMatrices.ProjMatrix.M[2][0] = 0.0f;
	View.ViewMatrices.ProjMatrix.M[2][1] = 0.0f;

	// Compute the view projection matrix and its inverse.
	View.ViewProjectionMatrix = View.ViewMatrices.ViewMatrix * View.ViewMatrices.ProjMatrix;
	View.InvViewProjectionMatrix = View.ViewMatrices.GetInvProjMatrix() * View.InvViewMatrix;

	/** The view transform, starting from world-space points translated by -ViewOrigin. */
	FMatrix TranslatedViewMatrix = FTranslationMatrix(-View.ViewMatrices.PreViewTranslation) * View.ViewMatrices.ViewMatrix;

	// Compute a transform from view origin centered world-space to clip space.
	View.ViewMatrices.TranslatedViewProjectionMatrix = TranslatedViewMatrix * View.ViewMatrices.ProjMatrix;
	View.ViewMatrices.InvTranslatedViewProjectionMatrix = View.ViewMatrices.TranslatedViewProjectionMatrix.InverseSafe();

	View.InitRHIResources();
}

FPooledRenderTargetDesc FRCPassPostProcessTemporalAA::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.Reset();
	Ret.Format = PF_FloatRGBA;
	Ret.DebugName = TEXT("TemporalAA");

	return Ret;
}
