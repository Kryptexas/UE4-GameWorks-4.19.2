// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperTileMapRenderSceneProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodySetup2D.h"

//////////////////////////////////////////////////////////////////////////
// FPaperTileMapRenderSceneProxy

FPaperTileMapRenderSceneProxy::FPaperTileMapRenderSceneProxy(const UPaperTileMapRenderComponent* InComponent)
	: FPaperRenderSceneProxy(InComponent)
	, TileMap(nullptr)
{
	if (const UPaperTileMapRenderComponent* InTileComponent = Cast<const UPaperTileMapRenderComponent>(InComponent))
	{
		TileMap = InTileComponent->TileMap;
		Material = (TileMap != nullptr) ? TileMap->Material : nullptr;

		if (Material)
		{
			MaterialRelevance = Material->GetRelevance(GetScene().GetFeatureLevel());
		}
	}
}

void FPaperTileMapRenderSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FPaperTileMapRenderSceneProxy_GetDynamicMeshElements);
	checkSlow(IsInRenderingThread());

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			// Draw the tile maps
			//@TODO: RenderThread race condition
			if (TileMap != NULL)
			{
				FColor WireframeColor = FColor(0, 255, 255, 255);

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
				// Draw the debug outline
				if (View->Family->EngineShowFlags.Grid)
				{
					const uint8 DPG = SDPG_Foreground;//GetDepthPriorityGroup(View);

					// Draw separation wires if selected
					FLinearColor OverrideColor;
					bool bUseOverrideColor = false;

					const bool bShowAsSelected = !(GIsEditor && View->Family->EngineShowFlags.Selection) || IsSelected();
					if (bShowAsSelected || IsHovered())
					{
						bUseOverrideColor = true;
						OverrideColor = GetSelectionColor(FLinearColor::White, bShowAsSelected, IsHovered());
					}

					FTransform LocalToWorld(GetLocalToWorld());

					const float TW = TileMap->TileWidth;
					const float TH = TileMap->TileHeight;
					//@TODO: PAPER: Tiles probably shouldn't be drawn with their pivot at the center - RE: all the 0.5f in here

					if (bUseOverrideColor)
					{
						// Draw horizontal lines
						for (int32 Y = 0; Y <= TileMap->MapHeight; ++Y)
						{
							int32 X = 0;
							const FVector Start((X - 0.5f) * TW, 0.0f, -(Y - 0.5f) * TH);

							X = TileMap->MapWidth;
							const FVector End((X - 0.5f) * TW, 0.0f, -(Y - 0.5f) * TH);

							PDI->DrawLine(LocalToWorld.TransformPosition(Start), LocalToWorld.TransformPosition(End), OverrideColor, DPG);
						}

						// Draw vertical lines
						for (int32 X = 0; X <= TileMap->MapWidth; ++X)
						{
							int32 Y = 0;
							const FVector Start((X - 0.5f) * TW, 0.0f, -(Y - 0.5f) * TH);

							Y = TileMap->MapHeight;
							const FVector End((X - 0.5f) * TW, 0.0f, -(Y - 0.5f) * TH);

							PDI->DrawLine(LocalToWorld.TransformPosition(Start), LocalToWorld.TransformPosition(End), OverrideColor, DPG);
						}
					}


					// Create a local space bounding box
					const FVector TopLeft(-TW*0.5f, -2.0f, TH*0.5f);
					const FVector Dimensions(TileMap->MapWidth * TW, 4.0f, -TileMap->MapHeight * TH);
					const FBox OutlineBox(TopLeft, TopLeft + Dimensions);

					// Draw it		
					DrawWireBox(PDI, OutlineBox.TransformBy(LocalToWorld), FLinearColor::White, DPG); //@TODO: Paper: Doesn't handle rotation well - need to use DrawOrientedWireBox
				}
#endif
			}
		}
	}

	// Draw all of the queued up sprites
	FPaperRenderSceneProxy::GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);
}

