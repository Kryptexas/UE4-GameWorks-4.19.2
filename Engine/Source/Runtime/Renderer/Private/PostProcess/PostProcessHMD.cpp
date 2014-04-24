// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessHMD.cpp: Post processing for HMD devices.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "PostProcessHMD.h"
#include "PostProcessing.h"
#include "PostProcessHistogram.h"
#include "PostProcessEyeAdaptation.h"
#include "IHeadMountedDisplay.h"

/** Encapsulates the post processing HMD distortion and correction pixel shader. */
template <bool bChromaAbCorrectionEnabled>
class FPostProcessHMDPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessHMDPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return GetMaxSupportedFeatureLevel(Platform) >= ERHIFeatureLevel::SM3;
	}

	/** Default constructor. */
	FPostProcessHMDPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	
	// Distortion parameter values
	FShaderParameter LensCenter;
	FShaderParameter ScreenCenter;
	FShaderParameter Scale;
	FShaderParameter HMDWarpParam;
	FShaderParameter ChromaAbParam;

	/** Initialization constructor. */
	FPostProcessHMDPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);

		LensCenter.Bind(Initializer.ParameterMap, TEXT("LensCenter"));
		ScreenCenter.Bind(Initializer.ParameterMap, TEXT("ScreenCenter"));
		Scale.Bind(Initializer.ParameterMap, TEXT("Scale"));
		HMDWarpParam.Bind(Initializer.ParameterMap, TEXT("HMDWarpParam"));
		if (bChromaAbCorrectionEnabled)
		{
			ChromaAbParam.Bind(Initializer.ParameterMap, TEXT("ChromaAbParam"));
		}
	}

	void SetPS(const FRenderingCompositePassContext& Context, FIntRect SrcRect, FIntPoint SrcBufferSize, EStereoscopicPass StereoPass, FMatrix& QuadTexTransform)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		
		FGlobalShader::SetParameters(ShaderRHI, Context.View);

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		DeferredParameters.Set(ShaderRHI, Context.View);

		{
			check(GEngine->HMDDevice.IsValid());

            const FIntPoint SrcSize = SrcRect.Size();
            const float BufferRatioX = float(SrcSize.X)/float(SrcBufferSize.X);
            const float BufferRatioY = float(SrcSize.Y)/float(SrcBufferSize.Y);
            const float w = float(SrcSize.X)/float(SrcBufferSize.X);
            const float h = float(SrcSize.Y)/float(SrcBufferSize.Y);
            const float x = float(SrcRect.Min.X)/float(SrcBufferSize.X);
            const float y = float(SrcRect.Min.Y)/float(SrcBufferSize.Y);

            const float AbsoluteLensSeparationDistance = GEngine->HMDDevice->GetLensCenterOffset();
			const float XCenterOffset = (StereoPass == eSSP_RIGHT_EYE) ? -AbsoluteLensSeparationDistance : AbsoluteLensSeparationDistance;

			const float AspectRatio = (float)SrcSize.X / (float)SrcSize.Y;
			const float ScaleFactor = GEngine->HMDDevice->GetDistortionScalingFactor();
	
			// Shifts texture coordinates to the center of the distortion function around the center of the lens
			FVector2D ViewLensCenter;
			ViewLensCenter.X = x + (0.5f + XCenterOffset * 0.5f) * w;
			ViewLensCenter.Y = y + h * 0.5f;
			SetShaderValue(ShaderRHI, LensCenter, ViewLensCenter);

			// Texture coordinate for the center of the half scene texture, used to clamp sampling
			FVector2D ViewScreenCenter;
			ViewScreenCenter.X = x + w * 0.5f;
			ViewScreenCenter.Y = y + h * 0.5f;
			SetShaderValue(ShaderRHI, ScreenCenter, ViewScreenCenter);
			
			// Rescale output (sample) coordinates back to texture range and increase scale to support rendering outside the the screen
			FVector2D ViewScale;
			ViewScale.X = (w/2) * ScaleFactor;
			ViewScale.Y = (h/2) * ScaleFactor * AspectRatio;
            SetShaderValue(ShaderRHI, Scale, ViewScale);

            // Distortion coefficients
            FVector4 DistortionValues;
            GEngine->HMDDevice->GetDistortionWarpValues(DistortionValues);
            SetShaderValue(ShaderRHI, HMDWarpParam, DistortionValues);
			if (bChromaAbCorrectionEnabled)
			{
				FVector4 ChromaAbValues;
				if (GEngine->HMDDevice->GetChromaAbCorrectionValues(ChromaAbValues))
				{
					SetShaderValue(ShaderRHI, ChromaAbParam, ChromaAbValues);
				}
			}

			// Rescale the texture coordinates to [-1,1] unit range and corrects aspect ratio
			FVector2D ViewScaleIn;
            ViewScaleIn.X = (2/w);
            ViewScaleIn.Y = (2/h) / AspectRatio;
            
            QuadTexTransform = FMatrix::Identity;
            QuadTexTransform *= FTranslationMatrix(FVector(-ViewLensCenter.X * SrcBufferSize.X, -ViewLensCenter.Y * SrcBufferSize.Y, 0));
            QuadTexTransform *= FMatrix(FPlane(ViewScaleIn.X,0,0,0), FPlane(0,ViewScaleIn.Y,0,0), FPlane(0,0,0,0), FPlane(0,0,0,1));
		}
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << LensCenter << ScreenCenter << Scale << HMDWarpParam;
		if (bChromaAbCorrectionEnabled)
		{
			Ar << ChromaAbParam;
		}
		return bShaderHasOutdatedParameters;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_CHA_CORRECTION"), uint32(bChromaAbCorrectionEnabled ? 1 : 0));
	}
};

