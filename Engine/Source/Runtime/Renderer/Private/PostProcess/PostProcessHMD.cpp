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
#include "RHICommandList.h"


/** The filter vertex declaration resource type. */
class FDistortionVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	/** Destructor. */
	virtual ~FDistortionVertexDeclaration() {}

	virtual void InitRHI()
	{
		uint16 Stride = sizeof(FDistortionVertex);
		FVertexDeclarationElementList Elements;
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, Position),VET_Float2,0,Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, TexR), VET_Float2, 1,Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, TexG), VET_Float2, 2,Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, TexB), VET_Float2, 3,Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, Color), VET_Float4, 4,Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** The Distortion vertex declaration. */
TGlobalResource<FDistortionVertexDeclaration> GDistortionVertexDeclaration;

/** Encapsulates the post processing vertex shader. */
class FPostProcessHMDVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessHMDVS, Global);

	// Distortion parameter values
	FShaderParameter EyeToSrcUVScale;
	FShaderParameter EyeToSrcUVOffset;

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	/** Default constructor. */
	FPostProcessHMDVS() {}

public:

	/** Initialization constructor. */
	FPostProcessHMDVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		EyeToSrcUVScale.Bind(Initializer.ParameterMap, TEXT("EyeToSrcUVScale"));
		EyeToSrcUVOffset.Bind(Initializer.ParameterMap, TEXT("EyeToSrcUVOffset"));
	}

	void SetVS(const FRenderingCompositePassContext& Context, EStereoscopicPass StereoPass)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();

		FGlobalShader::SetParameters(Context.RHICmdList, ShaderRHI, Context.View);

		check(GEngine->HMDDevice.IsValid());
		FVector2D EyeToSrcUVScaleValue;
		FVector2D EyeToSrcUVOffsetValue;
		GEngine->HMDDevice->GetEyeRenderParams_RenderThread(StereoPass, EyeToSrcUVScaleValue, EyeToSrcUVOffsetValue);
		SetShaderValue(Context.RHICmdList, ShaderRHI, EyeToSrcUVScale, EyeToSrcUVScaleValue);
		SetShaderValue(Context.RHICmdList, ShaderRHI, EyeToSrcUVOffset, EyeToSrcUVOffsetValue);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << EyeToSrcUVScale << EyeToSrcUVOffset;
		return bShaderHasOutdatedParameters;
	}
};

/** Encapsulates the post processing HMD distortion and correction pixel shader. */
class FPostProcessHMDPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessHMDPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	/** Default constructor. */
	FPostProcessHMDPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;



	/** Initialization constructor. */
	FPostProcessHMDPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);

	}

	void SetPS(const FRenderingCompositePassContext& Context, FIntRect SrcRect, FIntPoint SrcBufferSize, EStereoscopicPass StereoPass, FMatrix& QuadTexTransform)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(Context.RHICmdList, ShaderRHI, Context.View);

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
		DeferredParameters.Set(Context.RHICmdList, ShaderRHI, Context.View);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters;
		return bShaderHasOutdatedParameters;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_TIMEWARP"), uint32(0));
	}
};

IMPLEMENT_SHADER_TYPE(, FPostProcessHMDVS, TEXT("PostProcessHMD"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FPostProcessHMDPS, TEXT("PostProcessHMD"), TEXT("MainPS"), SF_Pixel);



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
	
	const FIntRect SrcRect = View.ViewRect;
	const FIntRect DestRect = View.UnscaledViewRect;
	const FIntPoint SrcSize = InputDesc->Extent;

    const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef());

	Context.SetViewportAndCallRHI(DestRect);
	Context.RHICmdList.Clear(true, FLinearColor::Black, false, 1.0f, false, 0, FIntRect());

	// set the state
	Context.RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	Context.RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	Context.RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

	FMatrix QuadTexTransform = FMatrix::Identity;
	FMatrix QuadPosTransform = FMatrix::Identity;

	check(GEngine->HMDDevice.IsValid());

	{
		TShaderMapRef<FPostProcessHMDVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FPostProcessHMDPS> PixelShader(GetGlobalShaderMap());
		static FGlobalBoundShaderState BoundShaderState;
		Context.RHICmdList.CheckIsNull(); // need new approach for "static FGlobalBoundShaderState" for parallel rendering
		SetGlobalBoundShaderState(Context.RHICmdList, BoundShaderState, GDistortionVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		VertexShader->SetVS(Context, View.StereoPass);
		PixelShader->SetPS(Context, SrcRect, SrcSize, View.StereoPass, QuadTexTransform);
	}
	Context.RHICmdList.CheckIsNull(); // this call may not properly use the cmd list in the context
	GEngine->HMDDevice->DrawDistortionMesh_RenderThread(Context, View, SrcSize);

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessHMD::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassOutputs[0].RenderTargetDesc;

	Ret.Reset();
	Ret.DebugName = TEXT("HMD");

	return Ret;
}
