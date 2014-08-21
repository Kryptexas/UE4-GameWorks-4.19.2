// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CustomDepthRendering.cpp: CustomDepth rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "Engine.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "PostProcessing.h"
#include "RenderingCompositionGraph.h"
#include "SceneFilterRendering.h"
#include "SceneUtils.h"

/*-----------------------------------------------------------------------------
	FCustomDepthPrimSet
-----------------------------------------------------------------------------*/

bool FCustomDepthPrimSet::DrawPrims(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	bool bDirty=false;

	if(Prims.Num())
	{
		SCOPED_DRAW_EVENT(CustomDepth, DEC_SCENE_ITEMS);

		const bool bUseGetMeshElements = ShouldUseGetDynamicMeshElements();

		TDynamicPrimitiveDrawer<FDepthDrawingPolicyFactory> Drawer(RHICmdList, &View, FDepthDrawingPolicyFactory::ContextType(DDM_AllOccluders), true);

		for (int32 PrimIdx = 0; PrimIdx < Prims.Num(); PrimIdx++)
		{
			FPrimitiveSceneProxy* PrimitiveSceneProxy = Prims[PrimIdx];
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSceneProxy->GetPrimitiveSceneInfo();

			if (View.PrimitiveVisibilityMap[PrimitiveSceneInfo->GetIndex()])
			{
				const FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveSceneInfo->GetIndex()];

				if (bUseGetMeshElements)
				{
					FDepthDrawingPolicyFactory::ContextType Context(DDM_AllOccluders);

					for (int32 MeshBatchIndex = 0; MeshBatchIndex < View.DynamicMeshElements.Num(); MeshBatchIndex++)
					{
						const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];

						if (MeshBatchAndRelevance.PrimitiveSceneProxy == PrimitiveSceneProxy)
						{
							const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;
							FDepthDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, Context, MeshBatch, false, true, MeshBatchAndRelevance.PrimitiveSceneProxy, MeshBatch.BatchHitProxyId);
						}
					}
				}
				else if (ViewRelevance.bDynamicRelevance)
				{
					Drawer.SetPrimitive(PrimitiveSceneProxy);
					PrimitiveSceneProxy->DrawDynamicElements(&Drawer, &View);
				}

				if (ViewRelevance.bStaticRelevance)
				{
					for (int32 StaticMeshIdx = 0; StaticMeshIdx < PrimitiveSceneInfo->StaticMeshes.Num(); StaticMeshIdx++)
					{
						const FStaticMesh& StaticMesh = PrimitiveSceneInfo->StaticMeshes[StaticMeshIdx];

						if (View.StaticMeshVisibilityMap[StaticMesh.Id])
						{
							bDirty |= FDepthDrawingPolicyFactory::DrawStaticMesh(
								RHICmdList, 
								View,
								FDepthDrawingPolicyFactory::ContextType(DDM_AllOccluders),
								StaticMesh,
								StaticMesh.Elements.Num() == 1 ? 1 : View.StaticMeshBatchVisibility[StaticMesh.Id],
								true,
								PrimitiveSceneProxy,
								StaticMesh.BatchHitProxyId
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
