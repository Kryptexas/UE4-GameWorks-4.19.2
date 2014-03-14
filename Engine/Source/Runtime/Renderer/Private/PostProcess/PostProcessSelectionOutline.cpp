// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessSelectionOutline.cpp: Post processing outline effect.
=============================================================================*/

#include "RendererPrivate.h"
#include "PostProcessing.h"
#include "SceneFilterRendering.h"
#include "PostProcessSelectionOutline.h"
#include "OneColorShader.h"

///////////////////////////////////////////
// FRCPassPostProcessSelectionOutlineColor
///////////////////////////////////////////

void FRCPassPostProcessSelectionOutlineColor::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessSelectionOutlineBuffer, DEC_SCENE_ITEMS);

	const FPooledRenderTargetDesc* SceneColorInputDesc = GetInputDesc(ePId_Input0);

	if(!SceneColorInputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FViewInfo& View = Context.View;
	FIntRect ViewRect = View.ViewRect;
	FIntPoint SrcSize = SceneColorInputDesc->Extent;

	// Get the output render target
	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);
	
	// Set the render target/viewport.
	RHISetRenderTarget(FTextureRHIParamRef(), DestRenderTarget.TargetableTexture);	
	// This is a reversed Z depth surface, so 0.0f is the far plane.
	RHIClear(false, FLinearColor(0, 0, 0, 0), true, 0.0f, true, 0, FIntRect());
	Context.SetViewportAndCallRHI(ViewRect);

	if(View.VisibleDynamicPrimitives.Num() && View.Family->EngineShowFlags.Selection)
	{
		TDynamicPrimitiveDrawer<FHitProxyDrawingPolicyFactory> Drawer(&View, FHitProxyDrawingPolicyFactory::ContextType(), true, false, false, false, true);
		TMultiMap<FName, const FPrimitiveSceneInfo*> PrimitivesByActor;

		for (int32 PrimitiveIndex = 0; PrimitiveIndex < View.VisibleDynamicPrimitives.Num();PrimitiveIndex++)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives[PrimitiveIndex];

			// Only draw the primitive if relevant
			if(PrimitiveSceneInfo->Proxy->IsSelected())
			{
				PrimitivesByActor.Add(PrimitiveSceneInfo->Proxy->GetOwnerName(), PrimitiveSceneInfo);
			}
		}

		if (PrimitivesByActor.Num())
		{
			// 0 means no object, 1 means BSP so we start with 1
			uint32 StencilValue = 2;

			TShaderMapRef<FOneColorVS> VertexShader(GetGlobalShaderMap());
			TShaderMapRef<FOneColorPS> PixelShader(GetGlobalShaderMap());

			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
			RHISetBlendState(TStaticBlendStateWriteMask<CW_NONE, CW_NONE, CW_NONE, CW_NONE>::GetRHI());

			// Sort by actor
			TArray<FName> Actors;
			PrimitivesByActor.GetKeys(Actors);
			for( TArray<FName>::TConstIterator ActorIt(Actors); ActorIt; ++ActorIt )
			{
				bool bBSP = *ActorIt == NAME_BSP;
				if (bBSP)
				{
					// This is a reversed Z depth surface, using CF_GreaterEqual.
					RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual,true,CF_Always,SO_Keep,SO_Keep,SO_Replace>::GetRHI(),1);
				}
				else
				{
					// This is a reversed Z depth surface, using CF_GreaterEqual.
					RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual,true,CF_Always,SO_Keep,SO_Keep,SO_Replace>::GetRHI(),StencilValue);

					// we want to use 1..255 for all objects, not correct silhouettes after that is acceptable
					StencilValue = (StencilValue == 255) ? 2 : (StencilValue + 1);
				}

				TArray<const FPrimitiveSceneInfo*> Primitives;
				PrimitivesByActor.MultiFind(*ActorIt, Primitives);

				for( TArray<const FPrimitiveSceneInfo*>::TConstIterator PrimIt(Primitives); PrimIt; ++PrimIt )
				{
					const FPrimitiveSceneInfo* PrimitiveSceneInfo = *PrimIt;
					// Render the object to the stencil/depth buffer
					Drawer.SetPrimitive(PrimitiveSceneInfo->Proxy);
					PrimitiveSceneInfo->Proxy->DrawDynamicElements(&Drawer, &View);
				}
			}
		}

		// to get an outline around the objects if it's partly outside of the screen
		{
			FIntRect InnerRect = ViewRect;

			// 1 as we have an outline that is that thick
			InnerRect.InflateRect(-1);

			// We could use Clear with InnerRect but this is just an optimization - on some hardware it might do a full clear (and we cannot disable yet)
			//			RHIClear(false, FLinearColor(0, 0, 0, 0), true, 0.0f, true, 0, InnerRect);
			// so we to 4 clears - one for each border.

			// top
			RHISetScissorRect(true, ViewRect.Min.X, ViewRect.Min.Y, ViewRect.Max.X, InnerRect.Min.Y);
			RHIClear(false, FLinearColor(0, 0, 0, 0), true, 0.0f, true, 0, FIntRect());
			// bottom
			RHISetScissorRect(true, ViewRect.Min.X, InnerRect.Max.Y, ViewRect.Max.X, ViewRect.Max.Y);
			RHIClear(false, FLinearColor(0, 0, 0, 0), true, 0.0f, true, 0, FIntRect());
			// left
			RHISetScissorRect(true, ViewRect.Min.X, ViewRect.Min.Y, InnerRect.Min.X, ViewRect.Max.Y);
			RHIClear(false, FLinearColor(0, 0, 0, 0), true, 0.0f, true, 0, FIntRect());
			// right
			RHISetScissorRect(true, InnerRect.Max.X, ViewRect.Min.Y, ViewRect.Max.X, ViewRect.Max.Y);
			RHIClear(false, FLinearColor(0, 0, 0, 0), true, 0.0f, true, 0, FIntRect());

			RHISetScissorRect(false, 0, 0, 0, 0);
		}
	}
	
	// Resolve to the output
	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessSelectionOutlineColor::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.Reset();

	Ret.Format = PF_DepthStencil;
	Ret.Flags = TexCreate_None;
	Ret.TargetableFlags = TexCreate_DepthStencilTargetable;
	Ret.DebugName = TEXT("SelectionDepthStencil");
	Ret.NumSamples = GSceneRenderTargets.GetEditorMSAACompositingSampleCount();

	return Ret;
}


