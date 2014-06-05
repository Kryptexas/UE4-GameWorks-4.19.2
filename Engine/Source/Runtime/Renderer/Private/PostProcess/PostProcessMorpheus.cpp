// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessMorpheus.cpp: Post processing for Sony Morpheus HMD device.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessMorpheus.h"
#include "PostProcessing.h"
#include "PostProcessHistogram.h"
#include "PostProcessEyeAdaptation.h"
#include "IHeadMountedDisplay.h"

DEFINE_LOG_CATEGORY_STATIC(LogMorpheusHMDPostProcess, All, All);

#if HAS_MORPHEUS

/** Encapsulates the post processing HMD distortion and correction pixel shader. */
class FPostProcessMorpheusPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessMorpheusPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		// we must use a run time check for this because the builds the build machines create will have Morpheus defined,
		// but a user will not necessarily have the Morpheus files
		bool bEnableMorpheus = false;
		if (GConfig->GetBool(TEXT("Morpheus.Settings"), TEXT("EnableMorpheus"), bEnableMorpheus, GEngineIni))
		{
			return bEnableMorpheus;
		}
		return false;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
#if NEW_MORPHEUS_DISTORTION
		OutEnvironment.SetDefine(TEXT("NEW_MORPHEUS_DISTORTION"), TEXT("1"));
#endif
	}

	/** Default constructor. */
	FPostProcessMorpheusPS()
	{
#if !NEW_MORPHEUS_DISTORTION
		DistortionTexture = NULL;
#endif
	}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	
#if NEW_MORPHEUS_DISTORTION
	// Distortion parameter values
	FShaderParameter TextureScale;
	FShaderParameter TextureOffset;
	FShaderParameter TextureUVOffset;
#else
	FShaderParameter LensCenter;
	FShaderParameter ScreenCenter;
	FShaderParameter Scale;
	FShaderParameter HMDWarpParam;
	FShaderParameter CAWarpParam;
	const FTexture*		DistortionTexture;
#endif

	FShaderResourceParameter DistortionTextureParam; 
	FShaderResourceParameter DistortionTextureSampler; 

	/** Initialization constructor. */
	FPostProcessMorpheusPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);

#if NEW_MORPHEUS_DISTORTION
		TextureScale.Bind(Initializer.ParameterMap, TEXT("TextureScale"));
		//check(TextureScaleLeft.IsBound());		

		TextureOffset.Bind(Initializer.ParameterMap, TEXT("TextureOffset"));
		//check(TextureOffsetRight.IsBound());

		TextureUVOffset.Bind(Initializer.ParameterMap, TEXT("TextureUVOffset"));
		//check(TextureUVOffset.IsBound());
			
		DistortionTextureParam.Bind(Initializer.ParameterMap, TEXT("DistortionTextureArray"));
		//check(DistortionTextureLeftParam.IsBound());		

		DistortionTextureSampler.Bind(Initializer.ParameterMap, TEXT("DistortionTextureSampler"));
		//check(DistortionTextureSampler.IsBound());		
#else
		LensCenter.Bind(Initializer.ParameterMap, TEXT("LensCenter"));
		ScreenCenter.Bind(Initializer.ParameterMap, TEXT("ScreenCenter"));
		Scale.Bind(Initializer.ParameterMap, TEXT("Scale"));
		HMDWarpParam.Bind(Initializer.ParameterMap, TEXT("HMDWarpParam"));
		CAWarpParam.Bind(Initializer.ParameterMap, TEXT("CAWarpParam"));
	
		DistortionTextureParam.Bind(Initializer.ParameterMap, TEXT("DistortionTexture"));
		check(DistortionTextureParam.IsBound());

		DistortionTextureSampler.Bind(Initializer.ParameterMap, TEXT("DistortionTextureSampler"));
		check(DistortionTextureSampler.IsBound());

		DistortionTexture = NULL;
