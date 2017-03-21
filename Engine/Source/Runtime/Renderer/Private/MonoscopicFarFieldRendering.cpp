// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MonoscopicFarFieldRendering.cpp: Monoscopic far field rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "PipelineStateCache.h"

/** Pixel shader to composite the monoscopic view into the stereo views. */
class FCompositeMonoscopicFarFieldViewPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCompositeMonoscopicFarFieldViewPS, Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	FCompositeMonoscopicFarFieldViewPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
		MonoColorTextureParameter.Bind(Initializer.ParameterMap, TEXT("MonoColorTexture"));
		MonoColorTextureParameterSampler.Bind(Initializer.ParameterMap, TEXT("MonoColorTextureSampler"));
	}

	FCompositeMonoscopicFarFieldViewPS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, GetPixelShader(), View.ViewUniformBuffer);
		const FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
		const FSamplerStateRHIRef Filter = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		SetTextureParameter(RHICmdList, GetPixelShader(), MonoColorTextureParameter, MonoColorTextureParameterSampler, Filter, SceneContext.GetSceneMonoColorTexture());
		SceneTextureParameters.Set(RHICmdList, GetPixelShader(), View);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		const bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << MonoColorTextureParameter;
		Ar << MonoColorTextureParameterSampler;
		Ar << SceneTextureParameters;
		return bShaderHasOutdatedParameters;
	}

	FShaderResourceParameter MonoColorTextureParameter;
	FShaderResourceParameter MonoColorTextureParameterSampler;
	FSceneTextureShaderParameters SceneTextureParameters;
};

IMPLEMENT_SHADER_TYPE(, FCompositeMonoscopicFarFieldViewPS, TEXT("MonoscopicFarFieldRendering"), TEXT("CompositeMonoscopicFarFieldView"), SF_Pixel);

/**
	Pixel Shader to mask the monoscopic far field view's depth buffer where pixels were rendered into the stereo views.  
	This ensures we only render pixels in the monoscopic far field view that will be visible.
*/
class FMonoscopicFarFieldMaskPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMonoscopicFarFieldMaskPS, Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	FMonoscopicFarFieldMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
		MonoColorTextureParameter.Bind(Initializer.ParameterMap, TEXT("MonoColorTexture"));
		MonoColorTextureParameterSampler.Bind(Initializer.ParameterMap, TEXT("MonoColorTextureSampler"));
		LeftViewWidthNDCParameter.Bind(Initializer.ParameterMap, TEXT("LeftViewWidthNDC"));
		LateralOffsetNDCParameter.Bind(Initializer.ParameterMap, TEXT("LateralOffsetNDC"));
	}

	FMonoscopicFarFieldMaskPS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, const float LeftViewWidthNDC, const float LateralOffsetNDC)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, GetPixelShader(), View.ViewUniformBuffer);
		const FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

		const FSamplerStateRHIRef Filter = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

		const FTextureRHIParamRef SceneColor = (View.Family->bUseSeparateRenderTarget) ? static_cast<FTextureRHIRef>(View.Family->RenderTarget->GetRenderTargetTexture()) : SceneContext.GetSceneColorTexture();
		SetTextureParameter(RHICmdList, GetPixelShader(), MonoColorTextureParameter, MonoColorTextureParameterSampler, Filter, SceneColor);
		
		SetShaderValue(RHICmdList, GetPixelShader(), LeftViewWidthNDCParameter, LeftViewWidthNDC);
		SetShaderValue(RHICmdList, GetPixelShader(), LateralOffsetNDCParameter, LateralOffsetNDC);

		SceneTextureParameters.Set(RHICmdList, GetPixelShader(), View);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		const bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << MonoColorTextureParameter;
		Ar << MonoColorTextureParameterSampler;
		Ar << LeftViewWidthNDCParameter;
		Ar << LateralOffsetNDCParameter;
		Ar << SceneTextureParameters;
		return bShaderHasOutdatedParameters;
	}

	FShaderResourceParameter MonoColorTextureParameter;
	FShaderResourceParameter MonoColorTextureParameterSampler;

	FSceneTextureShaderParameters SceneTextureParameters;
	FShaderParameter LeftViewWidthNDCParameter;
	FShaderParameter LateralOffsetNDCParameter;
};

IMPLEMENT_SHADER_TYPE(, FMonoscopicFarFieldMaskPS, TEXT("MonoscopicFarFieldRendering"), TEXT("MonoscopicFarFieldMask"), SF_Pixel);

