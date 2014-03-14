//-----------------------------------------------------------------------------
// File:		PostProcessLpvIndirect.cpp
//
// Summary:		Light propagation volume postprocessing
//
// Created:		11/03/2013
//
// Author:		mailto:benwood@microsoft.com
//
//				Copyright (C) Microsoft. All rights reserved.
//-----------------------------------------------------------------------------

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PostProcess/SceneFilterRendering.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcessLpvIndirect.h"
#include "LightPropagationVolume.h"
#include "UniformBuffer.h"

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FLpvReadUniformBufferParameters,TEXT("LpvRead"));
typedef TUniformBufferRef<FLpvReadUniformBufferParameters> FLpvReadUniformBufferRef;

/** Encapsulates the post processing ambient pixel shader. */
class FPostProcessLpvIndirectPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessLpvIndirectPS, Global);

	//@todo-rco: Remove this when reenabling for OpenGL
	static bool ShouldCache( EShaderPlatform Platform )		{ return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && !IsOpenGLPlatform(Platform); }

	/** Default constructor. */
	FPostProcessLpvIndirectPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;

#if LPV_VOLUME_TEXTURE
	FShaderResourceParameter LpvBufferSRVParameters[7];
	FShaderResourceParameter LpvVolumeTextureSampler;
#else
	FShaderResourceParameter LpvBufferSRV;
#endif 

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter PreIntegratedGF;
	FShaderResourceParameter PreIntegratedGFSampler;

	/** Initialization constructor. */
	FPostProcessLpvIndirectPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
#if LPV_VOLUME_TEXTURE
		for ( int i=0; i<7; i++ )
		{
			LpvBufferSRVParameters[i].Bind( Initializer.ParameterMap, LpvVolumeTextureSRVNames[i] );
		}
		LpvVolumeTextureSampler.Bind(Initializer.ParameterMap, TEXT("gLpv3DTextureSampler"));
#else
		LpvBufferSRV.Bind( Initializer.ParameterMap, TEXT("gLpvBuffer") );
#endif

		PreIntegratedGF.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGF"));
		PreIntegratedGFSampler.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGFSampler"));
	}

	void SetParameters(	
#if LPV_VOLUME_TEXTURE
		FTextureRHIParamRef* LpvBufferSRVsIn, 
#else
		FShaderResourceViewRHIParamRef LpvBufferSRVIn, 
#endif 

		FLpvReadUniformBufferRef LpvUniformBuffer, 
		const FRenderingCompositePassContext& Context )
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		SetUniformBufferParameter( ShaderRHI, GetUniformBufferParameter<FLpvReadUniformBufferParameters>(), LpvUniformBuffer );

#if LPV_VOLUME_TEXTURE
		for ( int i=0; i<7; i++ )
		{
			if ( LpvBufferSRVParameters[i].IsBound() )
			{
				RHISetShaderTexture( ShaderRHI, LpvBufferSRVParameters[i].GetBaseIndex(), LpvBufferSRVsIn[i] );
				SetTextureParameter( ShaderRHI, LpvBufferSRVParameters[i], LpvVolumeTextureSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), LpvBufferSRVsIn[i] );
			}
		}
#else
		if ( LpvBufferSRV.IsBound() )
		{
			RHISetShaderResourceViewParameter( ShaderRHI, LpvBufferSRV.GetBaseIndex(), LpvBufferSRVIn );
		}
#endif
		FGlobalShader::SetParameters(ShaderRHI, Context.View);
		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
		DeferredParameters.Set(ShaderRHI, Context.View);
		SetTextureParameter( ShaderRHI, PreIntegratedGF, PreIntegratedGFSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), GSystemTextures.PreintegratedGF->GetRenderTargetItem().ShaderResourceTexture );
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter;

#if LPV_VOLUME_TEXTURE
		for ( int i=0; i<7; i++ )
		{
			Ar << LpvBufferSRVParameters[i];
		}
		Ar << LpvVolumeTextureSampler;
#else
		Ar << LpvBufferSRV;
