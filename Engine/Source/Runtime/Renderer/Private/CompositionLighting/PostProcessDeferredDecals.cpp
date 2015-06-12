// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessDeferredDecals.cpp: Deferred Decals implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessing.h"
#include "PostProcessDeferredDecals.h"
#include "ScreenRendering.h"
#include "SceneUtils.h"

#define DBUFFER_DONT_USE_STENCIL_YET 1


static TAutoConsoleVariable<float> CVarStencilSizeThreshold(
	TEXT("r.Decal.StencilSizeThreshold"),
	0.1f,
	TEXT("Control a per decal stencil pass that allows to large (screen space) decals faster. It adds more overhead per decals so this\n")
	TEXT("  <0: optimization is disabled\n")
	TEXT("   0: optimization is enabled no matter how small (screen space) the decal is\n")
	TEXT("0..1: optimization is enabled, value defines the minimum size (screen space) to trigger the optimization (default 0.1)")
	);

enum ERenderTargetMode
{
	RTM_Unknown = -1,
	RTM_SceneColorAndGBuffer,
	RTM_SceneColorAndGBufferDepthWrite,
	RTM_DBuffer,
	RTM_GBufferNormal,
	RTM_SceneColor,
};

enum EDecalDepthInputState
{
	DDS_Undefined,
	DDS_Always,
	DDS_DepthTest,
	DDS_DepthAlways_StencilEqual1Write,
	DDS_DepthAlways_StencilEqual0,
	DDS_DepthTest_StencilEqual1Write,
	DDS_DepthTest_StencilEqual0,
};

struct FDecalDepthState
{
	EDecalDepthInputState DepthTest;
	bool bDepthOutput;

	FDecalDepthState()
		: DepthTest(DDS_Undefined)
		, bDepthOutput(false)
	{
	}

	bool operator !=(const FDecalDepthState &rhs) const
	{
		return DepthTest != rhs.DepthTest || bDepthOutput != rhs.bDepthOutput;
	}
};

enum EDecalRasterizerState
{
	DRS_Undefined,
	DRS_CCW,
	DRS_CW,
};


// @return DECAL_RENDERTARGET_COUNT for the shader
uint32 ComputeRenderTargetCount(ERenderTargetMode RenderTargetMode)
{
	switch(RenderTargetMode)
	{
		case RTM_SceneColorAndGBuffer:				return 4;
		case RTM_SceneColorAndGBufferDepthWrite:	return 4;
		case RTM_DBuffer:							return 3;
		case RTM_GBufferNormal:						return 1;
		case RTM_SceneColor:						return 1;
	}

	return 0;
}

EDecalBlendMode ComputeFinalDecalBlendMode(EDecalBlendMode DecalBlendMode, bool bUseNormal)
{
	if(!bUseNormal)
	{
		if(DecalBlendMode == DBM_DBuffer_ColorNormalRoughness)
		{
			DecalBlendMode = DBM_DBuffer_ColorRoughness;
		}
		else if(DecalBlendMode == DBM_DBuffer_NormalRoughness)
		{
			DecalBlendMode = DBM_DBuffer_Roughness;
		}
	}

	return DecalBlendMode;
}

ERenderTargetMode ComputeRenderTargetMode(EDecalBlendMode DecalBlendMode)
{
	switch(DecalBlendMode)
	{
		case DBM_Translucent:
		case DBM_Stain:
			return RTM_SceneColorAndGBuffer;

		case DBM_Normal:
			return RTM_GBufferNormal;

		case DBM_Emissive:
			return RTM_SceneColor;

		case DBM_DBuffer_ColorNormalRoughness:
		case DBM_DBuffer_Color:
		case DBM_DBuffer_ColorNormal:
		case DBM_DBuffer_ColorRoughness:
		case DBM_DBuffer_Normal:
		case DBM_DBuffer_NormalRoughness:
		case DBM_DBuffer_Roughness:
			// can be optimized using less MRT when possible
			return RTM_DBuffer;

		case DBM_Volumetric_DistanceFunction:
			return RTM_SceneColorAndGBufferDepthWrite;
	}

	// add the missing decal blend mode to the switch
	check(0);
	return RTM_Unknown;
}

// @return see EDecalRenderStage
EDecalRenderStage ComputeRenderStage(EDecalBlendMode DecalBlendMode)
{
	switch(DecalBlendMode)
	{
		case DBM_DBuffer_ColorNormalRoughness:
		case DBM_DBuffer_Color:
		case DBM_DBuffer_ColorNormal:
		case DBM_DBuffer_ColorRoughness:
		case DBM_DBuffer_Normal:
		case DBM_DBuffer_NormalRoughness:
		case DBM_DBuffer_Roughness:
			return DRS_BeforeBasePass;

		case DBM_Translucent:
		case DBM_Stain:
		case DBM_Normal:
		case DBM_Emissive:
			return DRS_BeforeLighting;
		
		case DBM_Volumetric_DistanceFunction:
			return DRS_AfterBasePass;

		default:
			check(0);
	}
	
	return DRS_BeforeBasePass;
}

struct FTransientDecalRenderData
{
	EDecalBlendMode DecalBlendMode;
	const FMaterialRenderProxy* MaterialProxy;
	const FMaterial* MaterialResource;
	const FDeferredDecalProxy* DecalProxy;
	bool bHasNormal;