#endif
	}


	void SetPS(FRHICommandList* RHICmdList, const FRenderingCompositePassContext& Context, FIntRect SrcRect, FIntPoint SrcBufferSize, EStereoscopicPass StereoPass, FMatrix& QuadTexTransform)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, Context.View);

		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View);

		{
			check(GEngine->HMDDevice.IsValid());
			TSharedPtr< class IHeadMountedDisplay > HMDDevice = GEngine->HMDDevice;

#if NEW_MORPHEUS_DISTORTION

			check (StereoPass != eSSP_FULL);
			if (StereoPass == eSSP_LEFT_EYE)
			{
				FTexture* TextureLeft = HMDDevice->GetDistortionTextureLeft();
				SetTextureParameter(ShaderRHI, DistortionTextureParam, DistortionTextureSampler, TextureLeft->SamplerStateRHI, TextureLeft->TextureRHI);
				SetShaderValue(ShaderRHI, TextureScale, HMDDevice->GetTextureScaleLeft());
				SetShaderValue(ShaderRHI, TextureOffset, HMDDevice->GetTextureOffsetLeft());
				SetShaderValue(ShaderRHI, TextureUVOffset, 0.0f);
			}
			else
			{
				FTexture* TextureRight = HMDDevice->GetDistortionTextureRight();
				SetTextureParameter(ShaderRHI, DistortionTextureParam, DistortionTextureSampler, TextureRight->SamplerStateRHI, TextureRight->TextureRHI);
				SetShaderValue(ShaderRHI, TextureScale, HMDDevice->GetTextureScaleRight());
				SetShaderValue(ShaderRHI, TextureOffset, HMDDevice->GetTextureOffsetRight());
				SetShaderValue(ShaderRHI, TextureUVOffset, -0.5f);
			}				
			      
            QuadTexTransform = FMatrix::Identity;            
#else
            const FIntPoint SrcSize = SrcRect.Size();
            const float BufferRatioX = float(SrcSize.X)/float(SrcBufferSize.X);
            const float BufferRatioY = float(SrcSize.Y)/float(SrcBufferSize.Y);
            const float w = float(SrcSize.X)/float(SrcBufferSize.X);
            const float h = float(SrcSize.Y)/float(SrcBufferSize.Y);
            const float x = float(SrcRect.Min.X)/float(SrcBufferSize.X);
            const float y = float(SrcRect.Min.Y)/float(SrcBufferSize.Y);

			float DistortionCenterOffsetX = 0, DistortionCenterOffsetY = 0;
			GEngine->HMDDevice->GetDistortionCenterOffset(DistortionCenterOffsetX, DistortionCenterOffsetY);

			const float XCenterOffset = (StereoPass == eSSP_RIGHT_EYE) ? -DistortionCenterOffsetX : DistortionCenterOffsetX;
			const float YCenterOffset = DistortionCenterOffsetY;

			const float AspectRatio = (float)SrcSize.X / (float)SrcSize.Y;
			const float ScaleFactor = GEngine->HMDDevice->GetDistortionScalingFactor();

			if(DistortionTexture == NULL)
			{
				check(IsInRenderingThread());
				DistortionTexture = GEngine->HMDDevice->GetDistortionTexture();
				check(DistortionTexture != NULL);
			}
			SetTextureParameter(RHICmdList, ShaderRHI, DistortionTextureParam, DistortionTextureSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), DistortionTexture->TextureRHI);

			// Shifts texture coordinates to the center of the distortion function around the center of the lens
			FVector2D ViewLensCenter;
			ViewLensCenter.X = x + (0.5f + XCenterOffset) * w;
			ViewLensCenter.Y = y + (0.5f + YCenterOffset) * h;
			SetShaderValue(RHICmdList, ShaderRHI, LensCenter, ViewLensCenter);

			// Texture coordinate for the center of the half scene texture, used to clamp sampling
			FVector2D ViewScreenCenter;
			ViewScreenCenter.X = x + w * 0.5f;
			ViewScreenCenter.Y = y + h * 0.5f;
			SetShaderValue(RHICmdList, ShaderRHI, ScreenCenter, ViewScreenCenter);
			
			// Rescale output (sample) coordinates back to texture range and increase scale to support rendering outside the the screen
			FVector2D ViewScale;
			ViewScale.X = (w/2) * ScaleFactor;
			ViewScale.Y = (h/2) * ScaleFactor * AspectRatio;
			SetShaderValue(RHICmdList, ShaderRHI, Scale, ViewScale);

            // Distortion coefficients
            FVector4 DistortionValues;
            GEngine->HMDDevice->GetDistortionWarpValues(DistortionValues);
			SetShaderValue(RHICmdList, ShaderRHI, HMDWarpParam, DistortionValues);

			// CNN - Morpheus changes
			// CA correction values
			FVector4 CAValues;
			GEngine->HMDDevice->GetChromaAbCorrectionValues(CAValues);
			SetShaderValue(RHICmdList, ShaderRHI, CAWarpParam, CAValues);

			// Rescale the texture coordinates to [-1,1] unit range and corrects aspect ratio
			FVector2D ViewScaleIn;
            ViewScaleIn.X = (2/w);
            ViewScaleIn.Y = (2/h) / AspectRatio;
            
            QuadTexTransform = FMatrix::Identity;
            QuadTexTransform *= FTranslationMatrix(FVector(-ViewLensCenter.X * SrcBufferSize.X, -ViewLensCenter.Y * SrcBufferSize.Y, 0));
            QuadTexTransform *= FMatrix(FPlane(ViewScaleIn.X,0,0,0), FPlane(0,ViewScaleIn.Y,0,0), FPlane(0,0,0,0), FPlane(0,0,0,1));
