// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperTileMapRenderSceneProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodySetup2D.h"
#include "EngineGlobals.h"
#include "SceneManagement.h"
#include "Engine/Engine.h"
#include "PaperTileMap.h"
#include "PaperTileLayer.h"
#include "PaperTileMapComponent.h"

DECLARE_CYCLE_STAT(TEXT("Tile Map Proxy"), STAT_TileMap_GetDynamicMeshElements, STATGROUP_Paper2D);

//////////////////////////////////////////////////////////////////////////
// FPaperTileMapRenderSceneProxy

FPaperTileMapRenderSceneProxy::FPaperTileMapRenderSceneProxy(const UPaperTileMapComponent* InComponent)
	: FPaperRenderSceneProxy(InComponent)
	, TileMap(nullptr)
#if WITH_EDITOR
	, bShowPerTileGrid(false)
	, bShowPerLayerGrid(false)
	, bShowOutlineWhenUnselected(false)
#endif
{
	check(InComponent);

	WireframeColor = InComponent->GetWireframeColor();
	TileMap = InComponent->TileMap;
	Material = InComponent->GetMaterial(0);
	MaterialRelevance = InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel());

#if WITH_EDITORONLY_DATA
	bShowPerTileGrid = InComponent->bShowPerTileGridWhenSelected;
	bShowPerLayerGrid = InComponent->bShowPerLayerGridWhenSelected;
	bShowOutlineWhenUnselected = InComponent->bShowOutlineWhenUnselected;
#endif
}

void FPaperTileMapRenderSceneProxy::DrawBoundsForLayer(FPrimitiveDrawInterface* PDI, const FLinearColor& Color, int32 LayerIndex) const
{
	// Slight depth bias so that the wireframe grid overlay doesn't z-fight with the tiles themselves
	const float DepthBias = 0.0001f;

	const FMatrix& LocalToWorld = GetLocalToWorld();
	const FVector TL(LocalToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(0, 0, LayerIndex)));
	const FVector TR(LocalToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(TileMap->MapWidth, 0, LayerIndex)));
	const FVector BL(LocalToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(0, TileMap->MapHeight, LayerIndex)));
	const FVector BR(LocalToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(TileMap->MapWidth, TileMap->MapHeight, LayerIndex)));

	PDI->DrawLine(TL, TR, Color, SDPG_Foreground, 0.0f, DepthBias);
	PDI->DrawLine(TR, BR, Color, SDPG_Foreground, 0.0f, DepthBias);
	PDI->DrawLine(BR, BL, Color, SDPG_Foreground, 0.0f, DepthBias);
	PDI->DrawLine(BL, TL, Color, SDPG_Foreground, 0.0f, DepthBias);
}

void FPaperTileMapRenderSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	SCOPE_CYCLE_COUNTER(STAT_TileMap_GetDynamicMeshElements);
	checkSlow(IsInRenderingThread());

	// Slight depth bias so that the wireframe grid overlay doesn't z-fight with the tiles themselves
	const float DepthBias = 0.0001f;

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			// Draw the tile maps
			//@TODO: RenderThread race condition
			if (TileMap != nullptr)
			{
				if ((View->Family->EngineShowFlags.Collision /*@TODO: && bIsCollisionEnabled*/) && AllowDebugViewmodes())
				{
					if (UBodySetup2D* BodySetup2D = Cast<UBodySetup2D>(TileMap->BodySetup))
					{
						//@TODO: Draw 2D debugging geometry
					}
					else if (UBodySetup* BodySetup = TileMap->BodySetup)
					{
						if (FMath::Abs(GetLocalToWorld().Determinant()) < SMALL_NUMBER)
						{
							// Catch this here or otherwise GeomTransform below will assert
							// This spams so commented out
							//UE_LOG(LogStaticMesh, Log, TEXT("Zero scaling not supported (%s)"), *StaticMesh->GetPathName());
						}
						else
						{
							// Make a material for drawing solid collision stuff
							const UMaterial* LevelColorationMaterial = View->Family->EngineShowFlags.Lighting
								? GEngine->ShadedLevelColorationLitMaterial : GEngine->ShadedLevelColorationUnlitMaterial;

							auto CollisionMaterialInstance = new FColoredMaterialRenderProxy(
								LevelColorationMaterial->GetRenderProxy(IsSelected(), IsHovered()),
								WireframeColor
								);

							// Draw the static mesh's body setup.

							// Get transform without scaling.
							FTransform GeomTransform(GetLocalToWorld());

							// In old wireframe collision mode, always draw the wireframe highlighted (selected or not).
							bool bDrawWireSelected = IsSelected();
							if (View->Family->EngineShowFlags.Collision)
							{
								bDrawWireSelected = true;
							}

							// Differentiate the color based on bBlockNonZeroExtent.  Helps greatly with skimming a level for optimization opportunities.
							FColor CollisionColor = FColor(157, 149, 223, 255);

							const bool bPerHullColor = false;
							const bool bDrawSimpleSolid = false;
							BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(CollisionColor, bDrawWireSelected, IsHovered()), CollisionMaterialInstance, bPerHullColor, bDrawSimpleSolid, UseEditorDepthTest(), ViewIndex, Collector);
						}
					}
				}

				// Draw the bounds
				RenderBounds(PDI, View->Family->EngineShowFlags, GetBounds(), IsSelected());