	FTransientDecalRenderData(const FScene& InScene, FDeferredDecalProxy* InDecalProxy)
		: DecalProxy(InDecalProxy)
	{
		MaterialProxy = InDecalProxy->DecalMaterial->GetRenderProxy(InDecalProxy->bOwnerSelected);
		MaterialResource = MaterialProxy->GetMaterial(InScene.GetFeatureLevel());
		bHasNormal = MaterialResource->HasNormalConnected();
		DecalBlendMode = ComputeFinalDecalBlendMode((EDecalBlendMode)MaterialResource->GetDecalBlendMode(), bHasNormal);
		check(MaterialProxy && MaterialResource);
	}
};
// @param RenderState 0:before BasePass, 1:before lighting, (later we could add "after lighting" and multiply)
void SetDecalBlendState(FRHICommandList& RHICmdList, const ERHIFeatureLevel::Type SMFeatureLevel, EDecalRenderStage InDecalRenderStage, EDecalBlendMode DecalBlendMode, bool bHasNormal)
{
	if(InDecalRenderStage == DRS_BeforeBasePass)
	{
		// before base pass (for DBuffer decals)

		if(SMFeatureLevel == ERHIFeatureLevel::SM4)
		{
			// DX10 doesn't support masking/using different blend modes per MRT.
			// We set the opacity in the shader to 0 so we can use the same frame buffer blend.

			RHICmdList.SetBlendState( TStaticBlendState< 
				CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
				CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
				CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha >::GetRHI() );
			return;
		}
		else
		{
			// see DX10 comment above
			// As we set the opacity in the shader we don't need to set different frame buffer blend modes but we like to hint to the driver that we
			// don't need to output there. We also could replace this with many SetRenderTarget calls but it might be slower (needs to be tested).

			switch(DecalBlendMode)
			{
			case DBM_DBuffer_ColorNormalRoughness:
				RHICmdList.SetBlendState( TStaticBlendState< 
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha >::GetRHI() );		
				break;

			case DBM_DBuffer_Color:
				// we can optimize using less MRT later
				RHICmdList.SetBlendState( TStaticBlendState< 
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One,
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One>::GetRHI() );		
				break;

			case DBM_DBuffer_ColorNormal:
				// we can optimize using less MRT later
				RHICmdList.SetBlendState( TStaticBlendState< 
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One >::GetRHI() );		
				break;

			case DBM_DBuffer_ColorRoughness:
				// we can optimize using less MRT later
				RHICmdList.SetBlendState( TStaticBlendState< 
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One,
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha >::GetRHI() );		
				break;

			case DBM_DBuffer_Normal:
				// we can optimize using less MRT later
				RHICmdList.SetBlendState( TStaticBlendState< 
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One,
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One>::GetRHI() );		
				break;

			case DBM_DBuffer_NormalRoughness:
				// we can optimize using less MRT later
				RHICmdList.SetBlendState( TStaticBlendState< 
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One,
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha,
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha >::GetRHI() );		
				break;

			case DBM_DBuffer_Roughness:
				// we can optimize using less MRT later
				RHICmdList.SetBlendState( TStaticBlendState< 
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One,
					CW_RGBA, BO_Add, BF_Zero, BF_One,								BO_Add,BF_Zero,BF_One,
					CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add,BF_Zero,BF_InverseSourceAlpha >::GetRHI() );		
				break;

			default:
				// the decal type should not be rendered in this pass - internal error
				check(0);	
				break;
			}
		}

		return;
	}
	else if(InDecalRenderStage == DRS_AfterBasePass)
	{
		ensure(DecalBlendMode == DBM_Volumetric_DistanceFunction);

		RHICmdList.SetBlendState( TStaticBlendState<>::GetRHI() );
	}	
	else
	{
		// before lighting (for non DBuffer decals)

		switch(DecalBlendMode)
		{
		case DBM_Translucent:
			// @todo: Feature Level 10 does not support separate blends modes for each render target. This could result in the
			// translucent and stain blend modes looking incorrect when running in this mode.
			if(GSupportsSeparateRenderTargetBlendState)
			{
				if(bHasNormal)
				{
					RHICmdList.SetBlendState( TStaticBlendState<
						CW_RGB, BO_Add, BF_SourceAlpha, BF_One,						BO_Add, BF_Zero, BF_One,	// Emissive
						CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Normal
						CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Metallic, Specular, Roughness
						CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One		// BaseColor
					>::GetRHI() );
				}
				else
				{
					RHICmdList.SetBlendState( TStaticBlendState<
						CW_RGB, BO_Add, BF_SourceAlpha, BF_One,						BO_Add, BF_Zero, BF_One,	// Emissive
						CW_RGB, BO_Add, BF_Zero,		BF_One,						BO_Add, BF_Zero, BF_One,	// Normal
						CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Metallic, Specular, Roughness
						CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One		// BaseColor
					>::GetRHI() );
				}
			}
			else if(SMFeatureLevel == ERHIFeatureLevel::SM4)
			{
				RHICmdList.SetBlendState( TStaticBlendState<
					CW_RGB, BO_Add, BF_SourceAlpha, BF_One,							BO_Add, BF_Zero, BF_One,	// Emissive
					CW_RGB, BO_Add, BF_SourceAlpha, BF_One,							BO_Add, BF_Zero, BF_One,	// Normal
					CW_RGB, BO_Add, BF_SourceAlpha, BF_One,							BO_Add, BF_Zero, BF_One,	// Metallic, Specular, Roughness
					CW_RGB, BO_Add, BF_SourceAlpha,	BF_One,							BO_Add, BF_Zero, BF_One		// BaseColor
				>::GetRHI() );
			}
			break;

		case DBM_Stain:
			if(GSupportsSeparateRenderTargetBlendState)
			{
				if(bHasNormal)
				{
					RHICmdList.SetBlendState( TStaticBlendState<
						CW_RGB, BO_Add, BF_SourceAlpha, BF_One,						BO_Add, BF_Zero, BF_One,	// Emissive
						CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Normal
						CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Metallic, Specular, Roughness
						CW_RGB, BO_Add, BF_DestColor,	BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One		// BaseColor
					>::GetRHI() );
				}
				else
				{
					RHICmdList.SetBlendState( TStaticBlendState<
						CW_RGB, BO_Add, BF_SourceAlpha, BF_One,						BO_Add, BF_Zero, BF_One,	// Emissive
						CW_RGB, BO_Add, BF_Zero,		BF_One,						BO_Add, BF_Zero, BF_One,	// Normal
						CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Metallic, Specular, Roughness
						CW_RGB, BO_Add, BF_DestColor,	BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One		// BaseColor
					>::GetRHI() );
				}
			}
			else if(SMFeatureLevel == ERHIFeatureLevel::SM4)
			{
				RHICmdList.SetBlendState( TStaticBlendState<
					CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add, BF_Zero, BF_One,	// Emissive
					CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add, BF_Zero, BF_One,	// Normal
					CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,			BO_Add, BF_Zero, BF_One,	// Metallic, Specular, Roughness
					CW_RGB, BO_Add, BF_SourceAlpha,	BF_InverseSourceAlpha,			BO_Add, BF_Zero, BF_One		// BaseColor
				>::GetRHI() );
			}
			break;

		case DBM_Normal:
			RHICmdList.SetBlendState( TStaticBlendState< CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha >::GetRHI() );
			break;

		case DBM_Emissive:
			RHICmdList.SetBlendState( TStaticBlendState< CW_RGB, BO_Add, BF_SourceAlpha, BF_One >::GetRHI() );
			break;

		default:
			// the decal type should not be rendered in this pass - internal error
			check(0);	
			break;
		}
	}
}