///////////////////////////////////////////
// FRCPassPostProcessSelectionOutline
///////////////////////////////////////////

/**
 * Pixel shader for rendering the selection outline
 */
template<uint32 MSAASampleCount>
class FPostProcessSelectionOutlinePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessSelectionOutlinePS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		if(!IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5))
		{
			if(MSAASampleCount > 1)
			{
				return false;
			}
		}

		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine( TEXT("MSAA_SAMPLE_COUNT"), MSAASampleCount);
	}

	/** Default constructor. */
	FPostProcessSelectionOutlinePS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter OutlineColor;
	FShaderParameter BSPSelectionIntensity;
	FShaderResourceParameter PostprocessInput1MS;
	FShaderResourceParameter EditorPrimitivesStencil;
	FShaderParameter EditorRenderParams;

	/** Initialization constructor. */
	FPostProcessSelectionOutlinePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		OutlineColor.Bind(Initializer.ParameterMap, TEXT("OutlineColor"));
		BSPSelectionIntensity.Bind(Initializer.ParameterMap, TEXT("BSPSelectionIntensity"));
		PostprocessInput1MS.Bind(Initializer.ParameterMap, TEXT("PostprocessInput1MS"));
		EditorRenderParams.Bind(Initializer.ParameterMap, TEXT("EditorRenderParams"));
		EditorPrimitivesStencil.Bind(Initializer.ParameterMap,TEXT("EditorPrimitivesStencil"));
	}

	void SetPS(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, Context.View);

		DeferredParameters.Set(ShaderRHI, Context.View);

		const FPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;
		const FSceneViewFamily& ViewFamily = *(Context.View.Family);
		FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());

		// PostprocessInput1MS and EditorPrimitivesStencil
		{
			FRenderingCompositeOutputRef* OutputRef = Context.Pass->GetInput(ePId_Input1);

			check(OutputRef);

			FRenderingCompositeOutput* Input = OutputRef->GetOutput();

			check(Input);

			TRefCountPtr<IPooledRenderTarget> InputPooledElement = Input->RequestInput();

			check(InputPooledElement);

			FTexture2DRHIRef& TargetableTexture = (FTexture2DRHIRef&)InputPooledElement->GetRenderTargetItem().TargetableTexture;

			SetTextureParameter(ShaderRHI, PostprocessInput1MS, TargetableTexture);

			// cache the stencil SRV to avoid create calls each frame (the cache element is stored in the state)
			{
				if(ViewState->SelectionOutlineCacheKey != TargetableTexture)
				{
					// release if not the right one (as the internally SRV stores a pointer to the texture we cannot get a false positive)
					ViewState->SelectionOutlineCacheKey.SafeRelease();
					ViewState->SelectionOutlineCacheValue.SafeRelease();
				}

				if(!ViewState->SelectionOutlineCacheValue)
				{
					// create if needed
					ViewState->SelectionOutlineCacheKey = TargetableTexture;
					ViewState->SelectionOutlineCacheValue = RHICreateShaderResourceView(TargetableTexture, 0, 1, PF_X24_G8);
				}
			}

			SetSRVParameter(ShaderRHI, EditorPrimitivesStencil, ViewState->SelectionOutlineCacheValue);
		}

