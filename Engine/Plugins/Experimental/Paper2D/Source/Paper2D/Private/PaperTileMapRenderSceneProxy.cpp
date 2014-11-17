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
			MaterialRelevance = Material->GetRelevance();
		}
	}
}

void FPaperTileMapRenderSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	BatchedSprites.Empty();

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
					FColor CollisionColor = FColor(157,149,223,255);

					const bool bPerHullColor = false;
					const bool bDrawSimpleSolid = false;
					BodySetup->AggGeom.DrawAggGeom(PDI, GeomTransform, GetSelectionColor(CollisionColor, bDrawWireSelected, IsHovered()), &CollisionMaterialInstance, bPerHullColor, bDrawSimpleSolid);
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

			const float TW = TileMap ->TileWidth;
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
			const FBox OutlineBox(TopLeft, TopLeft+Dimensions);
		
			// Draw it		
			DrawWireBox(PDI, OutlineBox.TransformBy(LocalToWorld), FLinearColor::White, DPG); //@TODO: Paper: Doesn't handle rotation well - need to use DrawOrientedWireBox
		}
#endif

		const float LayerSeparation = 64.0f;

		const int32 TileSeparationX = TileMap->TileWidth;
		const int32 TileSeparationY = TileMap->TileHeight;

		for (int32 LayerIndex = 0; LayerIndex < TileMap->TileLayers.Num(); ++LayerIndex)
		{
			UPaperTileLayer* Layer = TileMap->TileLayers[LayerIndex];
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

			ensure(Layer->LayerWidth == TileMap->MapWidth);
			ensure(Layer->LayerHeight == TileMap->MapHeight);

			UPaperTileSet* TileSet = Layer->TileSet;
				
			int32 TileWidth = 4;
			int32 TileHeight = 4;

			UTexture2D* LastSourceTexture = nullptr;
			FVector2D InverseTextureSize(1.0f, 1.0f);
			FVector2D SourceDimensionsUV(1.0f, 1.0f);

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
					const FPaperTileInfo TileInfo = Layer->GetCell(X, Y);
					UTexture2D* SourceTexture = nullptr;

					FVector2D SourceUV = FVector2D::ZeroVector;
					if (Layer->bCollisionLayer)
					{
						if (TileInfo.PackedTileIndex == 0)
						{
							continue;
						}
						SourceTexture = UCanvas::StaticClass()->GetDefaultObject<UCanvas>()->DefaultTexture;
					}
					else
					{
						if (TileInfo.TileSet == nullptr)
						{
							continue;
						}

						if (!TileInfo.TileSet->GetTileUV(TileInfo.PackedTileIndex, /*out*/ SourceUV))
						{
							continue;
						}

						SourceTexture = TileInfo.TileSet->TileSheet;
						if (SourceTexture == nullptr)
						{
							continue;
						}
					}


					if (SourceTexture != LastSourceTexture)
					{
						TileWidth = TileSet->TileWidth;
						TileHeight = TileSet->TileHeight;
						InverseTextureSize = FVector2D(1.0f / SourceTexture->GetSizeX(), 1.0f / SourceTexture->GetSizeY());
						SourceDimensionsUV = FVector2D(TileWidth * InverseTextureSize.X, TileHeight * InverseTextureSize.Y);
						LastSourceTexture = SourceTexture;
					}


					SourceUV.X *= InverseTextureSize.X;
					SourceUV.Y *= InverseTextureSize.Y;

					const FVector DrawPos(TileSeparationX * (X - 0.5f) * PaperAxisX + TileSeparationY * -(Y + 0.5f) * PaperAxisY + ((LayerIndex * LayerSeparation) * PaperAxisZ));

					FSpriteDrawCallRecord& NewTile = *(new (BatchedSprites) FSpriteDrawCallRecord());
					NewTile.Destination = DrawPos;
					NewTile.Texture = SourceTexture;
					NewTile.Color = DrawColor;
					
					const float W = TileSeparationX;
					const float H = TileSeparationY;

					const FVector4 BottomLeft(0.0f, 0.0f, SourceUV.X, SourceUV.Y + SourceDimensionsUV.Y);
					const FVector4 BottomRight(W, 0.0f, SourceUV.X + SourceDimensionsUV.X, SourceUV.Y + SourceDimensionsUV.Y);
					const FVector4 TopRight(W, H, SourceUV.X + SourceDimensionsUV.X, SourceUV.Y);
					const FVector4 TopLeft(0.0f, H, SourceUV.X, SourceUV.Y);

					new (NewTile.RenderVerts) FVector4(BottomLeft);
					new (NewTile.RenderVerts) FVector4(TopRight);
					new (NewTile.RenderVerts) FVector4(BottomRight);

					new (NewTile.RenderVerts) FVector4(BottomLeft);
					new (NewTile.RenderVerts) FVector4(TopLeft);
					new (NewTile.RenderVerts) FVector4(TopRight);
				}
			}
		}
	}

	// Draw all of the queued up sprites
	FPaperRenderSceneProxy::DrawDynamicElements(PDI, View);
}
