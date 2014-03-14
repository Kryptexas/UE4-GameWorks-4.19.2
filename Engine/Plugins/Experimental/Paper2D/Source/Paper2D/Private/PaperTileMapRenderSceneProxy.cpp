// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperTileMapRenderSceneProxy.h"
#include "PhysicsEngine/BodySetup.h"

//////////////////////////////////////////////////////////////////////////
// FPaperTileMapRenderSceneProxy

FPaperTileMapRenderSceneProxy::FPaperTileMapRenderSceneProxy(const UPaperTileMapRenderComponent* InComponent)
	: FPaperRenderSceneProxy(InComponent)
	, TileComponent(NULL)
{
	if (const UPaperTileMapRenderComponent* InTileComponent = Cast<const UPaperTileMapRenderComponent>(InComponent))
	{
		TileComponent = InTileComponent;
		Material = TileComponent->Material;
	}
}

void FPaperTileMapRenderSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	BatchedSprites.Empty();

	// Draw the tile maps
	//@TODO: RenderThread race condition
	if (TileComponent != NULL)
	{
		FColor WireframeColor = FColor(0, 255, 255, 255);
		bool bIsCollisionEnabled = TileComponent->IsCollisionEnabled();

		if((View->Family->EngineShowFlags.Collision && bIsCollisionEnabled) && AllowDebugViewmodes())
		{
			if(TileComponent->ShapeBodySetup)
			{
				if(FMath::Abs(GetLocalToWorld().Determinant()) < SMALL_NUMBER)
				{
					// Catch this here or otherwise GeomTransform below will assert
					// This spams so commented out
					//UE_LOG(LogPaper2D, Log, TEXT("Zero scaling not supported (%s)"), *StaticMesh->GetPathName());
				}
				else
				{
					// Make a material for drawing solid collision stuff
					const UMaterial* LevelColorationMaterial = View->Family->EngineShowFlags.Lighting 
						? GEngine->ShadedLevelColorationLitMaterial : GEngine->ShadedLevelColorationUnlitMaterial;

					const FColoredMaterialRenderProxy CollisionMaterialInstance(
						LevelColorationMaterial->GetRenderProxy(IsSelected(), IsHovered()),
						WireframeColor
						);

					// Draw the static mesh's body setup.

					// Get transform without scaling.
					FTransform GeomTransform(GetLocalToWorld());

					// In old wireframe collision mode, always draw the wireframe highlighted (selected or not).
					bool bDrawWireSelected = IsSelected();
					if(View->Family->EngineShowFlags.Collision)
					{
						bDrawWireSelected = true;
					}

					// Differentiate the color based on bBlockNonZeroExtent.  Helps greatly with skimming a level for optimization opportunities.
					FColor collisionColor = FColor(157,149,223,255);

					const bool bPerHullColor = false;
					const bool bDrawSimpleSolid = false;
					TileComponent->ShapeBodySetup->AggGeom.DrawAggGeom(PDI, GeomTransform, GetSelectionColor(collisionColor, bDrawWireSelected, IsHovered()), &CollisionMaterialInstance, bPerHullColor, bDrawSimpleSolid);
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

			const float TW = TileComponent->TileWidth;
			const float TH = TileComponent->TileHeight;
			//@TODO: PAPER: Tiles probably shouldn't be drawn with their pivot at the center - RE: all the 0.5f in here

			if (bUseOverrideColor)
			{
				// Draw horizontal lines
				for (int32 Y = 0; Y <= TileComponent->MapHeight; ++Y)
				{
					int32 X = 0;
					const FVector Start((X - 0.5f) * TW, 0.0f, -(Y - 0.5f) * TH);

					X = TileComponent->MapWidth;
					const FVector End((X - 0.5f) * TW, 0.0f, -(Y - 0.5f) * TH);

					PDI->DrawLine(LocalToWorld.TransformPosition(Start), LocalToWorld.TransformPosition(End), OverrideColor, DPG);
				}

				// Draw vertical lines
				for (int32 X = 0; X <= TileComponent->MapWidth; ++X)
				{
					int32 Y = 0;
					const FVector Start((X - 0.5f) * TW, 0.0f, -(Y - 0.5f) * TH);

					Y = TileComponent->MapHeight;
					const FVector End((X - 0.5f) * TW, 0.0f, -(Y - 0.5f) * TH);

					PDI->DrawLine(LocalToWorld.TransformPosition(Start), LocalToWorld.TransformPosition(End), OverrideColor, DPG);
				}
			}


			// Create a local space bounding box
			const FVector TopLeft(-TW*0.5f, -2.0f, TH*0.5f);
			const FVector Dimensions(TileComponent->MapWidth * TW, 4.0f, -TileComponent->MapHeight * TH);
			const FBox OutlineBox(TopLeft, TopLeft+Dimensions);
		
			// Draw it		
			DrawWireBox(PDI, OutlineBox.TransformBy(LocalToWorld), FLinearColor::White, DPG); //@TODO: Paper: Doesn't handle rotation well - need to use DrawOrientedWireBox
		}
#endif

		const float LayerSeparation = 64.0f;

		const int32 TileSeparationX = TileComponent->TileWidth;
		const int32 TileSeparationY = TileComponent->TileHeight;

		for (int32 LayerIndex = 0; LayerIndex < TileComponent->TileLayers.Num(); ++LayerIndex)
		{
			UPaperTileLayer* Layer = TileComponent->TileLayers[LayerIndex];
			if (Layer == NULL)
			{
				continue;
			}

			if (Layer->bHiddenInEditor)
			{
				continue;
			}

			if (Layer->bCollisionLayer && View->bIsGameView)
			{
				continue;
			}

			ensure(Layer->LayerWidth == TileComponent->MapWidth);
			ensure(Layer->LayerHeight == TileComponent->MapHeight);

			UPaperTileSet* TileSet = Layer->TileSet;
				
			int32 TileWidth = 4;
			int32 TileHeight = 4;
			UTexture2D* SourceTexture = NULL;
			if ( Layer->bCollisionLayer )
			{
				SourceTexture = UCanvas::StaticClass()->GetDefaultObject<UCanvas>()->DefaultTexture;
			}
			else
			{
				TileWidth = TileSet->TileWidth;
				TileHeight = TileSet->TileHeight;
				SourceTexture = TileSet->TileSheet;
			}
			FVector2D SourceDimensionsUV(TileWidth, TileHeight);

			UPaperTileMapRenderComponent* TileMap = Layer->GetTileMap();
			FVector2D TileSizeXY(TileMap->TileWidth, TileMap->TileHeight);
			FLinearColor DrawColor = FLinearColor::White;
			EBlendMode BlendMode = BLEND_Translucent;
			if (Layer->bCollisionLayer)
			{
				DrawColor = FLinearColor::Blue;
				BlendMode = BLEND_Opaque;
			}

			for (int32 Y = 0; Y < Layer->LayerHeight; ++Y)
			{
				for (int32 X = 0; X < Layer->LayerWidth; ++X)
				{
					const int32 TileIndex = Layer->GetCell(X, Y);

					FVector2D SourceUV = FVector2D::ZeroVector;
					if (Layer->bCollisionLayer)
					{
						if ( TileIndex == 0 )
						{
							continue;
						}
					}
					else
					{
						if (!TileSet->GetTileUV(TileIndex, /*out*/ SourceUV))
						{
							continue;
						}
					}

					const FVector DrawPos(TileSeparationX * X, LayerIndex * LayerSeparation, -TileSeparationY * Y);

					FSpriteDrawCallRecord& NewTile = *(new (BatchedSprites) FSpriteDrawCallRecord());
					//NewTile.SourceUV = SourceUV;
					//NewTile.SourceDimension = SourceDimensionsUV;
					NewTile.Destination = DrawPos;
					//NewTile.DestinationDimension = TileSizeXY;
					NewTile.Texture = SourceTexture;
					NewTile.Color = DrawColor;
					NewTile.BlendMode = BlendMode;
					
					//@TODO: PAPER: TILE EDITOR: Build data
				}
			}
		}
	}

	// Draw all of the queued up sprites
	FPaperRenderSceneProxy::DrawDynamicElements(PDI, View);
}