const FVector GDefaultDecalSize(1.0f, 1.0f, 1.0f);

/** Pixel shader used to setup the decal receiver mask */
class FStencilDecalMaskPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FStencilDecalMaskPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4); 
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
	}

	FStencilDecalMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		MaskComparison.Bind(Initializer.ParameterMap,TEXT("MaskComparison"));
	}
	FStencilDecalMaskPS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		SetShaderValue(RHICmdList, ShaderRHI, MaskComparison, View.Family->EngineShowFlags.ShaderComplexity ? -1.0f : 0.5f);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters << MaskComparison;
		return bShaderHasOutdatedParameters;
	}

private:
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter MaskComparison;
};

IMPLEMENT_SHADER_TYPE(,FStencilDecalMaskPS,TEXT("DeferredDecal"),TEXT("StencilDecalMaskMain"),SF_Pixel);

FGlobalBoundShaderState StencilDecalMaskBoundShaderState;

/** Draws a full view quad that sets stencil to 1 anywhere that decals should not be projected. */
void StencilDecalMask(FRHICommandList& RHICmdList, const FViewInfo& View)
{
	SCOPED_DRAW_EVENT(RHICmdList, StencilDecalMask);
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
	RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
	RHICmdList.SetBlendState(TStaticBlendState<CW_NONE>::GetRHI());
	SetRenderTarget(RHICmdList, NULL, SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EUninitializedColorExistingDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);
	RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

	// Write 1 to highest bit of stencil to areas that should not receive decals
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always, true, CF_Always, SO_Replace, SO_Replace, SO_Replace>::GetRHI(), 0x80);

	const auto FeatureLevel = View.GetFeatureLevel();
	auto ShaderMap = View.ShaderMap;
	TShaderMapRef<FScreenVS> ScreenVertexShader(ShaderMap);
	TShaderMapRef<FStencilDecalMaskPS> PixelShader(ShaderMap);
	
	SetGlobalBoundShaderState(RHICmdList, FeatureLevel, StencilDecalMaskBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *ScreenVertexShader, *PixelShader);

	PixelShader->SetParameters(RHICmdList, View);

	DrawRectangle( 
		RHICmdList,
		0, 0, 
		View.ViewRect.Width(), View.ViewRect.Height(),
		View.ViewRect.Min.X, View.ViewRect.Min.Y, 
		View.ViewRect.Width(), View.ViewRect.Height(),
		FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
		SceneContext.GetBufferSizeXY(),
		*ScreenVertexShader,
		EDRF_UseTriangleOptimization);
}

/**
 * A vertex shader for projecting a deferred decal onto the scene.
 */
class FDeferredDecalVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDeferredDecalVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FDeferredDecalVS( )	{ }
	FDeferredDecalVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		FrustumComponentToClip.Bind(Initializer.ParameterMap, TEXT("FrustumComponentToClip"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, const FMatrix& InFrustumComponentToClip)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		SetShaderValue(RHICmdList, ShaderRHI, FrustumComponentToClip, InFrustumComponentToClip);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << FrustumComponentToClip;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter FrustumComponentToClip;
};

IMPLEMENT_SHADER_TYPE(,FDeferredDecalVS,TEXT("DeferredDecal"),TEXT("MainVS"),SF_Vertex);

/**
 * A pixel shader for projecting a deferred decal onto the scene.
 */
class FDeferredDecalPS : public FMaterialShader
{
	DECLARE_SHADER_TYPE(FDeferredDecalPS,Material);
public:

