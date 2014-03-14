// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CustomDepthRendering.cpp: CustomDepth rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "PostProcessing.h"
#include "RenderingCompositionGraph.h"
#include "SceneFilterRendering.h"

/*-----------------------------------------------------------------------------
	FCustomDepthPrimSet
-----------------------------------------------------------------------------*/

bool FCustomDepthPrimSet::DrawPrims(const FViewInfo* ViewInfo,bool bInitializeOffsets)
{
	bool bDirty=false;

	if(Prims.Num())
	{
		SCOPED_DRAW_EVENT(DistortionAccum, DEC_SCENE_ITEMS);
		TDynamicPrimitiveDrawer<FDepthDrawingPolicyFactory> Drawer(ViewInfo, FDepthDrawingPolicyFactory::ContextType(DDM_AllOccluders), true);

		for (int32 PrimIdx = 0; PrimIdx < Prims.Num(); PrimIdx++)
		{
			FPrimitiveSceneProxy* PrimitiveSceneProxy = Prims[PrimIdx];
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSceneProxy->GetPrimitiveSceneInfo();

			if (ViewInfo->PrimitiveVisibilityMap[PrimitiveSceneInfo->GetIndex()])
			{
				const FPrimitiveViewRelevance& ViewRelevance = ViewInfo->PrimitiveViewRelevanceMap[PrimitiveSceneInfo->GetIndex()];

				if (ViewRelevance.bDynamicRelevance)
				{
					Drawer.SetPrimitive(PrimitiveSceneProxy);
					PrimitiveSceneProxy->DrawDynamicElements(&Drawer, ViewInfo);
				}

				if (ViewRelevance.bStaticRelevance)
				{
					for (int32 StaticMeshIdx = 0; StaticMeshIdx < PrimitiveSceneInfo->StaticMeshes.Num(); StaticMeshIdx++)
					{
						const FStaticMesh& StaticMesh = PrimitiveSceneInfo->StaticMeshes[StaticMeshIdx];

						if (ViewInfo->StaticMeshVisibilityMap[StaticMesh.Id])
						{
							bDirty |= FDepthDrawingPolicyFactory::DrawStaticMesh(
								*ViewInfo,
								FDepthDrawingPolicyFactory::ContextType(DDM_AllOccluders),
								StaticMesh,
								StaticMesh.Elements.Num() == 1 ? 1 : ViewInfo->StaticMeshBatchVisibility[StaticMesh.Id],
								true,
								PrimitiveSceneProxy,
								StaticMesh.HitProxyId
								);
						}
					}
				}
			}
		}

		// Mark dirty if dynamic drawer rendered
		bDirty |= Drawer.IsDirty();
	}

	return bDirty;
}
