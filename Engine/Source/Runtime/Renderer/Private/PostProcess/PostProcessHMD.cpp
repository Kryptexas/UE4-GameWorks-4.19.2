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
		FVertexDeclarationElementList Elements;
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, Position),VET_Float2,0));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, TexR), VET_Float2, 1));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, TexG), VET_Float2, 2));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, TexB), VET_Float2, 3));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FDistortionVertex, Color), VET_Float4, 4));
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
		return !IsOpenGLPlatform(Platform);
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

	void SetVS(FRHICommandList& RHICmdList, const FRenderingCompositePassContext& Context, EStereoscopicPass StereoPass)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, Context.View);

		check(GEngine->HMDDevice.IsValid());
		FVector2D EyeToSrcUVScaleValue;
		FVector2D EyeToSrcUVOffsetValue;
		GEngine->HMDDevice->GetEyeRenderParams_RenderThread(StereoPass, EyeToSrcUVScaleValue, EyeToSrcUVOffsetValue);
		SetShaderValue(RHICmdList, ShaderRHI, EyeToSrcUVScale, EyeToSrcUVScaleValue);
		SetShaderValue(RHICmdList, ShaderRHI, EyeToSrcUVOffset, EyeToSrcUVOffsetValue);
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
		return !IsOpenGLPlatform(Platform);
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

	void SetPS(FRHICommandList& RHICmdList, const FRenderingCompositePassContext& Context, FIntRect SrcRect, FIntPoint SrcBufferSize, EStereoscopicPass StereoPass, FMatrix& QuadTexTransform)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, Context.View);

		PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View);
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
		FDeferredPixelShaderParameters::ModifyCompilationEnvironment(Platform,OutEnvironment);
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

	//@todo-rco: RHIPacketList
	FRHICommandList& RHICmdList = FRHICommandList::GetNullRef();

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());	

	Context.SetViewportAndCallRHI(DestRect);
	RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, FIntRect());

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	FMatrix QuadTexTransform = FMatrix::Identity;
	FMatrix QuadPosTransform = FMatrix::Identity;

	check(GEngine->HMDDevice.IsValid());

	{
		TShaderMapRef<FPostProcessHMDVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FPostProcessHMDPS> PixelShader(GetGlobalShaderMap());
		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(BoundShaderState, GDistortionVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		VertexShader->SetVS(RHICmdList, Context, View.StereoPass);
		PixelShader->SetPS(RHICmdList, Context, SrcRect, SrcSize, View.StereoPass, QuadTexTransform);
	}

	GEngine->HMDDevice->DrawDistortionMesh_RenderThread(Context, View, SrcSize);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessHMD::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassOutputs[0].RenderTargetDesc;

	Ret.Reset();
	Ret.DebugName = TEXT("HMD");

	return Ret;
}