	/**
	  * Makes sure only shaders for materials that are explicitly flagged
	  * as 'UsedAsDeferredDecal' in the Material Editor gets compiled into
	  * the shader cache.
	  */
	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material)
	{
		if (!Material->IsUsedWithDeferredDecal())
		{
			return false;
		}

		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
	{
		FMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);

		check(Material);
		
		EDecalBlendMode DecalBlendMode = (EDecalBlendMode)Material->GetDecalBlendMode();
		ERenderTargetMode RenderTargetMode = ComputeRenderTargetMode(DecalBlendMode);
		uint32 RenderTargetCount = ComputeRenderTargetCount(RenderTargetMode);

		// avoid using the index directly, better use DECALBLENDMODEID_VOLUMETRIC, DECALBLENDMODEID_STAIN, ...
		OutEnvironment.SetDefine(TEXT("DECAL_BLEND_MODE"), (int32)DecalBlendMode);
		OutEnvironment.SetDefine(TEXT("DECAL_PROJECTION"), 1);
		OutEnvironment.SetDefine(TEXT("DECAL_RENDERTARGET_COUNT"), RenderTargetCount);
		OutEnvironment.SetDefine(TEXT("DECAL_RENDERSTAGE"), (int32)ComputeRenderStage(DecalBlendMode));

		// to compare against DECAL_BLEND_MODE, we can expose more if needed
		OutEnvironment.SetDefine(TEXT("DECALBLENDMODEID_VOLUMETRIC"), (int32)DBM_Volumetric_DistanceFunction);
		OutEnvironment.SetDefine(TEXT("DECALBLENDMODEID_STAIN"), (int32)DBM_Stain);
		OutEnvironment.SetDefine(TEXT("DECALBLENDMODEID_NORMAL"), (int32)DBM_Normal);
	}

	FDeferredDecalPS() {}
	FDeferredDecalPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMaterialShader(Initializer)
	{
		ScreenToDecal.Bind(Initializer.ParameterMap,TEXT("ScreenToDecal"));
		DecalToWorld.Bind(Initializer.ParameterMap,TEXT("DecalToWorld"));
		WorldToDecal.Bind(Initializer.ParameterMap,TEXT("WorldToDecal"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, const FMaterialRenderProxy* MaterialProxy, const FDeferredDecalProxy& DecalProxy)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FMaterialShader::SetParameters(RHICmdList, ShaderRHI, MaterialProxy, *MaterialProxy->GetMaterial(View.GetFeatureLevel()), View, true, ESceneRenderTargetsMode::SetTextures);

		FTransform ComponentTrans = DecalProxy.ComponentTrans;

		// 1,1,1 requires no scale
		//			ComponentTrans = ComponentTrans.GetScaled(GDefaultDecalSize);

		FMatrix WorldToComponent = ComponentTrans.ToMatrixWithScale().InverseFast();

		// Set the transform from screen space to light space.
		if(ScreenToDecal.IsBound())
		{
			const FMatrix ScreenToDecalValue = 
				FMatrix(
					FPlane(1,0,0,0),
					FPlane(0,1,0,0),
					FPlane(0,0,View.ViewMatrices.ProjMatrix.M[2][2],1),
					FPlane(0,0,View.ViewMatrices.ProjMatrix.M[3][2],0)
				) * View.InvViewProjectionMatrix * WorldToComponent;

			SetShaderValue(RHICmdList, ShaderRHI, ScreenToDecal, ScreenToDecalValue);
		}

		// Set the transform from light space to world space
		if(DecalToWorld.IsBound())
		{
			const FMatrix DecalToWorldValue = ComponentTrans.ToMatrixWithScale();
			
			// 1,1,1 requires no scale
			//			DecalToWorldValue = DecalToWorldValue.GetScaled(GDefaultDecalSize);

			SetShaderValue(RHICmdList, ShaderRHI, DecalToWorld, DecalToWorldValue);
		}

		SetShaderValue(RHICmdList, ShaderRHI, WorldToDecal, WorldToComponent);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FMaterialShader::Serialize(Ar);
		Ar << ScreenToDecal << DecalToWorld << WorldToDecal;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter ScreenToDecal;
	FShaderParameter DecalToWorld;
	FShaderParameter WorldToDecal;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FDeferredDecalPS,TEXT("DeferredDecal"),TEXT("MainPS"),SF_Pixel);

/**
* Static vertex buffer for a unit sized cube. Used to draw the frustum for deferred decals.
*/
class FUnitCubeVertexBuffer : public FVertexBuffer
{
public:
	/**
	* Initialize the RHI for this rendering resource
	*/
	void InitRHI() override
	{
		const int32 NumVerts = 8;
		TResourceArray<FVector4, VERTEXBUFFER_ALIGNMENT> Verts;
		Verts.SetNumUninitialized(NumVerts);

		for (uint32 Z = 0; Z < 2; Z++)
		{
			for (uint32 Y = 0; Y < 2; Y++)
			{
				for (uint32 X = 0; X < 2; X++)
				{
					const FVector4 Vertex = FVector4(
					  (X ? -1 : 1),
					  (Y ? -1 : 1),
					  (Z ? -1 : 1),
					  1.0f
					);

					Verts[GetCubeVertexIndex(X, Y, Z)] = Vertex;
				}
			}
		}

		uint32 Size = Verts.GetResourceDataSize();

		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&Verts);
		VertexBufferRHI = RHICreateVertexBuffer(Size, BUF_Static, CreateInfo);
	}
};

/** 
* Unit cube index buffer
*/
class FUnitCubeIndexBuffer : public FIndexBuffer
{
public:
	/**
	* Initialize the RHI for this rendering resource
	*/
	void InitRHI() override
	{
		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> Indices;
		
		NumIndices = ARRAY_COUNT(GCubeIndices);
		Indices.AddUninitialized(NumIndices);
		FMemory::Memcpy(Indices.GetData(), GCubeIndices, NumIndices * sizeof(uint16));

		const uint32 Size = Indices.GetResourceDataSize();
		const uint32 Stride = sizeof(uint16);

		// Create index buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&Indices);
		IndexBufferRHI = RHICreateIndexBuffer(Stride, Size, BUF_Static, CreateInfo);
	}

	int32 GetIndexCount() const
	{
		return NumIndices;
	};

private:
	int32 NumIndices;
};

static TGlobalResource<FUnitCubeVertexBuffer> GUnitCubeVertexBuffer;
static TGlobalResource<FUnitCubeIndexBuffer> GUnitCubeIndexBuffer;

void SetShader(const FRenderingCompositePassContext& Context, bool bShaderComplexity, const FTransientDecalRenderData& DecalData, FShader* VertexShader)
{
	const FSceneView& View = Context.View;

	const FMaterialShaderMap* MaterialShaderMap = DecalData.MaterialResource->GetRenderingThreadShaderMap();

	auto PixelShader = MaterialShaderMap->GetShader<FDeferredDecalPS>();

	if(bShaderComplexity)
	{
		const FViewInfo &ViewInfo = (const FViewInfo &)View;

		TShaderMapRef<FShaderComplexityAccumulatePS> VisualizePixelShader(ViewInfo.ShaderMap);
		const uint32 NumPixelShaderInstructions = PixelShader->GetNumInstructions();
		const uint32 NumVertexShaderInstructions = VertexShader->GetNumInstructions();

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(Context.RHICmdList, Context.GetFeatureLevel(), BoundShaderState, GetVertexDeclarationFVector3(), VertexShader, *VisualizePixelShader);

		VisualizePixelShader->SetParameters(Context.RHICmdList, NumVertexShaderInstructions, NumPixelShaderInstructions, View.GetFeatureLevel());
	}
	else
	{
		// first Bind, then SetParameters()
		Context.RHICmdList.SetLocalBoundShaderState(Context.RHICmdList.BuildLocalBoundShaderState(GetVertexDeclarationFVector3(), VertexShader->GetVertexShader(), FHullShaderRHIRef(), FDomainShaderRHIRef(), PixelShader->GetPixelShader(), FGeometryShaderRHIRef()));

		PixelShader->SetParameters(Context.RHICmdList, View, DecalData.MaterialProxy, *DecalData.DecalProxy);
 	}
}

