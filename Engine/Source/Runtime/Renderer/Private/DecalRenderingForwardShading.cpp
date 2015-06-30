// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DecalRenderingForwardShading.cpp: Decals for forward renderer
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneUtils.h"
#include "DecalRenderingShared.h"

void FForwardShadingSceneRenderer::RenderDecals(FRHICommandListImmediate& RHICmdList)
{
	if (Scene->Decals.Num() == 0)
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_DecalsDrawTime);

	// Setup read access to depth/stencil buffer on PC platform (mobile preview)
	if (IsPCPlatform(ViewFamily.GetShaderPlatform()))
	{
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
		FRHIDepthRenderTargetView	DepthView(SceneContext.GetSceneDepthSurface(), ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::ENoAction, ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::ENoAction, FExclusiveDepthStencil(FExclusiveDepthStencil::DepthRead_StencilRead));
		FRHIRenderTargetView		ColorView(SceneContext.GetSceneColorSurface(), 0, -1, ERenderTargetLoadAction::ENoAction, ERenderTargetStoreAction::EStore);
		FRHISetRenderTargetsInfo	Info(1, &ColorView, DepthView);
		RHICmdList.SetRenderTargetsAndClear(Info);
	}

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];
		const bool bShaderComplexity = View.Family->EngineShowFlags.ShaderComplexity;
		
		// Build a list of decals that need to be rendered for this view
		FTransientDecalRenderDataList SortedDecals;
		FDecalRendering::BuildVisibleDecalList(*Scene, View, DRS_ForwardShading, SortedDecals);

		if (SortedDecals.Num())
		{
			SCOPED_DRAW_EVENT(RHICmdList, DeferredDecals);
			INC_DWORD_STAT_BY(STAT_Decals, SortedDecals.Num());
		
			RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);
			RHICmdList.SetStreamSource(0, GetUnitCubeVertexBuffer(), sizeof(FVector4), 0);

			TOptional<EDecalBlendMode> LastDecalBlendMode;
			TOptional<bool> LastDecalDepthState;
				
			for (int32 DecalIndex = 0, DecalCount = SortedDecals.Num(); DecalIndex < DecalCount; DecalIndex++)
			{
				const FTransientDecalRenderData& DecalData = SortedDecals[DecalIndex];
				const FDeferredDecalProxy& DecalProxy = *DecalData.DecalProxy;
				const FMatrix ComponentToWorldMatrix = DecalProxy.ComponentTrans.ToMatrixWithScale();
				const FMatrix FrustumComponentToClip = FDecalRendering::ComputeComponentToClipMatrix(View, ComponentToWorldMatrix);
						
				const float ConservativeRadius = DecalData.ConservativeRadius;
				const bool bInsideDecal = ((FVector)View.ViewMatrices.ViewOrigin - ComponentToWorldMatrix.GetOrigin()).SizeSquared() < FMath::Square(ConservativeRadius * 1.05f + View.NearClippingDistance * 2.0f);

				if (!LastDecalDepthState.IsSet() || LastDecalDepthState.GetValue() != bInsideDecal)
				{
					LastDecalDepthState = bInsideDecal;
					if (bInsideDecal)
					{
						RHICmdList.SetRasterizerState(View.bReverseCulling ? TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI() : TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI());
						RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false,CF_Always, true, CF_Equal, SO_Keep, SO_Keep, SO_Keep>::GetRHI(), 0);
					}
					else
					{
						RHICmdList.SetRasterizerState(View.bReverseCulling ? TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI() : TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI());
						RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false,CF_DepthNearOrEqual, true, CF_Equal, SO_Keep, SO_Keep, SO_Keep>::GetRHI(), 0);
					}
				}

				if (!LastDecalBlendMode.IsSet() || LastDecalBlendMode.GetValue() != DecalData.DecalBlendMode)
				{
					LastDecalBlendMode = DecalData.DecalBlendMode;
					switch(DecalData.DecalBlendMode)
					{
					case DBM_Translucent:
						RHICmdList.SetBlendState(TStaticBlendState<CW_RGB, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI());
						break;
					case DBM_Stain:
						// Modulate
						RHICmdList.SetBlendState(TStaticBlendState<CW_RGB, BO_Add, BF_DestColor, BF_InverseSourceAlpha>::GetRHI());
						break;
					case DBM_Emissive:
						// Additive
						RHICmdList.SetBlendState(TStaticBlendState<CW_RGB, BO_Add, BF_SourceAlpha, BF_One>::GetRHI());
						break;
					default:
						check(0);
					};
				}

				// Set shader params
				FDecalRendering::SetShader(RHICmdList, View, bShaderComplexity, DecalData, FrustumComponentToClip);
			
				RHICmdList.DrawIndexedPrimitive(GetUnitCubeIndexBuffer(), PT_TriangleList, 0, 0, 8, 0, ARRAY_COUNT(GCubeIndices) / 3, 1);
			}
		}
	}
}