#if WITH_EDITOR
		{
			FLinearColor Value = Context.View.SelectionOutlineColor;

			Value.A = GEngine->SelectionHighlightIntensity;

			SetShaderValue(ShaderRHI, OutlineColor, Value );
			SetShaderValue(ShaderRHI, BSPSelectionIntensity, GEngine->BSPSelectionHighlightIntensity);
		}
#else
		check(!"This shader is not used outside of the Editor.");
#endif

		{
			static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.Editor.MovingPattern"));
		
			FLinearColor Value(0, CVar->GetValueOnRenderThread(), 0, 0);

			if(!ViewFamily.bRealtimeUpdate)
			{
				// no animation if realtime update is disabled
				Value.G = 0;
			}

			SetShaderValue(ShaderRHI, EditorRenderParams, Value );
		}
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << OutlineColor << BSPSelectionIntensity << DeferredParameters << PostprocessInput1MS << EditorPrimitivesStencil << EditorRenderParams;
		return bShaderHasOutdatedParameters;
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("PostProcessSelectionOutline");
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("MainPS");
	}
};

// #define avoids a lot of code duplication
#define VARIATION1(A) typedef FPostProcessSelectionOutlinePS<A> FPostProcessSelectionOutlinePS##A; \
	IMPLEMENT_SHADER_TYPE2(FPostProcessSelectionOutlinePS##A, SF_Pixel);

VARIATION1(1)			VARIATION1(2)			VARIATION1(4)			VARIATION1(8)
#undef VARIATION1


template <uint32 MSAASampleCount>
static void SetSelectionOutlineShaderTempl(const FRenderingCompositePassContext& Context)
{
	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FPostProcessSelectionOutlinePS<MSAASampleCount> > PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetPS(Context);
}

void FRCPassPostProcessSelectionOutline::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessSelectionOutline, DEC_SCENE_ITEMS);
	const FPooledRenderTargetDesc* SceneColorInputDesc = GetInputDesc(ePId_Input0);
	const FPooledRenderTargetDesc* SelectionColorInputDesc = GetInputDesc(ePId_Input1);

	if (!SceneColorInputDesc || !SelectionColorInputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FSceneView& View = Context.View;
	FIntRect ViewRect = View.ViewRect;
	FIntPoint SrcSize = SceneColorInputDesc->Extent;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());	
	RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, ViewRect);
	Context.SetViewportAndCallRHI(ViewRect);

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	const uint32 MSAASampleCount = GSceneRenderTargets.GetEditorMSAACompositingSampleCount();

	if(MSAASampleCount == 1)
	{
		SetSelectionOutlineShaderTempl<1>(Context);
	}
	else if(MSAASampleCount == 2)
	{
		SetSelectionOutlineShaderTempl<2>(Context);
	}
	else if(MSAASampleCount == 4)
	{
		SetSelectionOutlineShaderTempl<4>(Context);
	}
	else if(MSAASampleCount == 8)
	{
		SetSelectionOutlineShaderTempl<8>(Context);
	}
	else
	{
		// not supported, internal error
		check(0);
	}

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		0, 0,
		ViewRect.Width(), ViewRect.Height(),
		ViewRect.Min.X, ViewRect.Min.Y,
		ViewRect.Width(), ViewRect.Height(),
		ViewRect.Size(),
		SrcSize,
		EDRF_UseTriangleOptimization);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessSelectionOutline::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

	Ret.DebugName = TEXT("SelectionComposited");

	return Ret;
}
