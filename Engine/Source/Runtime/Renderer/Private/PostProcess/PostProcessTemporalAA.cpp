// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessTemporalAA.cpp: Post process MotionBlur implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessTemporalAA.h"
#include "PostProcessTonemap.h"
#include "PostProcessing.h"

static inline void Inverse4x4( double* dst, const float* src )
{
	const double s0  = (double)(src[ 0]); const double s1  = (double)(src[ 1]); const double s2  = (double)(src[ 2]); const double s3  = (double)(src[ 3]);
	const double s4  = (double)(src[ 4]); const double s5  = (double)(src[ 5]); const double s6  = (double)(src[ 6]); const double s7  = (double)(src[ 7]);
	const double s8  = (double)(src[ 8]); const double s9  = (double)(src[ 9]); const double s10 = (double)(src[10]); const double s11 = (double)(src[11]);
	const double s12 = (double)(src[12]); const double s13 = (double)(src[13]); const double s14 = (double)(src[14]); const double s15 = (double)(src[15]);

	double inv[16];
	inv[0]  =  s5 * s10 * s15 - s5 * s11 * s14 - s9 * s6 * s15 + s9 * s7 * s14 + s13 * s6 * s11 - s13 * s7 * s10;
	inv[1]  = -s1 * s10 * s15 + s1 * s11 * s14 + s9 * s2 * s15 - s9 * s3 * s14 - s13 * s2 * s11 + s13 * s3 * s10;
	inv[2]  =  s1 * s6  * s15 - s1 * s7  * s14 - s5 * s2 * s15 + s5 * s3 * s14 + s13 * s2 * s7  - s13 * s3 * s6;
	inv[3]  = -s1 * s6  * s11 + s1 * s7  * s10 + s5 * s2 * s11 - s5 * s3 * s10 - s9  * s2 * s7  + s9  * s3 * s6;
	inv[4]  = -s4 * s10 * s15 + s4 * s11 * s14 + s8 * s6 * s15 - s8 * s7 * s14 - s12 * s6 * s11 + s12 * s7 * s10;
	inv[5]  =  s0 * s10 * s15 - s0 * s11 * s14 - s8 * s2 * s15 + s8 * s3 * s14 + s12 * s2 * s11 - s12 * s3 * s10;
	inv[6]  = -s0 * s6  * s15 + s0 * s7  * s14 + s4 * s2 * s15 - s4 * s3 * s14 - s12 * s2 * s7  + s12 * s3 * s6;
	inv[7]  =  s0 * s6  * s11 - s0 * s7  * s10 - s4 * s2 * s11 + s4 * s3 * s10 + s8  * s2 * s7  - s8  * s3 * s6;
	inv[8]  =  s4 * s9  * s15 - s4 * s11 * s13 - s8 * s5 * s15 + s8 * s7 * s13 + s12 * s5 * s11 - s12 * s7 * s9;
	inv[9]  = -s0 * s9  * s15 + s0 * s11 * s13 + s8 * s1 * s15 - s8 * s3 * s13 - s12 * s1 * s11 + s12 * s3 * s9;
	inv[10] =  s0 * s5  * s15 - s0 * s7  * s13 - s4 * s1 * s15 + s4 * s3 * s13 + s12 * s1 * s7  - s12 * s3 * s5;
	inv[11] = -s0 * s5  * s11 + s0 * s7  * s9  + s4 * s1 * s11 - s4 * s3 * s9  - s8  * s1 * s7  + s8  * s3 * s5;
	inv[12] = -s4 * s9  * s14 + s4 * s10 * s13 + s8 * s5 * s14 - s8 * s6 * s13 - s12 * s5 * s10 + s12 * s6 * s9;
	inv[13] =  s0 * s9  * s14 - s0 * s10 * s13 - s8 * s1 * s14 + s8 * s2 * s13 + s12 * s1 * s10 - s12 * s2 * s9;
	inv[14] = -s0 * s5  * s14 + s0 * s6  * s13 + s4 * s1 * s14 - s4 * s2 * s13 - s12 * s1 * s6  + s12 * s2 * s5;
	inv[15] =  s0 * s5  * s10 - s0 * s6  * s9  - s4 * s1 * s10 + s4 * s2 * s9  + s8  * s1 * s6  - s8  * s2 * s5;

	double det = s0 * inv[0] + s1 * inv[4] + s2 * inv[8] + s3 * inv[12];
	if( det != 0.0 )
	{
		det = 1.0 / det;
	}
	for( int i = 0; i < 16; i++ )
	{
		dst[i] = inv[i] * det;
	}
}

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

		OutEnvironment.SetDefine( TEXT("RESPONSIVE"), Responsive );
	}

	/** Default constructor. */
	FPostProcessTemporalAAPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter SampleWeights;
	FShaderParameter LowpassWeights;
	FShaderParameter CameraMotion;
	FShaderParameter RandomOffset;

	/** Initialization constructor. */
	FPostProcessTemporalAAPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		SampleWeights.Bind(Initializer.ParameterMap, TEXT("SampleWeights"));
		LowpassWeights.Bind(Initializer.ParameterMap, TEXT("LowpassWeights"));
		CameraMotion.Bind(Initializer.ParameterMap, TEXT("CameraMotion"));
		RandomOffset.Bind(Initializer.ParameterMap, TEXT("RandomOffset"));
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << SampleWeights << LowpassWeights << CameraMotion << RandomOffset;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, Context.View);
		
		FSamplerStateRHIParamRef FilterTable[4];
		FilterTable[0] = TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		FilterTable[1] = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		FilterTable[2] = FilterTable[0];
		FilterTable[3] = FilterTable[0];

		PostprocessParameter.SetPS(ShaderRHI, Context, 0, false, FilterTable);

		DeferredParameters.Set(ShaderRHI, Context.View);

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
				SetShaderValue( ShaderRHI, SampleWeights, Weights[i] / TotalWeight, i );
				SetShaderValue( ShaderRHI, LowpassWeights, WeightsLow[i] / TotalWeightLow, i );
			}
			
			FVector2D RandomOffsetValue;
			TemporalRandom(&RandomOffsetValue, Context.View.FrameNumber);
			SetShaderValue(ShaderRHI, RandomOffset, RandomOffsetValue);

		}

		{
			FMatrix Proj = Context.View.ViewMatrices.ProjMatrix;
			FMatrix PrevProj = ViewState->PrevViewMatrices.ProjMatrix;

			// Remove jitter
			Proj.M[2][0] = 0.0f;
			Proj.M[2][1] = 0.0f;
			PrevProj.M[2][0] = 0.0f;
			PrevProj.M[2][1] = 0.0f;

			FMatrix ViewProj = ( Context.View.ViewMatrices.ViewMatrix * Proj ).GetTransposed();
			FMatrix PrevViewProj = ( ViewState->PrevViewMatrices.ViewMatrix * PrevProj ).GetTransposed();

			double InvViewProj[16];
			Inverse4x4( InvViewProj, (float*)ViewProj.M );

			const float* p = (float*)PrevViewProj.M;

			const double cxx = InvViewProj[ 0]; const double cxy = InvViewProj[ 1]; const double cxz = InvViewProj[ 2]; const double cxw = InvViewProj[ 3];
			const double cyx = InvViewProj[ 4]; const double cyy = InvViewProj[ 5]; const double cyz = InvViewProj[ 6]; const double cyw = InvViewProj[ 7];
			const double czx = InvViewProj[ 8]; const double czy = InvViewProj[ 9]; const double czz = InvViewProj[10]; const double czw = InvViewProj[11];
			const double cwx = InvViewProj[12]; const double cwy = InvViewProj[13]; const double cwz = InvViewProj[14]; const double cww = InvViewProj[15];

			const double pxx = (double)(p[ 0]); const double pxy = (double)(p[ 1]); const double pxz = (double)(p[ 2]); const double pxw = (double)(p[ 3]);
			const double pyx = (double)(p[ 4]); const double pyy = (double)(p[ 5]); const double pyz = (double)(p[ 6]); const double pyw = (double)(p[ 7]);
			const double pwx = (double)(p[12]); const double pwy = (double)(p[13]); const double pwz = (double)(p[14]); const double pww = (double)(p[15]);

			FVector4 Motion;

			Motion[0] = (float)(4.0*(cwx*pww + cxx*pwx + cyx*pwy + czx*pwz));
			Motion[1] = (float)((-4.0)*(cwy*pww + cxy*pwx + cyy*pwy + czy*pwz));
			Motion[2] = (float)(2.0*(cwz*pww + cxz*pwx + cyz*pwy + czz*pwz));
			Motion[3] = (float)(2.0*(cww*pww - cwx*pww + cwy*pww + (cxw - cxx + cxy)*pwx + (cyw - cyx + cyy)*pwy + (czw - czx + czy)*pwz));
			SetShaderValue( ShaderRHI, CameraMotion, Motion, 0 );

			Motion[0] = (float)(( 4.0)*(cwy*pww + cxy*pwx + cyy*pwy + czy*pwz));
			Motion[1] = (float)((-2.0)*(cwz*pww + cxz*pwx + cyz*pwy + czz*pwz));
			Motion[2] = (float)((-2.0)*(cww*pww + cwy*pww + cxw*pwx - 2.0*cxx*pwx + cxy*pwx + cyw*pwy - 2.0*cyx*pwy + cyy*pwy + czw*pwz - 2.0*czx*pwz + czy*pwz - cwx*(2.0*pww + pxw) - cxx*pxx - cyx*pxy - czx*pxz));
			Motion[3] = (float)(-2.0*(cyy*pwy + czy*pwz + cwy*(pww + pxw) + cxy*(pwx + pxx) + cyy*pxy + czy*pxz));
			SetShaderValue( ShaderRHI, CameraMotion, Motion, 1 );

			Motion[0] = (float)((-4.0)*(cwx*pww + cxx*pwx + cyx*pwy + czx*pwz));
			Motion[1] = (float)(cyz*pwy + czz*pwz + cwz*(pww + pxw) + cxz*(pwx + pxx) + cyz*pxy + czz*pxz);
			Motion[2] = (float)(cwy*pww + cwy*pxw + cww*(pww + pxw) - cwx*(pww + pxw) + (cxw - cxx + cxy)*(pwx + pxx) + (cyw - cyx + cyy)*(pwy + pxy) + (czw - czx + czy)*(pwz + pxz));
			Motion[3] = (float)(0);
			SetShaderValue( ShaderRHI, CameraMotion, Motion, 2 );

			Motion[0] = (float)((-4.0)*(cwx*pww + cxx*pwx + cyx*pwy + czx*pwz));
			Motion[1] = (float)((-2.0)*(cwz*pww + cxz*pwx + cyz*pwy + czz*pwz));
			Motion[2] = (float)(2.0*((-cww)*pww + cwx*pww - 2.0*cwy*pww - cxw*pwx + cxx*pwx - 2.0*cxy*pwx - cyw*pwy + cyx*pwy - 2.0*cyy*pwy - czw*pwz + czx*pwz - 2.0*czy*pwz + cwy*pyw + cxy*pyx + cyy*pyy + czy*pyz));
			Motion[3] = (float)(2.0*(cyx*pwy + czx*pwz + cwx*(pww - pyw) + cxx*(pwx - pyx) - cyx*pyy - czx*pyz));
			SetShaderValue( ShaderRHI, CameraMotion, Motion, 3 );

			Motion[0] = (float)(4.0*(cwy*pww + cxy*pwx + cyy*pwy + czy*pwz));
			Motion[1] = (float)(cyz*pwy + czz*pwz + cwz*(pww - pyw) + cxz*(pwx - pyx) - cyz*pyy - czz*pyz);
			Motion[2] = (float)(cwy*pww + cww*(pww - pyw) - cwy*pyw + cwx*((-pww) + pyw) + (cxw - cxx + cxy)*(pwx - pyx) + (cyw - cyx + cyy)*(pwy - pyy) + (czw - czx + czy)*(pwz - pyz));
			Motion[3] = (float)(0);
			SetShaderValue( ShaderRHI, CameraMotion, Motion, 4 );
		}
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

	VertexShader->SetVS(Context);
	PixelShader->SetParameters(Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y, 
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Size(),
		SrcSize,
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

	VertexShader->SetVS(Context);
	PixelShader->SetParameters(Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y, 
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Size(),
		SrcSize,
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

	VertexShader->SetVS(Context);
	PixelShader->SetParameters(Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y, 
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Size(),
		SrcSize,
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

	//RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, GSceneRenderTargets.GetSceneDepthTexture());

	// is optimized away if possible (RT size=view size, )
	RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, SrcRect);

	Context.SetViewportAndCallRHI(SrcRect);

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	//RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
	
	{	// Normal temporal feedback
		// Draw to pixels where stencil == 0
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always,true,CF_Equal,SO_Keep,SO_Keep,SO_Keep>::GetRHI(), 0);
		
		TShaderMapRef< FPostProcessTonemapVS >			VertexShader( GetGlobalShaderMap() );
		TShaderMapRef< FPostProcessTemporalAAPS<1,0> >	PixelShader( GetGlobalShaderMap() );

		static FGlobalBoundShaderState BoundShaderState;

		SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		
		VertexShader->SetVS(Context);
		PixelShader->SetParameters(Context);
		
		// Draw a quad mapping scene color to the view's render target
		DrawRectangle(
			0, 0,
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Min.X, SrcRect.Min.Y, 
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Size(),
			SrcSize,
			EDRF_UseTriangleOptimization);
	}

	{	// Responsive feedback for tagged pixels
		// Draw to pixels where stencil != 0
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always,true,CF_NotEqual,SO_Keep,SO_Keep,SO_Keep>::GetRHI(), 0);

		TShaderMapRef< FPostProcessTonemapVS >			VertexShader( GetGlobalShaderMap() );
		TShaderMapRef< FPostProcessTemporalAAPS<1,1> >	PixelShader( GetGlobalShaderMap() );

		static FGlobalBoundShaderState BoundShaderState;

		SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		
		VertexShader->SetVS(Context);
		PixelShader->SetParameters(Context);
		
		// Draw a quad mapping scene color to the view's render target
		DrawRectangle(
			0, 0,
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Min.X, SrcRect.Min.Y, 
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Size(),
			SrcSize,
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