void FSceneRenderer::RenderMonoscopicFarFieldMask(FRHICommandListImmediate& RHICmdList)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	SceneContext.BeginRenderingSceneMonoColor(RHICmdList, ESimpleRenderTargetMode::EClearColorAndDepth);

	const FViewInfo& LeftView = Views[0];
	const FViewInfo& RightView = Views[1];
	const FViewInfo& MonoView = Views[2];

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<true, CF_Always>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
	RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

	TShaderMapRef<FScreenVS> VertexShader(MonoView.ShaderMap);
	TShaderMapRef<FMonoscopicFarFieldMaskPS> PixelShader(MonoView.ShaderMap);

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	const float LeftViewWidthNDC = static_cast<float>(RightView.ViewRect.Min.X - LeftView.ViewRect.Min.X) / static_cast<float>(SceneContext.GetBufferSizeXY().X);
	const float LateralOffsetInPixels = roundf(ViewFamily.MonoParameters.LateralOffset * static_cast<float>(MonoView.ViewRect.Width()));
	const float LateralOffsetNDC = LateralOffsetInPixels / static_cast<float>(SceneContext.GetBufferSizeXY().X);

	PixelShader->SetParameters(RHICmdList, MonoView, LeftViewWidthNDC, LateralOffsetNDC);

	RHICmdList.SetViewport(
		MonoView.ViewRect.Min.X, 
		MonoView.ViewRect.Min.Y, 
		1.0, 
		MonoView.ViewRect.Max.X, 
		MonoView.ViewRect.Max.Y, 
		1.0);

	DrawRectangle(
		RHICmdList,
		0, 0,
		MonoView.ViewRect.Width(), MonoView.ViewRect.Height(),
		LeftView.ViewRect.Min.X, LeftView.ViewRect.Min.Y,
		LeftView.ViewRect.Width(), LeftView.ViewRect.Height(),
		FIntPoint(MonoView.ViewRect.Width(), MonoView.ViewRect.Height()),
		SceneContext.GetBufferSizeXY(),
		*VertexShader,
		EDRF_UseTriangleOptimization);
}

void FSceneRenderer::CompositeMonoscopicFarField(FRHICommandListImmediate& RHICmdList)
{
	if (ViewFamily.MonoParameters.Mode == EMonoscopicFarFieldMode::On || ViewFamily.MonoParameters.Mode == EMonoscopicFarFieldMode::MonoOnly)
	{
		const FViewInfo& LeftView = Views[0];
		const FViewInfo& RightView = Views[1];
		const FViewInfo& MonoView = Views[2];

		const FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
		const FTextureRHIParamRef SceneColor = (ViewFamily.bUseSeparateRenderTarget) ? static_cast<FTextureRHIRef>(ViewFamily.RenderTarget->GetRenderTargetTexture()) : SceneContext.GetSceneColorTexture();
		SetRenderTarget(RHICmdList, SceneColor, SceneContext.GetSceneDepthTexture(), ESimpleRenderTargetMode::EExistingColorAndDepth);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

		if (ViewFamily.MonoParameters.Mode == EMonoscopicFarFieldMode::MonoOnly)
		{
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_Zero>::GetRHI();
		}
		else
		{
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_InverseDestAlpha, BF_One>::GetRHI();
		}

		RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

		TShaderMapRef<FScreenVS> VertexShader(MonoView.ShaderMap);
		TShaderMapRef<FCompositeMonoscopicFarFieldViewPS> PixelShader(MonoView.ShaderMap);

		extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		PixelShader->SetParameters(RHICmdList, MonoView);

		const int32 LateralOffsetInPixels = static_cast<int32>(roundf(ViewFamily.MonoParameters.LateralOffset * static_cast<float>(MonoView.ViewRect.Width())));

		// Composite into left eye
		RHICmdList.SetViewport(LeftView.ViewRect.Min.X, LeftView.ViewRect.Min.Y, 0.0f, LeftView.ViewRect.Max.X, LeftView.ViewRect.Max.Y, 1.0f);
		DrawRectangle(
			RHICmdList,
			0, 0,
			LeftView.ViewRect.Width(), LeftView.ViewRect.Height(),
			MonoView.ViewRect.Min.X + LateralOffsetInPixels, MonoView.ViewRect.Min.Y,
			LeftView.ViewRect.Width(), LeftView.ViewRect.Height(),
			LeftView.ViewRect.Size(),
			MonoView.ViewRect.Max,
			*VertexShader,
			EDRF_UseTriangleOptimization);

		// Composite into right eye
		RHICmdList.SetViewport(RightView.ViewRect.Min.X, RightView.ViewRect.Min.Y, 0.0f, RightView.ViewRect.Max.X, RightView.ViewRect.Max.Y, 1.0f);
		DrawRectangle(
			RHICmdList,
			0, 0,
			LeftView.ViewRect.Width(), LeftView.ViewRect.Height(),
			MonoView.ViewRect.Min.X - LateralOffsetInPixels, MonoView.ViewRect.Min.Y,
			LeftView.ViewRect.Width(), LeftView.ViewRect.Height(),
			LeftView.ViewRect.Size(),
			MonoView.ViewRect.Max,
			*VertexShader,
			EDRF_UseTriangleOptimization);
	}

	// Remove the mono view before moving to post.
	Views.RemoveAt(2);
	ViewFamily.Views.RemoveAt(2);
}
