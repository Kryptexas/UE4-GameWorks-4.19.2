// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessMaterial.cpp: Post processing Material implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessMaterial.h"
#include "PostProcessing.h"
#include "PostProcessEyeAdaptation.h"
#include "../../../Engine/Public/TileRendering.h"

class FPostProcessMaterialVS : public FMaterialShader
{
	DECLARE_SHADER_TYPE(FPostProcessMaterialVS, Material);
public:

	/**
	  * Only compile these shaders for post processing domain materials
	  */
	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material)
	{
		return (Material->GetMaterialDomain() == MD_PostProcess) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	FPostProcessMaterialVS( )	{ }
	FPostProcessMaterialVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMaterialShader(Initializer)
	{
	}

	void SetParameters( const FRenderingCompositePassContext& Context )
	{
		FMaterialShader::SetParameters(GetVertexShader(), Context.View);
	}

	// Begin FShader interface
	virtual bool Serialize(FArchive& Ar) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FMaterialShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}
	//  End FShader interface 
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FPostProcessMaterialVS,TEXT("PostProcessMaterialShaders"),TEXT("MainVS"),SF_Vertex);

/**
 * A pixel shader for rendering a post process material
 */
class FPostProcessMaterialPS : public FMaterialShader
{
	DECLARE_SHADER_TYPE(FPostProcessMaterialPS,Material);
public:

	/**
	  * Only compile these shaders for post processing domain materials
	  */
	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material)
	{
		return (Material->GetMaterialDomain() == MD_PostProcess) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	FPostProcessMaterialPS() {}
	FPostProcessMaterialPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMaterialShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
	}

	void SetParameters( const FRenderingCompositePassContext& Context, const FMaterialRenderProxy* Material )
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FMaterialShader::SetParameters(ShaderRHI, Material, *Material->GetMaterial(GRHIFeatureLevel), Context.View, true, ESceneRenderTargetsMode::SetTextures);
		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMaterialShader::Serialize(Ar);
		Ar << PostprocessParameter;
		return bShaderHasOutdatedParameters;
	}

private:
	FPostProcessPassParameters PostprocessParameter;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FPostProcessMaterialPS,TEXT("PostProcessMaterialShaders"),TEXT("MainPS"),SF_Pixel);

FRCPassPostProcessMaterial::FRCPassPostProcessMaterial(UMaterialInterface* InMaterialInterface)
	: MaterialInterface(InMaterialInterface)
{
}

void FRCPassPostProcessMaterial::Process(FRenderingCompositePassContext& Context)
{
	FMaterialRenderProxy* Proxy = MaterialInterface->GetRenderProxy(false);

	check(Proxy);

	const FMaterial* Material = Proxy->GetMaterial(GRHIFeatureLevel);
	
	check(Material);

	SCOPED_DRAW_EVENTF(PostProcessMaterial, DEC_SCENE_ITEMS, TEXT("PostProcessMaterial Material=%s"), *Material->GetFriendlyName());

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	// hacky cast
	FRenderingCompositePassContext RenderingCompositePassContext((FViewInfo&)View);
	RenderingCompositePassContext.Pass = this;

	FIntRect SrcRect = View.ViewRect;
	FIntRect DestRect = View.ViewRect;
	FIntPoint SrcSize = InputDesc->Extent;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIParamRef());	

	if( ViewFamily.RenderTarget->GetRenderTargetTexture() != DestRenderTarget.TargetableTexture )
	{
		RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, View.ViewRect);
	}

	Context.SetViewportAndCallRHI(View.ViewRect);

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	float ScaleX = 1.0f / InputDesc->Extent.X;
	float ScaleY = 1.0f / InputDesc->Extent.Y;

	const FMaterialShaderMap* MaterialShaderMap = Material->GetRenderingThreadShaderMap();
	FPostProcessMaterialPS* PixelShader = MaterialShaderMap->GetShader<FPostProcessMaterialPS>();
	FPostProcessMaterialVS* VertexShader = MaterialShaderMap->GetShader<FPostProcessMaterialVS>();

	FBoundShaderStateRHIRef BoundShaderState = RHICreateBoundShaderState(GetVertexDeclarationFVector4(), VertexShader->GetVertexShader(), FHullShaderRHIRef(), FDomainShaderRHIRef(), PixelShader->GetPixelShader(), FGeometryShaderRHIRef());
	RHISetBoundShaderState(BoundShaderState);

	VertexShader->SetParameters(Context);
	PixelShader->SetParameters(Context, MaterialInterface->GetRenderProxy(false));

	DrawRectangle(
		0, 0,
		DestRect.Width(), DestRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y,
		SrcRect.Width(), SrcRect.Height(),
		DestRect.Size(),
		SrcSize,
		EDRF_UseTriangleOptimization);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessMaterial::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.Reset();
	Ret.DebugName = TEXT("PostProcessMaterial");

	return Ret;
}