#endif
		}
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
#if NEW_MORPHEUS_DISTORTION
		Ar << PostprocessParameter << DeferredParameters << TextureScale << TextureOffset << TextureUVOffset << DistortionTextureParam << DistortionTextureSampler;
#else
		Ar << PostprocessParameter << DeferredParameters << LensCenter << ScreenCenter << Scale << HMDWarpParam << CAWarpParam << DistortionTextureParam << DistortionTextureSampler;
#endif
		return bShaderHasOutdatedParameters;
	}
};

/** Encapsulates the post processing vertex shader. */
class FPostProcessMorpheusVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessMorpheusVS,Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		// we must use a run time check for this because the builds the build machines create will have Morpheus defined,
		// but a user will not necessarily have the Morpheus files
		bool bEnableMorpheus = false;
		if (GConfig->GetBool(TEXT("Morpheus.Settings"), TEXT("EnableMorpheus"), bEnableMorpheus, GEngineIni))
		{
			return bEnableMorpheus;
		}
		return false;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
#if NEW_MORPHEUS_DISTORTION
		OutEnvironment.SetDefine(TEXT("NEW_MORPHEUS_DISTORTION"), TEXT("1"));
#endif
	}

	/** Default constructor. */
	FPostProcessMorpheusVS() {}

	/** to have a similar interface as all other shaders */
	void SetParameters(FRHICommandList* RHICmdList, const FRenderingCompositePassContext& Context)
	{
		FGlobalShader::SetParameters(RHICmdList, GetVertexShader(), Context.View);
	}

	void SetParameters(FRHICommandList* RHICmdList, const FSceneView& View)
	{
		FGlobalShader::SetParameters(RHICmdList, GetVertexShader(), View);
	}

public:

	/** Initialization constructor. */
	FPostProcessMorpheusVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
	}
};

IMPLEMENT_SHADER_TYPE(, FPostProcessMorpheusVS, TEXT("MorpheusInclude"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FPostProcessMorpheusPS,TEXT("MorpheusInclude"),TEXT("MainPS"),SF_Pixel);

void FRCPassPostProcessMorpheus::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessMorpheus, DEC_SCENE_ITEMS);
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

	TShaderMapRef<FPostProcessMorpheusVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FPostProcessMorpheusPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

    FMatrix QuadTexTransform;
    FMatrix QuadPosTransform = FMatrix::Identity;
	//@todo-rco: RHIPacketList
	FRHICommandList* RHICmdList = nullptr;
	PixelShader->SetPS(RHICmdList, Context, SrcRect, SrcSize, View.StereoPass, QuadTexTransform);

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

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessMorpheus::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassOutputs[0].RenderTargetDesc;

	Ret.NumSamples = 1;	// no MSAA
	Ret.DebugName = TEXT("Morpheus");

	return Ret;
}

#endif // HAS_MORPHEUS