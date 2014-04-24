// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessAmbient.cpp: Post processing ambient implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessing.h"
#include "PostProcessAmbient.h"

/** Encapsulates the post processing ambient pixel shader. */
class FPostProcessAmbientPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessAmbientPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	/** Default constructor. */
	FPostProcessAmbientPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FCubemapShaderParameters CubemapShaderParameters;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter PreIntegratedGF;
	FShaderResourceParameter PreIntegratedGFSampler;

	/** Initialization constructor. */
	FPostProcessAmbientPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		CubemapShaderParameters.Bind(Initializer.ParameterMap);
		PreIntegratedGF.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGF"));
		PreIntegratedGFSampler.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGFSampler"));
	}

	void SetParameters(const FRenderingCompositePassContext& Context, const FFinalPostProcessSettings::FCubemapEntry& Entry)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, Context.View);
		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		DeferredParameters.Set(ShaderRHI, Context.View);
		CubemapShaderParameters.SetParameters(ShaderRHI, Entry);
		SetTextureParameter( ShaderRHI, PreIntegratedGF, PreIntegratedGFSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), GSystemTextures.PreintegratedGF->GetRenderTargetItem().ShaderResourceTexture );
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << CubemapShaderParameters << DeferredParameters << PreIntegratedGF << PreIntegratedGFSampler;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessAmbientPS,TEXT("PostProcessAmbient"),TEXT("MainPS"),SF_Pixel);

void FRCPassPostProcessAmbient::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessAmbient, DEC_SCENE_ITEMS);

	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	FIntRect SrcRect = View.ViewRect;
	// todo: view size should scale with input texture size so we can do SSAO in half resolution as well
	FIntRect DestRect = View.ViewRect;
	FIntPoint DestSize = DestRect.Size();

	const FSceneRenderTargetItem& DestRenderTarget = GSceneRenderTargets.SceneColor->GetRenderTargetItem();

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());	
	Context.SetViewportAndCallRHI(View.ViewRect);

	// set the state
	RHISetBlendState(TStaticBlendState<CW_RGB,BO_Add,BF_One,BF_One,BO_Add,BF_One,BF_One>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());

	FScene* Scene = ViewFamily.Scene->GetRenderScene();
	check(Scene);
	uint32 NumReflectionCaptures = Scene->ReflectionSceneData.RegisteredReflectionCaptures.Num();
	
	// Ambient cubemap specular will be applied in the reflection environment pass if it is enabled
	const bool bApplySpecular = View.Family->EngineShowFlags.ReflectionEnvironment == 0 || NumReflectionCaptures == 0;
	TShaderMapRef<FPostProcessAmbientPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;

	uint32 Count = Context.View.FinalPostProcessSettings.ContributingCubemaps.Num();
	for(uint32 i = 0; i < Count; ++i)
	{
		if(i == 0)
		{
			// call it once after setting up the shader data to avoid the warnings in the function
			SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		}

		PixelShader->SetParameters(Context, Context.View.FinalPostProcessSettings.ContributingCubemaps[i]);

		// Draw a quad mapping scene color to the view's render target
		DrawRectangle( 
			0, 0,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Min.X, View.ViewRect.Min.Y, 
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Size(),
			GSceneRenderTargets.SceneColor->GetDesc().Extent,
			EDRF_UseTriangleOptimization);
	}

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessAmbient::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	// we assume this pass is additively blended with the scene color so this data is not needed
	FPooledRenderTargetDesc Ret;

	Ret.DebugName = TEXT("AmbientCubeMap");

	return Ret;
}

/*-----------------------------------------------------------------------------
FCubemapShaderParameters
-----------------------------------------------------------------------------*/

void FCubemapShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	AmbientCubemapColor.Bind(ParameterMap, TEXT("AmbientCubemapColor"));
	AmbientCubemapMipAdjust.Bind(ParameterMap, TEXT("AmbientCubemapMipAdjust"));
	AmbientCubemap.Bind(ParameterMap, TEXT("AmbientCubemap"));
	AmbientCubemapSampler.Bind(ParameterMap, TEXT("AmbientCubemapSampler"));
}

void FCubemapShaderParameters::SetParameters(const FPixelShaderRHIParamRef ShaderRHI, const FFinalPostProcessSettings::FCubemapEntry& Entry) const
{
	SetParametersTemplate(ShaderRHI,Entry);
}

void FCubemapShaderParameters::SetParameters(const FComputeShaderRHIParamRef ShaderRHI, const FFinalPostProcessSettings::FCubemapEntry& Entry) const
{
	SetParametersTemplate(ShaderRHI,Entry);
}

template<typename TShaderRHIRef>
void FCubemapShaderParameters::SetParametersTemplate(const TShaderRHIRef& ShaderRHI, const FFinalPostProcessSettings::FCubemapEntry& Entry) const
{
	// floats to render the cubemap
	{
		float MipCount = 0;

		if(Entry.AmbientCubemap)
		{
			int32 CubemapWidth = Entry.AmbientCubemap->GetSurfaceWidth();
			MipCount = FMath::Log2(CubemapWidth) + 1.0f;
		}

		{
			FLinearColor AmbientCubemapTintMulScaleValue = Entry.AmbientCubemapTintMulScaleValue;

			SetShaderValue(ShaderRHI, AmbientCubemapColor, AmbientCubemapTintMulScaleValue);
		}

		{
			FVector4 AmbientCubemapMipAdjustValue(0, 0, 0, 0);

			AmbientCubemapMipAdjustValue.X =  1.0f - GDiffuseConvolveMipLevel / MipCount;
			AmbientCubemapMipAdjustValue.Y = (MipCount - 1.0f) * AmbientCubemapMipAdjustValue.X;
			AmbientCubemapMipAdjustValue.Z = MipCount - GDiffuseConvolveMipLevel;
			AmbientCubemapMipAdjustValue.W = MipCount;

			SetShaderValue(ShaderRHI, AmbientCubemapMipAdjust, AmbientCubemapMipAdjustValue);
		}
	}

	// cubemap texture
	{
		FTexture* InternalSpec = Entry.AmbientCubemap ? Entry.AmbientCubemap->Resource : GBlackTextureCube;

		SetTextureParameter(ShaderRHI, AmbientCubemap, AmbientCubemapSampler, InternalSpec);
	}
}

FArchive& operator<<(FArchive& Ar, FCubemapShaderParameters& P)
{
	Ar << P.AmbientCubemapColor << P.AmbientCubemap << P.AmbientCubemapSampler << P.AmbientCubemapMipAdjust;

	return Ar;
}
