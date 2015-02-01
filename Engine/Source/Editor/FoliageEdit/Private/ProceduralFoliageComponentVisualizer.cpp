// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "ProceduralFoliageComponentVisualizer.h"
#include "ProceduralFoliage.h"
#include "ProceduralFoliageComponent.h"


static const FColor	ProcTileColor(0,255,0);
static const FColor	ProcTileOverlapColor(255, 255, 0);

void FProceduralFoliageComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UProceduralFoliageComponent* ProcComponent = Cast<const UProceduralFoliageComponent>(Component))
	{
		if (ProcComponent->ProceduralFoliage && ProcComponent->bHideDebugTiles == false)
		{
			const float TileSize = ProcComponent->ProceduralFoliage->TileSize;
			const FVector TileSizeOffset(TileSize, TileSize, 0.f);
			const FVector TileLocation = ProcComponent->GetWorldPosition();
			const FVector OverlapV(ProcComponent->Overlap, ProcComponent->Overlap, 0.f);
			
			int32 XOffset, YOffset;
			int32 NumX = 0;
			int32 NumY = 0;
			float HalfHeight;

			ProcComponent->GetTilesLayout(XOffset, YOffset, NumX, NumY, HalfHeight);

			for (int32 X = 0; X < NumX; ++X)
			{
				for (int32 Y = 0; Y < NumY; ++Y)
				{
					const FVector StartOffset(X*TileSize, Y*TileSize, 0.f);
					
					const FBox BoxInner(TileLocation + StartOffset, TileLocation + StartOffset + TileSizeOffset);
					DrawWireBox(PDI, BoxInner, ProcTileColor, SDPG_World);

					const FBox BoxOuter = BoxInner + (BoxInner.Min - OverlapV) + (BoxInner.Max + OverlapV);
					DrawWireBox(PDI, BoxOuter, ProcTileOverlapColor, SDPG_World);
				}
			}
			
		}
	}
}
