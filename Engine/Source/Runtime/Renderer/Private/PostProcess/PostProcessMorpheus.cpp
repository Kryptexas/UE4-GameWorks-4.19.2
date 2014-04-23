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

	/** Default constructor. */
	FPostProcessMorpheusPS()
	{		
	}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	
	// Distortion parameter values	
	FShaderParameter TextureScale;
	FShaderParameter TextureOffset;
	FShaderParameter TextureUVOffset;

	FShaderResourceParameter DistortionTextureParam; 	
	FShaderResourceParameter DistortionTextureSampler; 

	/** Initialization constructor. */
	FPostProcessMorpheusPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);

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
	}


	void SetPS(const FRenderingCompositePassContext& Context, FIntRect SrcRect, FIntPoint SrcBufferSize, EStereoscopicPass StereoPass, FMatrix& QuadTexTransform)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		
		FGlobalShader::SetParameters(ShaderRHI, Context.View);

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		DeferredParameters.Set(ShaderRHI, Context.View);

		{
			check(GEngine->HMDDevice.IsValid());
			TSharedPtr< class IHeadMountedDisplay > HMDDevice = GEngine->HMDDevice;

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
		}
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << TextureScale << TextureOffset << TextureUVOffset << DistortionTextureParam << DistortionTextureSampler;
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

	/** Default constructor. */
	FPostProcessMorpheusVS() {}

	/** to have a similar interface as all other shaders */
	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		FGlobalShader::SetParameters(GetVertexShader(), Context.View);
	}

	void SetParameters(const FSceneView& View)
	{
		FGlobalShader::SetParameters(GetVertexShader(), View);
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

	PixelShader->SetPS(Context, SrcRect, SrcSize, View.StereoPass, QuadTexTransform);

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