#if WITH_EDITOR
				const bool bShowAsSelected = IsSelected();
				const bool bEffectivelySelected = bShowAsSelected || IsHovered();

				const uint8 DPG = SDPG_Foreground;//GetDepthPriorityGroup(View);

				// Draw separation wires if selected
				const FLinearColor OverrideColor = GetSelectionColor(FLinearColor::White, bShowAsSelected, IsHovered());

				FTransform LocalToWorld(GetLocalToWorld());

				// Draw the debug outline
				if (bEffectivelySelected)
				{
					const int32 SelectedLayerIndex = TileMap->SelectedLayerIndex;

					if (bShowPerLayerGrid)
					{
						// Draw a bound for every layer but the selected one (and even that one if the per-tile grid is off)
						for (int32 LayerIndex = 0; LayerIndex < TileMap->TileLayers.Num(); ++LayerIndex)
						{
							if ((LayerIndex != SelectedLayerIndex) || !bShowPerTileGrid)
							{
								DrawBoundsForLayer(PDI, OverrideColor, LayerIndex);
							}
						}
					}

					if (bShowPerTileGrid && (SelectedLayerIndex != INDEX_NONE))
					{
						// Draw horizontal lines on the selection
						for (int32 Y = 0; Y <= TileMap->MapHeight; ++Y)
						{
							int32 X = 0;
							const FVector Start(TileMap->GetTilePositionInLocalSpace(X, Y, SelectedLayerIndex));

							X = TileMap->MapWidth;
							const FVector End(TileMap->GetTilePositionInLocalSpace(X, Y, SelectedLayerIndex));

							PDI->DrawLine(LocalToWorld.TransformPosition(Start), LocalToWorld.TransformPosition(End), OverrideColor, DPG, 0.0f, DepthBias);
						}

						// Draw vertical lines
						for (int32 X = 0; X <= TileMap->MapWidth; ++X)
						{
							int32 Y = 0;
							const FVector Start(TileMap->GetTilePositionInLocalSpace(X, Y, SelectedLayerIndex));

							Y = TileMap->MapHeight;
							const FVector End(TileMap->GetTilePositionInLocalSpace(X, Y, SelectedLayerIndex));

							PDI->DrawLine(LocalToWorld.TransformPosition(Start), LocalToWorld.TransformPosition(End), OverrideColor, DPG, 0.0f, DepthBias);
						}
					}
				}
				else if (View->Family->EngineShowFlags.Grid && bShowOutlineWhenUnselected)
				{
					// Draw layer 0 even when not selected, so you can see where the tile map is in the editor
					DrawBoundsForLayer(PDI, WireframeColor, /*LayerIndex=*/ 0);
				}
#endif
			}
		}
	}

	// Draw all of the queued up sprites
	FPaperRenderSceneProxy::GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);
}