bool RenderPreStencil(FRenderingCompositePassContext& Context, const FMaterialShaderMap* MaterialShaderMap, const FMatrix& ComponentToWorldMatrix, const FMatrix& FrustumComponentToClip)
{
	const FSceneView& View = Context.View;

	float Distance = (View.ViewMatrices.ViewOrigin - ComponentToWorldMatrix.GetOrigin()).Size();
	float Radius = ComponentToWorldMatrix.GetMaximumAxisScale();

	// if not inside
	if(Distance > Radius)
	{
		float EstimatedDecalSize = Radius / Distance;
		
		float StencilSizeThreshold = CVarStencilSizeThreshold.GetValueOnRenderThread();

		// Check if it's large enough on screen
		if(EstimatedDecalSize < StencilSizeThreshold)
		{
			return false;
		}
	}

	TShaderMapRef<FDeferredDecalVS> VertexShader(Context.GetShaderMap());
	
	Context.RHICmdList.SetLocalBoundShaderState(Context.RHICmdList.BuildLocalBoundShaderState(GetVertexDeclarationFVector3(), VertexShader->GetVertexShader(), FHullShaderRHIRef(), FDomainShaderRHIRef(), NULL, FGeometryShaderRHIRef()));

	VertexShader->SetParameters(Context.RHICmdList, View, FrustumComponentToClip);

	// Set states, the state cache helps us avoiding redundant sets
	Context.RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());

	// all the same to have DX10 working
	Context.RHICmdList.SetBlendState(TStaticBlendState<
		CW_NONE, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Emissive
		CW_NONE, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Normal
		CW_NONE, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One,	// Metallic, Specular, Roughness
		CW_NONE, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,		BO_Add, BF_Zero, BF_One		// BaseColor
	>::GetRHI() );

	// Carmack's reverse on the bounds
	Context.RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
		false,CF_LessEqual,
		true,CF_Equal,SO_Keep,SO_Keep,SO_Increment,
		true,CF_Equal,SO_Keep,SO_Keep,SO_Decrement,
		0x80,0x7f
	>::GetRHI());

	// Render decal mask
	Context.RHICmdList.DrawIndexedPrimitive(GUnitCubeIndexBuffer.IndexBufferRHI, PT_TriangleList, 0, 0, 8, 0, GUnitCubeIndexBuffer.GetIndexCount() / 3, 1);

	return true;
}

bool IsDBufferEnabled()
{
	static auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DBuffer"));

	return CVar->GetValueOnRenderThread() > 0;
}


static EDecalRasterizerState ComputeDecalRasterizerState(bool bInsideDecal, const FViewInfo& View)
{
	bool bClockwise = bInsideDecal;

	if(View.bReverseCulling)
	{
		bClockwise = !bClockwise;
	}

	return bClockwise ? DRS_CW : DRS_CCW;
}

FRCPassPostProcessDeferredDecals::FRCPassPostProcessDeferredDecals(EDecalRenderStage InDecalRenderStage)
	: DecalRenderStage(InDecalRenderStage)
{
}

static FDecalDepthState ComputeDecalDepthState(EDecalBlendMode DecalBlendMode, bool bInsideDecal, bool bStencilDecalsInThisStage, bool bThisDecalUsesStencil)
{
	FDecalDepthState Ret;

	Ret.bDepthOutput = DecalBlendMode == DBM_Volumetric_DistanceFunction;
				
	if(Ret.bDepthOutput)
	{
		// can be made one enum
		Ret.DepthTest = DDS_DepthTest;
		return Ret;
	}

	if(bInsideDecal)
	{
		// Render backfaces with depth tests disabled since the camera is inside (or close to inside)
		if(bStencilDecalsInThisStage)
		{
			// Enable stencil testing, only write to pixels with stencil of 0
			if(bThisDecalUsesStencil)
			{
				Ret.DepthTest = DDS_DepthAlways_StencilEqual1Write;
			}
			else
			{
				Ret.DepthTest = DDS_DepthAlways_StencilEqual0;
			}
		}
		else
		{
			Ret.DepthTest = DDS_Always;
		}
	}
	else
	{
		// Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside
		if(bStencilDecalsInThisStage)
		{
			// Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside
			// Enable stencil testing, only write to pixels with stencil of 0
			if(bThisDecalUsesStencil)
			{
				Ret.DepthTest = DDS_DepthTest_StencilEqual1Write;
			}
			else
			{
				Ret.DepthTest = DDS_DepthTest_StencilEqual0;
			}
		}
		else
		{
			Ret.DepthTest = DDS_DepthTest;
		}
	}

	return Ret;
}

static void SetDecalDepthState(FDecalDepthState DecalDepthState, FRHICommandListImmediate& RHICmdList)
{
	switch(DecalDepthState.DepthTest)
	{
		case DDS_DepthAlways_StencilEqual1Write:
			check(!DecalDepthState.bDepthOutput);			// todo
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
				false,CF_Always,
				true,CF_Equal,SO_Zero,SO_Zero,SO_Zero,
				true,CF_Equal,SO_Zero,SO_Zero,SO_Zero,
				0xff, 0x7f>::GetRHI(), 1);
			break;

		case DDS_DepthAlways_StencilEqual0:
			check(!DecalDepthState.bDepthOutput);			// todo
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
				false,CF_Always,
				true,CF_Equal,SO_Keep,SO_Keep,SO_Keep,
				false,CF_Always,SO_Keep,SO_Keep,SO_Keep,
				0x80,0x00>::GetRHI(), 0);
			break;

		case DDS_Always:
			if(DecalDepthState.bDepthOutput)
			{
				RHICmdList.SetDepthStencilState(TStaticDepthStencilState<true, CF_Always, true>::GetRHI(), 0);
			}
			else
			{
				RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always, true>::GetRHI(), 0);
			}
			break;

		case DDS_DepthTest_StencilEqual1Write:
			check(!DecalDepthState.bDepthOutput);			// todo
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
				false,CF_DepthNearOrEqual,
				true,CF_Equal,SO_Zero,SO_Zero,SO_Zero,
				true,CF_Equal,SO_Zero,SO_Zero,SO_Zero,
				0xff, 0x7f>::GetRHI(), 1);
			break;

		case DDS_DepthTest_StencilEqual0:
			check(!DecalDepthState.bDepthOutput);			// todo
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
				false,CF_DepthNearOrEqual,
				true,CF_Equal,SO_Keep,SO_Keep,SO_Keep,
				false,CF_Always,SO_Keep,SO_Keep,SO_Keep,
				0x80,0x00>::GetRHI(), 0);
			break;

		case DDS_DepthTest:
			if(DecalDepthState.bDepthOutput)
			{
				RHICmdList.SetDepthStencilState(TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI(), 0);
			}
			else
			{
				RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI(), 0);
			}
			break;

		default:
			check(0);
	}
}

static void SetDecalRasterizerState(EDecalRasterizerState DecalRasterizerState, FRHICommandList& RHICmdList)
{
	switch (DecalRasterizerState)
	{
		case DRS_CW: RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI()); break;
		case DRS_CCW: RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI()); break;
		default: check(0);
	}
}