#endif
		Ar << DeferredParameters;
		Ar << PreIntegratedGF;
		Ar << PreIntegratedGFSampler;

		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessLpvIndirectPS,TEXT("PostProcessLpvIndirect"),TEXT("MainPS"),SF_Pixel);


void FRCPassPostProcessLpvIndirect::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessLpvIndirect, DEC_SCENE_ITEMS);

	const FPostProcessSettings& PostprocessSettings = Context.View.FinalPostProcessSettings;
	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	FIntRect SrcRect = View.ViewRect;
	// todo: view size should scale with input texture size so we can do SSAO in half resolution as well
	FIntRect DestRect = View.ViewRect;
	FIntPoint DestSize = DestRect.Size();

	uint32 NumReflectionCaptures = ViewFamily.Scene->GetRenderScene()->ReflectionSceneData.RegisteredReflectionCaptures.Num();

	const FSceneRenderTargetItem& DestColorRenderTarget = GSceneRenderTargets.SceneColor->GetRenderTargetItem();

	// Set the view family's render target/viewport.
	FTextureRHIParamRef RenderTargets[] =
	{
		DestColorRenderTarget.TargetableTexture,
	};

	// Set the view family's render target/viewport.
	RHISetRenderTargets(1, RenderTargets, GSceneRenderTargets.GetSceneDepthSurface(), 0, NULL);

	Context.SetViewportAndCallRHI(View.ViewRect);

	// set the state
	if ( ViewFamily.EngineShowFlags.LpvLightingOnly )
	{
		RHISetBlendState( TStaticBlendState<>::GetRHI() );
	}
	else
	{
		// additive blending
		RHISetBlendState(TStaticBlendState<CW_RGB,BO_Add,BF_One,BF_One>::GetRHI());
	}
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());

	FSceneViewState* ViewState = (FSceneViewState*)View.State;
	FLightPropagationVolume* Lpv = NULL;
	bool bUseLpv = false;
	if ( ViewState )
	{
		Lpv = ViewState->GetLightPropagationVolume();

		bUseLpv = Lpv && PostprocessSettings.LPVIntensity > 0.0f;
	}

	if ( !bUseLpv )
	{
		return;
	}

	TShaderMapRef<FPostProcessLpvIndirectPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;

	// call it once after setting up the shader data to avoid the warnings in the function
	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	FLpvReadUniformBufferParameters	LpvReadUniformBufferParams;
	FLpvReadUniformBufferRef LpvReadUniformBuffer;

	LpvReadUniformBufferParams = Lpv->GetReadUniformBufferParams();
	LpvReadUniformBuffer = FLpvReadUniformBufferRef::CreateUniformBufferImmediate( LpvReadUniformBufferParams, UniformBuffer_SingleUse ); 

#if LPV_VOLUME_TEXTURE
	FTextureRHIParamRef LpvBufferSrvs[7];
	for ( int i = 0; i < 7; i++ ) 
	{
		LpvBufferSrvs[i] = Lpv->GetLpvBufferSrv(i);
	}
	PixelShader->SetParameters(LpvBufferSrvs, LpvReadUniformBuffer, Context );
#else
	FShaderResourceViewRHIParamRef LpvBufferSrv = Lpv->GetLpvBufferSrv();
	PixelShader->SetParameters(LpvBufferSrv, LpvReadUniformBuffer, Context );
#endif

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle( 
		0, 0,
		View.ViewRect.Width(), View.ViewRect.Height(),
		View.ViewRect.Min.X, View.ViewRect.Min.Y, 
		View.ViewRect.Width(), View.ViewRect.Height(),
		View.ViewRect.Size(),
		GSceneRenderTargets.SceneColor->GetDesc().Extent);

	RHICopyToResolveTarget(DestColorRenderTarget.TargetableTexture, DestColorRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessLpvIndirect::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	// we assume this pass is additively blended with the scene color so this data is not needed
	FPooledRenderTargetDesc Ret;

	Ret.DebugName = TEXT("LpvIndirect");

	return Ret;
}
