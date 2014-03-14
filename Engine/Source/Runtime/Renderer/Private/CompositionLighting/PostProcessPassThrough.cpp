// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessPassThrough.cpp: Post processing pass through implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessPassThrough.h"
#include "PostProcessing.h"

/** Encapsulates a simple copy pixel shader. */
class FPostProcessPassThroughPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessPassThroughPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	/** Default constructor. */
	FPostProcessPassThroughPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;

	/** Initialization constructor. */
	FPostProcessPassThroughPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, Context.View);

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessPassThroughPS,TEXT("PostProcessPassThrough"),TEXT("MainPS"),SF_Pixel);


FRCPassPostProcessPassThrough::FRCPassPostProcessPassThrough(IPooledRenderTarget* InDest, bool bInAdditiveBlend)
	: Dest(InDest)
	, bAdditiveBlend(bInAdditiveBlend)
{
}

FRCPassPostProcessPassThrough::FRCPassPostProcessPassThrough(FPooledRenderTargetDesc InNewDesc)
	: Dest(0)
	, bAdditiveBlend(false)
	, NewDesc(InNewDesc)
{
}

void FRCPassPostProcessPassThrough::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PassThrough, DEC_SCENE_ITEMS);

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FSceneView& View = Context.View;

	FIntPoint TexSize = InputDesc->Extent;

	// we assume the input and output is full resolution

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = Dest ? Dest->GetDesc().Extent : PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 InputScaleFactor = GSceneRenderTargets.GetBufferSizeXY().X / SrcSize.X;
	uint32 OutputScaleFactor = GSceneRenderTargets.GetBufferSizeXY().X / DestSize.X;

	FIntRect SrcRect = View.ViewRect / InputScaleFactor;
	FIntRect DestRect = View.ViewRect / OutputScaleFactor;

	const FSceneRenderTargetItem& DestRenderTarget = Dest ? Dest->GetRenderTargetItem() : PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());	
	Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);

	// set the state
	if(bAdditiveBlend)
	{
		RHISetBlendState(TStaticBlendState<CW_RGB,BO_Add,BF_One,BF_One,BO_Add,BF_One,BF_One>::GetRHI());
	}
	else
	{
		RHISetBlendState(TStaticBlendState<>::GetRHI());
	}

	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FPostProcessPassThroughPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	VertexShader->SetParameters(Context);
	PixelShader->SetParameters(Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		DestRect.Min.X, DestRect.Min.Y,
		DestRect.Width(), DestRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y,
		SrcRect.Width(), SrcRect.Height(),
		DestSize,
		SrcSize,
		EDRF_UseTriangleOptimization);


	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}


FPooledRenderTargetDesc FRCPassPostProcessPassThrough::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret;

	// we assume this pass is additively blended with the scene color so an intermediate is not always needed
	if(!Dest)
	{
		Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

		if(NewDesc.IsValid())
		{
			Ret = NewDesc;
		}
	}

	Ret.DebugName = TEXT("PassThrough");

	return Ret;
}