void FRCPassPostProcessDeferredDecals::Process(FRenderingCompositePassContext& Context)
{
	FRHICommandListImmediate& RHICmdList = Context.RHICmdList;
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	const bool bShaderComplexity = Context.View.Family->EngineShowFlags.ShaderComplexity;
	const bool bDBuffer = IsDBufferEnabled();
	const bool bStencilSizeThreshold = CVarStencilSizeThreshold.GetValueOnRenderThread() >= 0;

	SCOPED_DRAW_EVENT(RHICmdList, PostProcessDeferredDecals);

	enum EDecalResolveBufferIndex
	{
		SceneColorIndex,
		GBufferAIndex,
		GBufferBIndex,
		GBufferCIndex,
		DBufferAIndex,
		DBufferBIndex,
		DBufferCIndex,
		ResolveBufferMax,
	};

	FTextureRHIParamRef TargetsToResolve[ResolveBufferMax] = { nullptr };

	if(DecalRenderStage == DRS_BeforeBasePass)
	{
		// before BasePass, only if DBuffer is enabled

		check(bDBuffer);

		// DBuffer: Decal buffer
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(SceneContext.GBufferA->GetDesc().Extent, 
			PF_B8G8R8A8, 
			TexCreate_None, 
			TexCreate_ShaderResource | TexCreate_RenderTargetable,
			false));

		if(!SceneContext.DBufferA)
		{
			GRenderTargetPool.FindFreeElement(Desc, SceneContext.DBufferA, TEXT("DBufferA"));
		}

		if(!SceneContext.DBufferB)
		{
			GRenderTargetPool.FindFreeElement(Desc, SceneContext.DBufferB, TEXT("DBufferB"));
		}

		Desc.Format = PF_R8G8;

		if(!SceneContext.DBufferC)
		{
			GRenderTargetPool.FindFreeElement(Desc, SceneContext.DBufferC, TEXT("DBufferC"));
		}

		// we assume views are non overlapping, then we need to clear only once in the beginning, otherwise we would need to set scissor rects
		// and don't get FastClear any more.
		bool bFirstView = Context.View.Family->Views[0] == &Context.View;

		if(bFirstView)
		{
			SCOPED_DRAW_EVENT(RHICmdList, DBufferClear);

			
			FRHIRenderTargetView RenderTargets[3];
			RenderTargets[0] = FRHIRenderTargetView(SceneContext.DBufferA->GetRenderTargetItem().TargetableTexture, 0, -1, ERenderTargetLoadAction::EClear, ERenderTargetStoreAction::EStore);
			RenderTargets[1] = FRHIRenderTargetView(SceneContext.DBufferB->GetRenderTargetItem().TargetableTexture, 0, -1, ERenderTargetLoadAction::EClear, ERenderTargetStoreAction::EStore);
			RenderTargets[2] = FRHIRenderTargetView(SceneContext.DBufferC->GetRenderTargetItem().TargetableTexture, 0, -1, ERenderTargetLoadAction::EClear, ERenderTargetStoreAction::EStore);

			FRHIDepthRenderTargetView DepthView(SceneContext.GetSceneDepthSurface(), ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::ENoAction, ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::ENoAction, FExclusiveDepthStencil(FExclusiveDepthStencil::DepthRead_StencilWrite));

			FRHISetRenderTargetsInfo Info(3, RenderTargets, DepthView);
			Info.ClearColors[0] = FLinearColor(0, 0, 0, 1);
			Info.ClearColors[1] = FLinearColor(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1);
			Info.ClearColors[2] = FLinearColor(0, 1, 0, 1);

			RHICmdList.SetRenderTargetsAndClear(Info);

			TargetsToResolve[DBufferAIndex] = SceneContext.DBufferA->GetRenderTargetItem().TargetableTexture;
			TargetsToResolve[DBufferBIndex] = SceneContext.DBufferB->GetRenderTargetItem().TargetableTexture;
			TargetsToResolve[DBufferCIndex] = SceneContext.DBufferC->GetRenderTargetItem().TargetableTexture;
		}
	}

	// this cast is safe as only the dedicated server implements this differently and this pass should not be executed on the dedicated server
	const FViewInfo& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);
	FScene& Scene = *(FScene*)ViewFamily.Scene;

	if(!Scene.Decals.Num())
	{
		// to avoid the stats showing up
		return;
	}

	TArray<FTransientDecalRenderData, SceneRenderingAllocator> SortedDecals;
	SortedDecals.Empty(Scene.Decals.Num());

	// Build a list of decals that need to be rendered for this view in SortedDecals
	for (TSparseArray<FDeferredDecalProxy*>::TConstIterator It(Scene.Decals); It; ++It)
	{
		FDeferredDecalProxy* DecalProxy = *It;
		bool bIsShown = true;

		// Handle the decal actor having bHidden set when we are in the editor, in G mode
#if WITH_EDITOR
		if (View.Family->EngineShowFlags.Editor)
#endif
		{
			if (!DecalProxy->DrawInGame)
			{
				bIsShown = false;
			}
		}

		const FMatrix ComponentToWorldMatrix = DecalProxy->ComponentTrans.ToMatrixWithScale();

		// can be optimized as we test against a sphere around the box instead of the box itself
		const float ConservativeRadius = FMath::Sqrt(
				ComponentToWorldMatrix.GetScaledAxis(EAxis::X).SizeSquared() * FMath::Square(GDefaultDecalSize.X) +
				ComponentToWorldMatrix.GetScaledAxis(EAxis::Y).SizeSquared() * FMath::Square(GDefaultDecalSize.Y) +
				ComponentToWorldMatrix.GetScaledAxis(EAxis::Z).SizeSquared() * FMath::Square(GDefaultDecalSize.Z));

		// can be optimized as the test is too conservative (sphere instead of OBB)
			if (ConservativeRadius < SMALL_NUMBER || !View.ViewFrustum.IntersectSphere(ComponentToWorldMatrix.GetOrigin(), ConservativeRadius))
		{
			bIsShown = false;
		}

		if (bIsShown)
		{
			FTransientDecalRenderData Data(Scene, DecalProxy);

			EDecalRenderStage LocalDecalRenderStage = ComputeRenderStage(Data.DecalBlendMode);

			// we could do this test earlier to avoid the decal intersection but getting DecalBlendMode also costs
			if (Context.View.Family->EngineShowFlags.ShaderComplexity || DecalRenderStage == LocalDecalRenderStage)
			{
				SortedDecals.Add(Data);
			}
		}
	}

	if (SortedDecals.Num() > 0)
	{
		FIntRect SrcRect = View.ViewRect;
		FIntRect DestRect = View.ViewRect;
		
		bool bStencilDecalsInThisStage = true;

#if DBUFFER_DONT_USE_STENCIL_YET
		if(DecalRenderStage != DRS_BeforeLighting)
		{
			bStencilDecalsInThisStage = false;
		}
#endif

		// Setup a stencil mask to prevent certain pixels from receiving deferred decals
		if(bStencilDecalsInThisStage)
		{
			StencilDecalMask(RHICmdList, View);
		}

		// Sort by sort order to allow control over composited result
		// Then sort decals by state to reduce render target switches
		// Also sort by component since Sort() is not stable
		struct FCompareFTransientDecalRenderData
		{
				FORCEINLINE bool operator()(const FTransientDecalRenderData& A, const FTransientDecalRenderData& B) const
			{
				if (B.DecalProxy->SortOrder != A.DecalProxy->SortOrder)
				{ 
					return A.DecalProxy->SortOrder < B.DecalProxy->SortOrder;
				}
				if (B.DecalBlendMode != A.DecalBlendMode)
				{
					return (int32)B.DecalBlendMode < (int32)A.DecalBlendMode;
				}
				if (B.bHasNormal != A.bHasNormal)
				{
					return B.bHasNormal < A.bHasNormal;
				}
				// Batch decals with the same material together
					if (B.MaterialProxy != A.MaterialProxy)
				{
					return B.MaterialProxy < A.MaterialProxy;
				}
				return (PTRINT)B.DecalProxy->Component < (PTRINT)A.DecalProxy->Component;
			}
		};

		// Sort decals by blend mode to reduce render target switches
		SortedDecals.Sort(FCompareFTransientDecalRenderData());

		// optimization to have less state changes
		EDecalRasterizerState LastDecalRasterizerState = DRS_Undefined;
		FDecalDepthState LastDecalDepthState;
		int32 LastDecalBlendMode = -1;
		int32 LastDecalHasNormal = -1; // Decal state can change based on its normal property.(SM5)
		ERenderTargetMode LastRenderTargetMode = RTM_Unknown;
		const ERHIFeatureLevel::Type SMFeatureLevel = Context.GetFeatureLevel();

		SCOPED_DRAW_EVENT(RHICmdList, Decals);
		INC_DWORD_STAT_BY(STAT_Decals, SortedDecals.Num());

	

		for (int32 DecalIndex = 0, DecalCount = SortedDecals.Num(); DecalIndex < DecalCount; DecalIndex++)
		{
			const FTransientDecalRenderData& DecalData = SortedDecals[DecalIndex];
			const FDeferredDecalProxy& DecalProxy = *DecalData.DecalProxy;
			const FMatrix ComponentToWorldMatrix = DecalProxy.ComponentTrans.ToMatrixWithScale();

			// Set vertex shader params
			const FMaterialShaderMap* MaterialShaderMap = DecalData.MaterialResource->GetRenderingThreadShaderMap();
			
			FScaleMatrix DecalScaleTransform(GDefaultDecalSize);
			FTranslationMatrix PreViewTranslation(View.ViewMatrices.PreViewTranslation);
			FMatrix FrustumComponentToClip = DecalScaleTransform * ComponentToWorldMatrix * PreViewTranslation * View.ViewMatrices.TranslatedViewProjectionMatrix;

			// can be optimized as we test against a sphere around the box instead of the box itself
			const float ConservativeRadius = FMath::Sqrt(
					ComponentToWorldMatrix.GetScaledAxis(EAxis::X).SizeSquared() * FMath::Square(GDefaultDecalSize.X) +
					ComponentToWorldMatrix.GetScaledAxis(EAxis::Y).SizeSquared() * FMath::Square(GDefaultDecalSize.Y) +
					ComponentToWorldMatrix.GetScaledAxis(EAxis::Z).SizeSquared() * FMath::Square(GDefaultDecalSize.Z));

			EDecalBlendMode DecalBlendMode = DecalData.DecalBlendMode;

			bool bStencilThisDecal = bStencilDecalsInThisStage;
			
#if DBUFFER_DONT_USE_STENCIL_YET
			if(ComputeRenderStage(DecalBlendMode) != DRS_BeforeLighting)
			{
				bStencilThisDecal = false;
			}
#endif				

			ERenderTargetMode CurrentRenderTargetMode = ComputeRenderTargetMode(DecalBlendMode);

			if (bShaderComplexity)
			{
				CurrentRenderTargetMode = RTM_SceneColor;
				// we want additive blending for the ShaderComplexity mode
				DecalBlendMode = DBM_Emissive;
			}

			// fewer rendertarget switches if possible
			if (CurrentRenderTargetMode != LastRenderTargetMode)
			{
				LastRenderTargetMode = CurrentRenderTargetMode;

				switch (CurrentRenderTargetMode)
				{
					case RTM_SceneColorAndGBuffer:
						{							
							TargetsToResolve[SceneColorIndex] = SceneContext.GetSceneColor()->GetRenderTargetItem().TargetableTexture;
							TargetsToResolve[GBufferAIndex] = SceneContext.GBufferA->GetRenderTargetItem().TargetableTexture;
							TargetsToResolve[GBufferBIndex] = SceneContext.GBufferB->GetRenderTargetItem().TargetableTexture;
							TargetsToResolve[GBufferCIndex] = SceneContext.GBufferC->GetRenderTargetItem().TargetableTexture;
							
							SetRenderTargets(RHICmdList, 4, TargetsToResolve, SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);
						}
						break;

					case RTM_SceneColorAndGBufferDepthWrite:
						{							
							TargetsToResolve[SceneColorIndex] = SceneContext.GetSceneColor()->GetRenderTargetItem().TargetableTexture;
							TargetsToResolve[GBufferAIndex] = SceneContext.GBufferA->GetRenderTargetItem().TargetableTexture;
							TargetsToResolve[GBufferBIndex] = SceneContext.GBufferB->GetRenderTargetItem().TargetableTexture;
							TargetsToResolve[GBufferCIndex] = SceneContext.GBufferC->GetRenderTargetItem().TargetableTexture;
							
							SetRenderTargets(RHICmdList, 4, TargetsToResolve, SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthWrite_StencilWrite);
						}
						break;

					case RTM_GBufferNormal:
						TargetsToResolve[GBufferAIndex] = SceneContext.GBufferA->GetRenderTargetItem().TargetableTexture;
						SetRenderTarget(RHICmdList, TargetsToResolve[GBufferAIndex], SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);
						break;
					
					case RTM_SceneColor:
						TargetsToResolve[SceneColorIndex] = SceneContext.GetSceneColor()->GetRenderTargetItem().TargetableTexture;
						SetRenderTarget(RHICmdList, TargetsToResolve[SceneColorIndex], SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);
						break;

					case RTM_DBuffer:
						{							
							TargetsToResolve[DBufferAIndex] = SceneContext.DBufferA->GetRenderTargetItem().TargetableTexture;
							TargetsToResolve[DBufferBIndex] = SceneContext.DBufferB->GetRenderTargetItem().TargetableTexture;
							TargetsToResolve[DBufferCIndex] = SceneContext.DBufferC->GetRenderTargetItem().TargetableTexture;
							SetRenderTargets(RHICmdList, 3, &TargetsToResolve[DBufferAIndex], SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthRead_StencilWrite);
						}
						break;

					default:
						check(0);	
						break;
				}
				Context.SetViewportAndCallRHI(DestRect);

				// we need to reset the stream source after any call to SetRenderTarget (at least for Metal, which doesn't queue up VB assignments)
				RHICmdList.SetStreamSource(0, GUnitCubeVertexBuffer.VertexBufferRHI, sizeof(FVector4), 0);
			}

			bool bThisDecalUsesStencil = false;

			if (bStencilThisDecal)
			{
				if (bStencilSizeThreshold)
				{
					// note this is after a SetStreamSource (in if CurrentRenderTargetMode != LastRenderTargetMode) call as it needs to get the VB input
					bThisDecalUsesStencil = RenderPreStencil(Context, MaterialShaderMap, ComponentToWorldMatrix, FrustumComponentToClip);

					LastDecalRasterizerState = DRS_Undefined;
					LastDecalDepthState = FDecalDepthState();
					LastDecalBlendMode = -1;
				}
			}

			const bool bBlendStateChange = DecalBlendMode != LastDecalBlendMode;// Has decal mode changed.
			const bool bDecalNormalChanged = GSupportsSeparateRenderTargetBlendState && // has normal changed for SM5 stain/translucent decals?
							(DecalBlendMode == DBM_Translucent || DecalBlendMode == DBM_Stain) &&
							(int32)DecalData.bHasNormal != LastDecalHasNormal;

			// fewer blend state changes if possible
			if (bBlendStateChange || bDecalNormalChanged)
			{
				LastDecalBlendMode = DecalBlendMode;
				LastDecalHasNormal = (int32)DecalData.bHasNormal;

				SetDecalBlendState(RHICmdList, SMFeatureLevel, DecalRenderStage, (EDecalBlendMode)LastDecalBlendMode, DecalData.bHasNormal);
			}

				// todo
//			const int32 IsInsideDecal = ((FVector)View.ViewMatrices.ViewOrigin - ComponentToWorldMatrix.GetOrigin()).SizeSquared() < FMath::Square(ConservativeRadius * 1.05f + View.NearClippingDistance * 2.0f) + ( bThisDecalUsesStencil ) ? 2 : 0;
			const bool bInsideDecal = ((FVector)View.ViewMatrices.ViewOrigin - ComponentToWorldMatrix.GetOrigin()).SizeSquared() < FMath::Square(ConservativeRadius * 1.05f + View.NearClippingDistance * 2.0f);
//			const bool bInsideDecal =  !(IsInsideDecal & 1);

			// update rasterizer state if needed
			{
				EDecalRasterizerState DecalRasterizerState = ComputeDecalRasterizerState(bInsideDecal, View);

				if(LastDecalRasterizerState != DecalRasterizerState)
				{
					LastDecalRasterizerState = DecalRasterizerState;
					SetDecalRasterizerState(DecalRasterizerState, RHICmdList);
				}
			}

			// update DepthStencil state if needed
			{
				FDecalDepthState DecalDepthState = ComputeDecalDepthState(DecalBlendMode, bInsideDecal, bStencilDecalsInThisStage, bThisDecalUsesStencil);

				if(LastDecalDepthState != DecalDepthState)
				{
					LastDecalDepthState = DecalDepthState;
					SetDecalDepthState(DecalDepthState, RHICmdList);
				}
			}

			TShaderMapRef<FDeferredDecalVS> VertexShader(Context.GetShaderMap());
			SetShader(Context, bShaderComplexity, DecalData, *VertexShader);

			VertexShader->SetParameters(RHICmdList, View, FrustumComponentToClip);

			RHICmdList.DrawIndexedPrimitive(GUnitCubeIndexBuffer.IndexBufferRHI, PT_TriangleList, 0, 0, 8, 0, GUnitCubeIndexBuffer.GetIndexCount() / 3, 1);
		}

		// we don't modify stencil but if out input was having stencil for us (after base pass - we need to clear)
		// Clear stencil to 0, which is the assumed default by other passes
		RHICmdList.Clear(false, FLinearColor::White, false, (float)ERHIZBuffer::FarPlane, true, 0, FIntRect());
		
		if(DecalRenderStage == DRS_BeforeBasePass)
		{
			// before BasePass
			GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SceneContext.DBufferA);
			GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SceneContext.DBufferB);
			GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SceneContext.DBufferC);
		}
	}

	// resolve the targets we wrote to.
	FResolveParams ResolveParams;
	for (int32 i = 0; i < ResolveBufferMax; ++i)
	{
		if (TargetsToResolve[i])
		{
			RHICmdList.CopyToResolveTarget(TargetsToResolve[i], TargetsToResolve[i], true, ResolveParams);
		}
	}		
}

FPooledRenderTargetDesc FRCPassPostProcessDeferredDecals::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	// This pass creates it's own output so the compositing graph output isn't needed.
	FPooledRenderTargetDesc Ret;
	
	Ret.DebugName = TEXT("DeferredDecals");

	return Ret;
}
