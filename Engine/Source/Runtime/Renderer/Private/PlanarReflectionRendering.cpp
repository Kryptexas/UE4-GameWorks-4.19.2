// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
 PlanarReflectionRendering.cpp
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessing.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ScreenRendering.h"
#include "ShaderParameterUtils.h"
#include "LightRendering.h"
#include "SceneUtils.h"
#include "PlanarReflectionSceneProxy.h"

class FPlanarReflectionPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPlanarReflectionPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	}

	/** Default constructor. */
	FPlanarReflectionPS() {}

	/** Initialization constructor. */
	FPlanarReflectionPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		PlanarReflectionParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FPlanarReflectionSceneProxy* ReflectionSceneProxy)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		PlanarReflectionParameters.SetParameters(RHICmdList, ShaderRHI, ReflectionSceneProxy);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PlanarReflectionParameters;
		Ar << DeferredParameters;
		return bShaderHasOutdatedParameters;
	}

private:

	FPlanarReflectionParameters PlanarReflectionParameters;
	FDeferredPixelShaderParameters DeferredParameters;
};

IMPLEMENT_SHADER_TYPE(,FPlanarReflectionPS,TEXT("PlanarReflectionShaders"),TEXT("PlanarReflectionPS"),SF_Pixel);

bool FDeferredShadingSceneRenderer::RenderDeferredPlanarReflections(FRHICommandListImmediate& RHICmdList, bool bLightAccumulationIsInUse, TRefCountPtr<IPooledRenderTarget>& Output)
{
	bool bAnyViewIsReflectionCapture = false;
	for (int32 ViewIndex = 0, Num = Views.Num(); ViewIndex < Num; ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];
		bAnyViewIsReflectionCapture = bAnyViewIsReflectionCapture || View.bIsPlanarReflection || View.bIsReflectionCapture;
	}

	// Prevent reflection recursion, or view-dependent planar reflections being seen in reflection captures
	if (Scene->PlanarReflections.Num() > 0 && !bAnyViewIsReflectionCapture)
	{
		bool bSSRAsInput = true;

		if (Output == GSystemTextures.BlackDummy)
		{
			bSSRAsInput = false;
			FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

			if (bLightAccumulationIsInUse)
			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(SceneContext.GetBufferSizeXY(), PF_FloatRGBA, FClearValueBinding::Black, TexCreate_None, TexCreate_RenderTargetable, false));
				GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Output, TEXT("PlanarReflectionComposite"));
			}
			else
			{
				Output = SceneContext.LightAccumulation;
			}
		}

		SCOPED_DRAW_EVENT(RHICmdList, CompositePlanarReflections);

		SetRenderTarget(RHICmdList, Output->GetRenderTargetItem().TargetableTexture, NULL);

		if (!bSSRAsInput)
		{
			RHICmdList.Clear(true, FLinearColor(0, 0, 0, 0), false, 0, false, 0, FIntRect());
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			FViewInfo& View = Views[ViewIndex];

			RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

			// Blend over previous reflections in the output target (SSR or planar reflections that have already been rendered)
			// Planar reflections win over SSR and reflection environment
			//@todo - this is order dependent blending, but ordering is coming from registration order
			RHICmdList.SetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha, BO_Max, BF_One, BF_One>::GetRHI());
			RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

			for (int32 PlanarReflectionIndex = 0; PlanarReflectionIndex < Scene->PlanarReflections.Num(); PlanarReflectionIndex++)
			{
				FPlanarReflectionSceneProxy* ReflectionSceneProxy = Scene->PlanarReflections[PlanarReflectionIndex];

				SCOPED_DRAW_EVENTF(RHICmdList, PlanarReflection, *ReflectionSceneProxy->OwnerName.ToString());

				TShaderMapRef<TDeferredLightVS<false> > VertexShader(View.ShaderMap);
				TShaderMapRef<FPlanarReflectionPS> PixelShader(View.ShaderMap);

				static FGlobalBoundShaderState BoundShaderState;
				SetGlobalBoundShaderState(RHICmdList, FeatureLevel, BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

				PixelShader->SetParameters(RHICmdList, View, ReflectionSceneProxy);
				VertexShader->SetSimpleLightParameters(RHICmdList, View, FSphere(0));

				DrawRectangle( 
					RHICmdList,
					0, 0,
					View.ViewRect.Width(), View.ViewRect.Height(),
					View.ViewRect.Min.X, View.ViewRect.Min.Y, 
					View.ViewRect.Width(), View.ViewRect.Height(),
					View.ViewRect.Size(),
					FSceneRenderTargets::Get(RHICmdList).GetBufferSizeXY(),
					*VertexShader,
					EDRF_UseTriangleOptimization);
			}
		}

		RHICmdList.CopyToResolveTarget(Output->GetRenderTargetItem().TargetableTexture, Output->GetRenderTargetItem().ShaderResourceTexture, false, FResolveParams());

		return true;
	}

	return false;
}