IMPLEMENT_SHADER_TYPE(template<>, FPostProcessHMDPS<true>, TEXT("PostProcessHMD"), TEXT("MainPS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>, FPostProcessHMDPS<false>, TEXT("PostProcessHMD"), TEXT("MainPS"), SF_Pixel);

#if !UE_BUILD_SHIPPING
class FSolidColorPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSolidColorPixelShader, Global);
public:
	FSolidColorPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}
	FSolidColorPixelShader() {}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}
};
IMPLEMENT_SHADER_TYPE(, FSolidColorPixelShader, TEXT("SolidColorPixelShader"), TEXT("Main"), SF_Pixel);

static void DrawLatencyTest_RenderThread(FColor DrawColor)
{
	FFilterVertex Vertices[4];
	
	Vertices[0].Position = FVector4(-0.7f, -0.7f, 0, 1);
	Vertices[1].Position = FVector4( 0.7f, -0.7f, 0, 1);
	Vertices[2].Position = FVector4(-0.7f, 0.7f, 0, 1);
	Vertices[3].Position = FVector4( 0.7f, 0.7f, 0, 1);

	for (int i = 0; i < 4; i++)
	{
		Vertices[i].UV.X = DrawColor.R / 255.f;
		Vertices[i].UV.Y = DrawColor.G / 255.f;
	}

	static const uint16 Indices[] =
	{
		0, 1, 3,
		0, 3, 2
	};

	static FGlobalBoundShaderState BoundShaderState;
	TShaderMapRef<FScreenVS> ScreenVertexShader(GetGlobalShaderMap());
	TShaderMapRef<FSolidColorPixelShader> SolidPixelShader(GetGlobalShaderMap());
	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI,
		*ScreenVertexShader, *SolidPixelShader);
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	RHIDrawIndexedPrimitiveUP(PT_TriangleList, 0, 4, 2, Indices, sizeof(Indices[0]), Vertices, sizeof(Vertices[0]));
}
#endif // #if !UE_BUILD_SHIPPING

void FRCPassPostProcessHMD::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessHMD, DEC_SCENE_ITEMS);
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

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());

	FMatrix QuadTexTransform;
	FMatrix QuadPosTransform = FMatrix::Identity;

	check(GEngine->HMDDevice.IsValid());
	if (GEngine->HMDDevice->IsChromaAbCorrectionEnabled())
	{
		TShaderMapRef<FPostProcessHMDPS<true> > PixelShader(GetGlobalShaderMap());
		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		PixelShader->SetPS(Context, SrcRect, SrcSize, View.StereoPass, QuadTexTransform);
	}
	else
	{
		TShaderMapRef<FPostProcessHMDPS<false> > PixelShader(GetGlobalShaderMap());
		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		PixelShader->SetPS(Context, SrcRect, SrcSize, View.StereoPass, QuadTexTransform);
	}
	// Draw a quad mapping scene color to the view's render target
	DrawTransformedRectangle(
		0, 0,
		DestRect.Width(), DestRect.Height(),
		QuadPosTransform,
		SrcRect.Min.X, SrcRect.Min.Y,
		SrcRect.Width(), SrcRect.Height(),
		QuadTexTransform,
		DestRect.Size(),
		SrcSize
		);

#if !UE_BUILD_SHIPPING
	FColor latencyColor;
	if (GEngine->HMDDevice->GetLatencyTesterColor_RenderThread(latencyColor, Context.View))
	{
		DrawLatencyTest_RenderThread(latencyColor);
	}
#endif

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessHMD::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassOutputs[0].RenderTargetDesc;

	Ret.Reset();
	Ret.DebugName = TEXT("HMD");

	return Ret;
}
