// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SpriteEditorViewportClient.h"
#include "SceneViewport.h"

#include "PreviewScene.h"
#include "ScopedTransaction.h"
#include "Runtime/Engine/Public/ComponentReregisterContext.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "CanvasTypes.h"
#include "CanvasItem.h"
#include "PaperEditorShared/SocketEditing.h"

#include "PhysicsEngine/BodySetup2D.h"

#define LOCTEXT_NAMESPACE "SpriteEditor"

//////////////////////////////////////////////////////////////////////////
// FSelectionTypes

const FName FSelectionTypes::GeometryShape(TEXT("GeometryShape"));
const FName FSelectionTypes::Vertex(TEXT("Vertex"));
const FName FSelectionTypes::Edge(TEXT("Edge"));
const FName FSelectionTypes::Pivot(TEXT("Pivot"));
const FName FSelectionTypes::SourceRegion(TEXT("SourceRegion"));


//////////////////////////////////////////////////////////////////////////
// Sprite editing constants

namespace SpriteEditingConstants
{
	// Tint the source texture darker to help distinguish it from the sprite being edited
	const FLinearColor SourceTextureDarkTintColor(0.05f, 0.05f, 0.05f, 1.0f);

	// Note: MinMouseRadius must be greater than MinArrowLength
	const FLinearColor BakedCollisionLineRenderColor(1.0f, 1.0f, 0.0f, 0.25f);
	const FLinearColor BakedCollisionRenderColor(1.0f, 1.0f, 0.0f, 0.5f);
	const float BakedCollisionVertexSize = 3.0f;

	// The length and color to draw a normal tick in
	const FLinearColor GeometryNormalColor(0.0f, 1.0f, 0.0f, 0.5f);
	const float GeometryNormalLength = 15.0f;

	const FLinearColor MarqueeColor(1.0f, 1.0f, 1.0f, 0.5f);

	const FLinearColor SourceRegionBoundsColor(1.0f, 1.0f, 1.0f, 0.8f);
	// Alpha is being ignored in FBatchedElements::AddLine :(
	const FLinearColor SourceRegionRelatedBoundsColor(0.3f, 0.3f, 0.3f, 0.2f);

	const FLinearColor CollisionShapeColor(0.0f, 0.7f, 1.0f, 1.0f);
	const FLinearColor RenderShapeColor(1.0f, 0.2f, 0.0f, 1.0f);
	const FLinearColor SubtractiveRenderShapeColor(0.0f, 0.2f, 1.0f, 1.0f);

	// Geometry edit rendering stuff
	const float GeometryVertexSize = 8.0f;
	const float GeometryBorderLineThickness = 2.0f;
	const int32 CircleShapeNumSides = 64;
	const FLinearColor GeometrySelectedColor(FLinearColor::White);
}

//////////////////////////////////////////////////////////////////////////
// FSpriteEditorViewportClient

FSpriteEditorViewportClient::FSpriteEditorViewportClient(TWeakPtr<FSpriteEditor> InSpriteEditor, TWeakPtr<class SSpriteEditorViewport> InSpriteEditorViewportPtr)
	: CurrentMode(ESpriteEditorMode::ViewMode)
	, SpriteEditorPtr(InSpriteEditor)
	, SpriteEditorViewportPtr(InSpriteEditorViewportPtr)
	, SpriteGeometryHelper(*this)
{
	check(SpriteEditorPtr.IsValid() && SpriteEditorViewportPtr.IsValid());

	PreviewScene = &OwnedPreviewScene;

	SetRealtime(true);

	DesiredWidgetMode = FWidget::WM_Translate;
	bManipulating = false;
	bManipulationDirtiedSomething = false;
	ScopedTransaction = nullptr;

	bShowSourceTexture = false;
	bShowSockets = true;
	bShowNormals = true;
	bShowPivot = true;
	bShowRelatedSprites = true;

	bDeferZoomToSprite = true;

	bIsMarqueeTracking = false;

	DrawHelper.bDrawGrid = false;

	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.CompositeEditorPrimitives = true;

	// Create a render component for the sprite being edited
	{
		RenderSpriteComponent = NewObject<UPaperSpriteComponent>();
		UPaperSprite* Sprite = GetSpriteBeingEdited();
		RenderSpriteComponent->SetSprite(Sprite);

		PreviewScene->AddComponent(RenderSpriteComponent, FTransform::Identity);
	}

	// Create a sprite and render component for the source texture view
	{
		UPaperSprite* DummySprite = NewObject<UPaperSprite>();
		DummySprite->SpriteCollisionDomain = ESpriteCollisionMode::None;
		DummySprite->PivotMode = ESpritePivotMode::Bottom_Left;
		DummySprite->CollisionGeometry.GeometryType = ESpritePolygonMode::SourceBoundingBox;
		DummySprite->RenderGeometry.GeometryType = ESpritePolygonMode::SourceBoundingBox;

		SourceTextureViewComponent = NewObject<UPaperSpriteComponent>();
		SourceTextureViewComponent->SetSprite(DummySprite);
		UpdateSourceTextureSpriteFromSprite(GetSpriteBeingEdited());

		// Nudge the source texture view back a bit so it doesn't occlude sprites
		const FTransform Transform(-1.0f * PaperAxisZ);
		SourceTextureViewComponent->bVisible = false;
		PreviewScene->AddComponent(SourceTextureViewComponent, Transform);
	}
}

void FSpriteEditorViewportClient::UpdateSourceTextureSpriteFromSprite(UPaperSprite* SourceSprite)
{
	UPaperSprite* TargetSprite = SourceTextureViewComponent->GetSprite();
	check(TargetSprite);

	if (SourceSprite != nullptr)
	{
		if ((SourceSprite->GetSourceTexture() != TargetSprite->GetSourceTexture()) || (TargetSprite->PixelsPerUnrealUnit != SourceSprite->PixelsPerUnrealUnit))
		{
			FComponentReregisterContext ReregisterSprite(SourceTextureViewComponent);

			FSpriteAssetInitParameters SpriteReinitParams;
			SpriteReinitParams.SetTextureAndFill(SourceSprite->SourceTexture);
			TargetSprite->PixelsPerUnrealUnit = SourceSprite->PixelsPerUnrealUnit;
			TargetSprite->InitializeSprite(SpriteReinitParams);

			bDeferZoomToSprite = true;
		}


		// Position the sprite for the mode its meant to be in
		FVector2D CurrentPivotPosition;
		ESpritePivotMode::Type CurrentPivotMode = TargetSprite->GetPivotMode(/*out*/CurrentPivotPosition);

		FVector Translation(1.0f * PaperAxisZ);
		if (IsInSourceRegionEditMode())
		{
			if (CurrentPivotMode != ESpritePivotMode::Bottom_Left)
			{
				TargetSprite->SetPivotMode(ESpritePivotMode::Bottom_Left, FVector2D::ZeroVector);
				TargetSprite->PostEditChange();
			}
			SourceTextureViewComponent->SetSpriteColor(FLinearColor::White);
			SourceTextureViewComponent->SetWorldTransform(FTransform(Translation));
		}
		else
		{
			const FVector2D PivotPosition = SourceSprite->GetPivotPosition();
			if (CurrentPivotMode != ESpritePivotMode::Custom || CurrentPivotPosition != PivotPosition)
			{
				TargetSprite->SetPivotMode(ESpritePivotMode::Custom, PivotPosition);
				TargetSprite->PostEditChange();
			}

			// Tint the source texture darker to help distinguish the two
			SourceTextureViewComponent->SetSpriteColor(SpriteEditingConstants::SourceTextureDarkTintColor);

			const bool bRotated = SourceSprite->IsRotatedInSourceImage();
			if (bRotated)
			{
				FQuat Rotation(PaperAxisZ, FMath::DegreesToRadians(90.0f));
				SourceTextureViewComponent->SetWorldTransform(FTransform(Rotation, Translation));
			}
			else
			{
				SourceTextureViewComponent->SetWorldTransform(FTransform(Translation));
			}
		}
	}
	else
	{
		// No source sprite, so don't draw the target either
		TargetSprite->SourceTexture = nullptr;
	}
}

FVector2D FSpriteEditorViewportClient::TextureSpaceToScreenSpace(const FSceneView& View, const FVector2D& SourcePoint) const
{
	const FVector WorldSpacePoint = TextureSpaceToWorldSpace(SourcePoint);

	FVector2D PixelLocation;
	View.WorldToPixel(WorldSpacePoint, /*out*/ PixelLocation);
	return PixelLocation;
}

FVector2D FSpriteEditorViewportClient::SourceTextureSpaceToScreenSpace(const FSceneView& View, const FVector2D& SourcePoint) const
{
	const FVector WorldSpacePoint = SourceTextureSpaceToWorldSpace(SourcePoint);

	FVector2D PixelLocation;
	View.WorldToPixel(WorldSpacePoint, /*out*/ PixelLocation);
	return PixelLocation;
}

FVector FSpriteEditorViewportClient::SourceTextureSpaceToWorldSpace(const FVector2D& SourcePoint) const
{
	UPaperSprite* Sprite = SourceTextureViewComponent->GetSprite();
	return Sprite->ConvertTextureSpaceToWorldSpace(SourcePoint);
}

void FSpriteEditorViewportClient::DrawTriangleList(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const TArray<FVector2D>& Triangles)
{
	// Draw the collision data (non-interactive)

	// Draw the vertices
	for (int32 VertexIndex = 0; VertexIndex < Triangles.Num(); ++VertexIndex)
	{
		const FVector2D VertexPos(TextureSpaceToScreenSpace(View, Triangles[VertexIndex]));
		Canvas.DrawTile(VertexPos.X - SpriteEditingConstants::BakedCollisionVertexSize*0.5f, VertexPos.Y - SpriteEditingConstants::BakedCollisionVertexSize*0.5f, SpriteEditingConstants::BakedCollisionVertexSize, SpriteEditingConstants::BakedCollisionVertexSize, 0.f, 0.f, 1.f, 1.f, SpriteEditingConstants::BakedCollisionRenderColor, GWhiteTexture);
	}

	// Draw the lines
	for (int32 TriangleIndex = 0; TriangleIndex < Triangles.Num() / 3; ++TriangleIndex)
	{
		for (int32 Offset = 0; Offset < 3; ++Offset)
		{
			const int32 VertexIndex = (TriangleIndex * 3) + Offset;
			const int32 NextVertexIndex = (TriangleIndex * 3) + ((Offset + 1) % 3);

			const FVector2D VertexPos(TextureSpaceToScreenSpace(View, Triangles[VertexIndex]));
			const FVector2D NextVertexPos(TextureSpaceToScreenSpace(View, Triangles[NextVertexIndex]));

			FCanvasLineItem LineItem(VertexPos, NextVertexPos);
			LineItem.SetColor(SpriteEditingConstants::BakedCollisionLineRenderColor);
			Canvas.DrawItem(LineItem);
		}
	}
}

void FSpriteEditorViewportClient::DrawRelatedSprites(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FLinearColor& LineColor)
{
	for (int32 SpriteIndex = 0; SpriteIndex < RelatedSprites.Num(); ++SpriteIndex)
	{
		FRelatedSprite& RelatedSprite = RelatedSprites[SpriteIndex];
		const FVector2D& SourceUV = RelatedSprite.SourceUV;
		const FVector2D& SourceDimension = RelatedSprite.SourceDimension;
		FVector2D BoundsVertices[4];
		BoundsVertices[0] = SourceTextureSpaceToScreenSpace(View, SourceUV);
		BoundsVertices[1] = SourceTextureSpaceToScreenSpace(View, SourceUV + FVector2D(SourceDimension.X, 0));
		BoundsVertices[2] = SourceTextureSpaceToScreenSpace(View, SourceUV + FVector2D(SourceDimension.X, SourceDimension.Y));
		BoundsVertices[3] = SourceTextureSpaceToScreenSpace(View, SourceUV + FVector2D(0, SourceDimension.Y));
		for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
		{
			const int32 NextVertexIndex = (VertexIndex + 1) % 4;

			FCanvasLineItem LineItem(BoundsVertices[VertexIndex], BoundsVertices[NextVertexIndex]);
			LineItem.SetColor(LineColor);
			Canvas.DrawItem(LineItem);
		}
	}
}

void FSpriteEditorViewportClient::DrawSourceRegion(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FLinearColor& GeometryVertexColor)
{
	const bool bIsHitTesting = Canvas.IsHitTesting();
	UPaperSprite* Sprite = GetSpriteBeingEdited();

    const float CornerCollisionVertexSize = 8.0f;
	const float EdgeCollisionVertexSize = 6.0f;

	const FLinearColor GeometryLineColor(GeometryVertexColor.R, GeometryVertexColor.G, GeometryVertexColor.B, 0.5f * GeometryVertexColor.A);
    
    const bool bDrawEdgeHitProxies = true;
    const bool bDrawCornerHitProxies = true;

	FVector2D BoundsVertices[4];
	BoundsVertices[0] = SourceTextureSpaceToScreenSpace(View, Sprite->SourceUV);
	BoundsVertices[1] = SourceTextureSpaceToScreenSpace(View, Sprite->SourceUV + FVector2D(Sprite->SourceDimension.X, 0));
	BoundsVertices[2] = SourceTextureSpaceToScreenSpace(View, Sprite->SourceUV + FVector2D(Sprite->SourceDimension.X, Sprite->SourceDimension.Y));
	BoundsVertices[3] = SourceTextureSpaceToScreenSpace(View, Sprite->SourceUV + FVector2D(0, Sprite->SourceDimension.Y));
	for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
	{
		const int32 NextVertexIndex = (VertexIndex + 1) % 4;

		FCanvasLineItem LineItem(BoundsVertices[VertexIndex], BoundsVertices[NextVertexIndex]);
		LineItem.SetColor(GeometryVertexColor);
		Canvas.DrawItem(LineItem);

		const FVector2D MidPoint = (BoundsVertices[VertexIndex] + BoundsVertices[NextVertexIndex]) * 0.5f;
        const FVector2D CornerPoint = BoundsVertices[VertexIndex];

        // Add edge hit proxy
        if (bDrawEdgeHitProxies)
        {
            if (bIsHitTesting)
            {
                TSharedPtr<FSpriteSelectedSourceRegion> Data = MakeShareable(new FSpriteSelectedSourceRegion());
                Data->SpritePtr = Sprite;
                Data->VertexIndex = 4 + VertexIndex;
                Canvas.SetHitProxy(new HSpriteSelectableObjectHitProxy(Data));
            }

            Canvas.DrawTile(MidPoint.X - EdgeCollisionVertexSize*0.5f, MidPoint.Y - EdgeCollisionVertexSize*0.5f, EdgeCollisionVertexSize, EdgeCollisionVertexSize, 0.f, 0.f, 1.f, 1.f, GeometryVertexColor, GWhiteTexture);

            if (bIsHitTesting)
            {
                Canvas.SetHitProxy(nullptr);
            }
        }

        // Add corner hit proxy
        if (bDrawCornerHitProxies)
        {
            if (bIsHitTesting)
            {
                TSharedPtr<FSpriteSelectedSourceRegion> Data = MakeShareable(new FSpriteSelectedSourceRegion());
                Data->SpritePtr = Sprite;
                Data->VertexIndex = VertexIndex;
                Canvas.SetHitProxy(new HSpriteSelectableObjectHitProxy(Data));
            }
            
            Canvas.DrawTile(CornerPoint.X - CornerCollisionVertexSize * 0.5f, CornerPoint.Y - CornerCollisionVertexSize * 0.5f, CornerCollisionVertexSize, CornerCollisionVertexSize, 0.f, 0.f, 1.f, 1.f, GeometryVertexColor, GWhiteTexture);

            if (bIsHitTesting)
            {
                Canvas.SetHitProxy(nullptr);
            }
        }
	}
}

void FSpriteEditorViewportClient::DrawMarquee(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FLinearColor& MarqueeColor)
{
	FVector2D MarqueeDelta = MarqueeEndPos - MarqueeStartPos;
	FVector2D MarqueeVertices[4];
	MarqueeVertices[0] = FVector2D(MarqueeStartPos.X, MarqueeStartPos.Y);
	MarqueeVertices[1] = MarqueeVertices[0] + FVector2D(MarqueeDelta.X, 0);
	MarqueeVertices[2] = MarqueeVertices[0] + FVector2D(MarqueeDelta.X, MarqueeDelta.Y);
	MarqueeVertices[3] = MarqueeVertices[0] + FVector2D(0, MarqueeDelta.Y);
	for (int32 MarqueeVertexIndex = 0; MarqueeVertexIndex < 4; ++MarqueeVertexIndex)
	{
		const int32 NextVertexIndex = (MarqueeVertexIndex + 1) % 4;
		FCanvasLineItem MarqueeLine(MarqueeVertices[MarqueeVertexIndex], MarqueeVertices[NextVertexIndex]);
		MarqueeLine.SetColor(MarqueeColor);
		Canvas.DrawItem(MarqueeLine);
	}
}

void FSpriteEditorViewportClient::DrawGeometry_CanvasPass(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, FSpriteGeometryCollection& Geometry, const FLinearColor& GeometryVertexColor, const FLinearColor& NegativeGeometryVertexColor, bool bIsRenderGeometry)
{
	const bool bIsHitTesting = Canvas.IsHitTesting();
	UPaperSprite* Sprite = GetSpriteBeingEdited();

	// Run thru the geometry shapes and draw hit proxies for them
	for (int32 ShapeIndex = 0; ShapeIndex < Geometry.Shapes.Num(); ++ShapeIndex)
	{
		const FSpriteGeometryShape& Shape = Geometry.Shapes[ShapeIndex];

		const bool bIsShapeSelected = IsShapeSelected(ShapeIndex);
		const FLinearColor LineColorRaw = Shape.bNegativeWinding ? NegativeGeometryVertexColor : GeometryVertexColor;
		const FLinearColor VertexColor = Shape.bNegativeWinding ? NegativeGeometryVertexColor : GeometryVertexColor;

		const FLinearColor LineColor = Shape.IsShapeValid() ? LineColorRaw : FMath::Lerp(LineColorRaw, FLinearColor::Red, 0.8f);

		// This is a work-in-progress, it currently blocks widget selection even with a lower hit proxy priority, so it is disabled
#define UE_DRAW_SPRITE_SHAPE_BACKGROUNDS 0
#if UE_DRAW_SPRITE_SHAPE_BACKGROUNDS
		// Draw the interior (allowing selection of the whole shape)
		if (bIsHitTesting)
		{
			TSharedPtr<FSpriteSelectedShape> Data = MakeShareable(new FSpriteSelectedShape(*this, Geometry, ShapeIndex, /*bIsBackground=*/ true));
			Canvas.SetHitProxy(new HSpriteSelectableObjectHitProxy(Data));
		}

		FLinearColor BackgroundColor = bIsShapeSelected ? SpriteEditingConstants::GeometrySelectedColor : LineColor;
		BackgroundColor.A *= 0.5f;

		if (Shape.ShapeType == ESpriteShapeType::Circle)
		{
			const FVector2D PixelSpaceRadius = Shape.BoxSize * 0.5f;

			//@TODO: This is the wrong size when in Perspective mode
			const FVector2D ScreenSpaceRadius = TextureSpaceToScreenSpace(View, PixelSpaceRadius) - TextureSpaceToScreenSpace(View, FVector2D::ZeroVector);

			const FVector2D ScreenPos = TextureSpaceToScreenSpace(View, Shape.BoxPosition);

			FCanvasNGonItem CircleBackground(ScreenPos, ScreenSpaceRadius, SpriteEditingConstants::CircleShapeNumSides, BackgroundColor);
			//@TODO: CircleBackground.BlendMode = SE_BLEND_Translucent;
			CircleBackground.Draw(&Canvas);
		}
		else if (Shape.ShapeType == ESpriteShapeType::Box)
		{
			const FVector2D HalfSize = Shape.BoxSize * 0.5f;
			const FVector2D TopLeft = TextureSpaceToScreenSpace(View, Shape.BoxPosition - HalfSize);
			const FVector2D BottomRight = TextureSpaceToScreenSpace(View, Shape.BoxPosition + HalfSize);
			const FVector2D TopRight = TextureSpaceToScreenSpace(View, FVector2D(Shape.BoxPosition.X + HalfSize.X, Shape.BoxPosition.Y - HalfSize.Y));
			const FVector2D BottomLeft = TextureSpaceToScreenSpace(View, FVector2D(Shape.BoxPosition.X - HalfSize.X, Shape.BoxPosition.Y + HalfSize.Y));

			FCanvasTriangleItem Triangle(TopLeft, TopRight, BottomRight, GWhiteTexture);
			//@TODO: Triangle.BlendMode = SE_BLEND_Translucent;
			Triangle.SetColor(BackgroundColor);
			Triangle.Draw(&Canvas);

			Triangle.SetPoints(TopLeft, BottomRight, BottomLeft);
			Triangle.Draw(&Canvas);
		}
		else
		{
			//@TODO: Draw the background for polygons
		}

		if (bIsHitTesting)
		{
			Canvas.SetHitProxy(nullptr);
		}
#endif

		// Draw the circle shape if necessary
		if (Shape.ShapeType == ESpriteShapeType::Circle)
		{
			if (bIsHitTesting)
			{
				TSharedPtr<FSpriteSelectedShape> Data = MakeShareable(new FSpriteSelectedShape(*this, Geometry, ShapeIndex, /*bIsBackground=*/ false));
				Canvas.SetHitProxy(new HSpriteSelectableObjectHitProxy(Data));
			}

			//@TODO: Draw the circle
			const float RadiusX = Shape.BoxSize.X * 0.5f;
			const float RadiusY = Shape.BoxSize.Y * 0.5f;

			const float	AngleDelta = 2.0f * PI / SpriteEditingConstants::CircleShapeNumSides;
			
			const float LastX = Shape.BoxPosition.X + RadiusX;
			const float LastY = Shape.BoxPosition.Y;
			FVector2D LastVertexPos = TextureSpaceToScreenSpace(View, FVector2D(LastX, LastY));


			for (int32 SideIndex = 0; SideIndex < SpriteEditingConstants::CircleShapeNumSides; SideIndex++)
			{
				const float X = Shape.BoxPosition.X + RadiusX * FMath::Cos(AngleDelta * (SideIndex + 1));
				const float Y = Shape.BoxPosition.Y + RadiusY * FMath::Sin(AngleDelta * (SideIndex + 1));
				const FVector2D ScreenPos = TextureSpaceToScreenSpace(View, FVector2D(X, Y));

				FCanvasLineItem LineItem(LastVertexPos, ScreenPos);
				LineItem.SetColor(bIsShapeSelected ? SpriteEditingConstants::GeometrySelectedColor : LineColor);
				LineItem.LineThickness = SpriteEditingConstants::GeometryBorderLineThickness;

				Canvas.DrawItem(LineItem);

				LastVertexPos = ScreenPos;
			}


			if (bIsHitTesting)
			{
				Canvas.SetHitProxy(nullptr);
			}
		}

		// Draw lines connecting the vertices of the shape
		for (int32 VertexIndex = 0; VertexIndex < Shape.Vertices.Num(); ++VertexIndex)
		{
			const int32 NextVertexIndex = (VertexIndex + 1) % Shape.Vertices.Num();

			const FVector2D ScreenPos = TextureSpaceToScreenSpace(View, Shape.ConvertShapeSpaceToTextureSpace(Shape.Vertices[VertexIndex]));
			const FVector2D NextScreenPos = TextureSpaceToScreenSpace(View, Shape.ConvertShapeSpaceToTextureSpace(Shape.Vertices[NextVertexIndex]));

			const bool bIsEdgeSelected = bIsShapeSelected || (IsPolygonVertexSelected(ShapeIndex, VertexIndex) && IsPolygonVertexSelected(ShapeIndex, NextVertexIndex));

			// Draw the normal tick
			if (bShowNormals)
			{
				const FVector2D Direction = (NextScreenPos - ScreenPos).GetSafeNormal();
				const FVector2D Normal = FVector2D(-Direction.Y, Direction.X);

				const FVector2D Midpoint = (ScreenPos + NextScreenPos) * 0.5f;
				const FVector2D NormalPoint = Midpoint - Normal * SpriteEditingConstants::GeometryNormalLength;
				FCanvasLineItem LineItem(Midpoint, NormalPoint);
				LineItem.SetColor(SpriteEditingConstants::GeometryNormalColor);

				Canvas.DrawItem(LineItem);
			}

			// Draw the edge
			{
				if (bIsHitTesting)
				{
					TSharedPtr<FSpriteSelectedEdge> Data = MakeShareable(new FSpriteSelectedEdge());
					Data->SpritePtr = Sprite;
					Data->ShapeIndex = ShapeIndex;
					Data->VertexIndex = VertexIndex;
					Data->bRenderData = bIsRenderGeometry;
					Canvas.SetHitProxy(new HSpriteSelectableObjectHitProxy(Data));
				}

				FCanvasLineItem LineItem(ScreenPos, NextScreenPos);
				LineItem.SetColor(bIsEdgeSelected ? SpriteEditingConstants::GeometrySelectedColor : LineColor);
				LineItem.LineThickness = SpriteEditingConstants::GeometryBorderLineThickness;
				Canvas.DrawItem(LineItem);

				if (bIsHitTesting)
				{
					Canvas.SetHitProxy(nullptr);
				}
			}
		}

		// Draw the vertices
		for (int32 VertexIndex = 0; VertexIndex < Shape.Vertices.Num(); ++VertexIndex)
		{
			const FVector2D ScreenPos = TextureSpaceToScreenSpace(View, Shape.ConvertShapeSpaceToTextureSpace(Shape.Vertices[VertexIndex]));
			const float X = ScreenPos.X;
			const float Y = ScreenPos.Y;

			const bool bIsVertexSelected = IsPolygonVertexSelected(ShapeIndex, VertexIndex);
			const bool bIsVertexLastAdded = IsAddingPolygon() && (AddingPolygonIndex == ShapeIndex) && (VertexIndex == Shape.Vertices.Num() - 1);
			const bool bNeedHighlightVertex = bIsShapeSelected || bIsVertexSelected || bIsVertexLastAdded;

			if (bIsHitTesting)
			{
				TSharedPtr<FSpriteSelectedVertex> Data = MakeShareable(new FSpriteSelectedVertex());
				Data->SpritePtr = Sprite;
				Data->ShapeIndex = ShapeIndex;
				Data->VertexIndex = VertexIndex;
				Data->bRenderData = bIsRenderGeometry;
				Canvas.SetHitProxy(new HSpriteSelectableObjectHitProxy(Data));
			}

			const float VertSize = SpriteEditingConstants::GeometryVertexSize;
			Canvas.DrawTile(ScreenPos.X - VertSize*0.5f, ScreenPos.Y - VertSize*0.5f, VertSize, VertSize, 0.f, 0.f, 1.f, 1.f, bNeedHighlightVertex ? SpriteEditingConstants::GeometrySelectedColor : VertexColor, GWhiteTexture);

			if (bIsHitTesting)
			{
				Canvas.SetHitProxy(nullptr);
			}
		}
	}
}

void FSpriteEditorViewportClient::DrawGeometryStats(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FSpriteGeometryCollection& Geometry, bool bIsRenderGeometry, int32& YPos)
{
	// Draw the type of geometry we're displaying stats for
	const FText GeometryName = bIsRenderGeometry ? LOCTEXT("RenderGeometry", "Render Geometry (source)") : LOCTEXT("CollisionGeometry", "Collision Geometry (source)");

	FCanvasTextItem TextItem(FVector2D(6, YPos), GeometryName, GEngine->GetSmallFont(), FLinearColor::White);
	TextItem.EnableShadow(FLinearColor::Black);

	TextItem.Draw(&Canvas);
	TextItem.Position += FVector2D(6.0f, 18.0f);

	// Draw the number of shapes
	TextItem.Text = FText::Format(LOCTEXT("PolygonCount", "Shapes: {0}"), FText::AsNumber(Geometry.Shapes.Num()));
	TextItem.Draw(&Canvas);
	TextItem.Position.Y += 18.0f;

	//@TODO: Should show the agg numbers instead for collision, and the render tris numbers for rendering
	// Draw the number of vertices
	int32 NumVerts = 0;
	for (int32 PolyIndex = 0; PolyIndex < Geometry.Shapes.Num(); ++PolyIndex)
	{
		NumVerts += Geometry.Shapes[PolyIndex].Vertices.Num();
	}

	TextItem.Text = FText::Format(LOCTEXT("VerticesCount", "Verts: {0}"), FText::AsNumber(NumVerts));
	TextItem.Draw(&Canvas);
	TextItem.Position.Y += 18.0f;

	YPos = (int32)TextItem.Position.Y;
}

void AttributeTrianglesByMaterialType(int32 NumTriangles, UMaterialInterface* Material, int32& NumOpaqueTriangles, int32& NumMaskedTriangles, int32& NumTranslucentTriangles)
{
	if (Material != nullptr)
	{
		switch (Material->GetBlendMode())
		{
		case EBlendMode::BLEND_Opaque:
			NumOpaqueTriangles += NumTriangles;
			break;
		case EBlendMode::BLEND_Translucent:
		case EBlendMode::BLEND_Additive:
		case EBlendMode::BLEND_Modulate:
			NumTranslucentTriangles += NumTriangles;
			break;
		case EBlendMode::BLEND_Masked:
			NumMaskedTriangles += NumTriangles;
			break;
		}
	}
}

void FSpriteEditorViewportClient::DrawCollisionStats(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, class UBodySetup* BodySetup, int32& YPos)
{
	FCanvasTextItem TextItem(FVector2D(6, YPos), LOCTEXT("CollisionGeomBaked", "Collision Geometry (baked)"), GEngine->GetSmallFont(), FLinearColor::White);
	TextItem.EnableShadow(FLinearColor::Black);

	TextItem.Draw(&Canvas);
	TextItem.Position += FVector2D(6.0f, 18.0f);

	// Collect stats
	const FKAggregateGeom& AggGeom3D = BodySetup->AggGeom;

	int32 NumSpheres = AggGeom3D.SphereElems.Num();
	int32 NumBoxes = AggGeom3D.BoxElems.Num();
	int32 NumCapsules = AggGeom3D.SphylElems.Num();
	int32 NumConvexElems = AggGeom3D.ConvexElems.Num();
	int32 NumConvexVerts = 0;
	bool bIs2D = false;

	for (const FKConvexElem& ConvexElement : AggGeom3D.ConvexElems)
	{
		NumConvexVerts += ConvexElement.VertexData.Num();
	}

	if (UBodySetup2D* BodySetup2D = Cast<UBodySetup2D>(BodySetup))
	{
		bIs2D = true;
		const FAggregateGeometry2D& AggGeom2D = BodySetup2D->AggGeom2D;
		NumSpheres += AggGeom2D.CircleElements.Num();
		NumBoxes += AggGeom2D.BoxElements.Num();
		NumConvexElems += AggGeom2D.ConvexElements.Num();

		for (const FConvexElement2D& ConvexElement : AggGeom2D.ConvexElements)
		{
			NumConvexVerts += ConvexElement.VertexData.Num();
		}
	}

	if (NumSpheres > 0)
	{
		static const FText SpherePrompt = LOCTEXT("SphereCount", "Spheres: {0}");
		static const FText CirclePrompt = LOCTEXT("CircleCount", "Circles: {0}");

		TextItem.Text = FText::Format(bIs2D ? CirclePrompt : SpherePrompt, FText::AsNumber(NumSpheres));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	if (NumBoxes > 0)
	{
		static const FText BoxPrompt = LOCTEXT("BoxCount", "Boxes: {0}");
		TextItem.Text = FText::Format(BoxPrompt, FText::AsNumber(NumBoxes));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	if (NumCapsules > 0)
	{
		static const FText CapsulePrompt = LOCTEXT("CapsuleCount", "Capsules: {0}");
		TextItem.Text = FText::Format(CapsulePrompt, FText::AsNumber(NumCapsules));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	if (NumConvexElems > 0)
	{
		static const FText ConvexPrompt = LOCTEXT("ConvexCount", "Convex Shapes: {0} ({1} verts)");
		TextItem.Text = FText::Format(ConvexPrompt, FText::AsNumber(NumConvexElems), FText::AsNumber(NumConvexVerts));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	if ((NumConvexElems + NumCapsules + NumBoxes + NumSpheres) == 0)
	{
		static const FText NoShapesPrompt = LOCTEXT("NoCollisionDataWarning", "Warning: Collision is enabled but there are no shapes");
		TextItem.Text = NoShapesPrompt;
		TextItem.SetColor(FLinearColor::Yellow);
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	YPos = (int32)TextItem.Position.Y;
}

void FSpriteEditorViewportClient::DrawRenderStats(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, class UPaperSprite* Sprite, int32& YPos)
{
	FCanvasTextItem TextItem(FVector2D(6, YPos), LOCTEXT("RenderGeomBaked", "Render Geometry (baked)"), GEngine->GetSmallFont(), FLinearColor::White);
	TextItem.EnableShadow(FLinearColor::Black);

	TextItem.Draw(&Canvas);
	TextItem.Position += FVector2D(6.0f, 18.0f);

	int32 NumSections = (Sprite->AlternateMaterialSplitIndex != INDEX_NONE) ? 2 : 1;
	if (NumSections > 1)
	{
		TextItem.Text = FText::Format(LOCTEXT("SectionCount", "Sections: {0}"), FText::AsNumber(NumSections));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	int32 NumOpaqueTriangles = 0;
	int32 NumMaskedTriangles = 0;
	int32 NumTranslucentTriangles = 0;

	int32 NumVerts = Sprite->BakedRenderData.Num();
	int32 DefaultTriangles = 0;
	int32 AlternateTriangles = 0;
	if (Sprite->AlternateMaterialSplitIndex != INDEX_NONE)
	{
		DefaultTriangles = Sprite->AlternateMaterialSplitIndex / 3;
		AlternateTriangles = (NumVerts - Sprite->AlternateMaterialSplitIndex) / 3;
	}
	else
	{
		DefaultTriangles = NumVerts / 3;
	}

	AttributeTrianglesByMaterialType(DefaultTriangles, Sprite->GetDefaultMaterial(), /*inout*/ NumOpaqueTriangles, /*inout*/ NumMaskedTriangles, /*inout*/ NumTranslucentTriangles);
	AttributeTrianglesByMaterialType(AlternateTriangles, Sprite->GetAlternateMaterial(), /*inout*/ NumOpaqueTriangles, /*inout*/ NumMaskedTriangles, /*inout*/ NumTranslucentTriangles);

	// Draw the number of triangles
	if (NumOpaqueTriangles > 0)
	{
		TextItem.Text = FText::Format(LOCTEXT("OpaqueTriangleCount", "Triangles: {0} (opaque)"), FText::AsNumber(NumOpaqueTriangles));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	if (NumMaskedTriangles > 0)
	{
		TextItem.Text = FText::Format(LOCTEXT("MaskedTriangleCount", "Triangles: {0} (masked)"), FText::AsNumber(NumMaskedTriangles));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	if (NumTranslucentTriangles > 0)
	{
		TextItem.Text = FText::Format(LOCTEXT("TranslucentTriangleCount", "Triangles: {0} (translucent)"), FText::AsNumber(NumTranslucentTriangles));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	if ((NumOpaqueTriangles + NumMaskedTriangles + NumTranslucentTriangles) == 0)
	{
		static const FText NoShapesPrompt = LOCTEXT("NoRenderDataWarning", "Warning: No rendering triangles (create a new shape using the toolbar)");
		TextItem.Text = NoShapesPrompt;
		TextItem.SetColor(FLinearColor::Yellow);
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

	YPos = (int32)TextItem.Position.Y;
}

void FSpriteEditorViewportClient::DrawBoundsAsText(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, int32& YPos)
{
	FNumberFormattingOptions NoDigitGroupingFormat;
	NoDigitGroupingFormat.UseGrouping = false;

	UPaperSprite* Sprite = GetSpriteBeingEdited();
	FBoxSphereBounds Bounds = Sprite->GetRenderBounds();

	const FText DisplaySizeText = FText::Format(LOCTEXT("BoundsSize", "Approx. Size: {0}x{1}x{2}"),
		FText::AsNumber((int32)(Bounds.BoxExtent.X * 2.0f), &NoDigitGroupingFormat),
		FText::AsNumber((int32)(Bounds.BoxExtent.Y * 2.0f), &NoDigitGroupingFormat),
		FText::AsNumber((int32)(Bounds.BoxExtent.Z * 2.0f), &NoDigitGroupingFormat));

	Canvas.DrawShadowedString(
		6,
		YPos,
		*DisplaySizeText.ToString(),
		GEngine->GetSmallFont(),
		FLinearColor::White);
	YPos += 18;
}

void FSpriteEditorViewportClient::DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas)
{
	const bool bIsHitTesting = Canvas.IsHitTesting();
	if (!bIsHitTesting)
	{
		Canvas.SetHitProxy(nullptr);
	}

	if (!SpriteEditorPtr.IsValid())
	{
		return;
	}

	UPaperSprite* Sprite = GetSpriteBeingEdited();

	int32 YPos = 42;

	static const FText GeomHelpStr = LOCTEXT("GeomEditHelp", "Shift + click to insert a vertex.\nSelect one or more vertices and press Delete to remove them.\nDouble click a vertex to select a polygon\n");
	static const FText GeomAddPolygonHelpStr = LOCTEXT("GeomClickAddPolygon", "Click to start creating a polygon\nCtrl + Click to start creating a subtractive polygon\n");
	static const FText GeomAddCollisionPolygonHelpStr = LOCTEXT("GeomClickAddCollisionPolygon", "Click to start creating a polygon");
	static const FText GeomAddVerticesHelpStr = LOCTEXT("GeomClickAddVertices", "Click to add points to the polygon\nEnter to finish adding a polygon\nEscape to cancel this polygon\n");
	static const FText SourceRegionHelpStr = LOCTEXT("SourceRegionHelp", "Drag handles to adjust source region\nDouble-click on an image region to select all connected pixels\nHold down Ctrl and drag a rectangle to create a new sprite at that position");

	switch (CurrentMode)
	{
	case ESpriteEditorMode::ViewMode:
	default:

		// Display the pivot
		{
			FNumberFormattingOptions NoDigitGroupingFormat;
			NoDigitGroupingFormat.UseGrouping = false;
			const FText PivotStr = FText::Format(LOCTEXT("PivotPosition", "Pivot: ({0}, {1})"), FText::AsNumber(Sprite->CustomPivotPoint.X, &NoDigitGroupingFormat), FText::AsNumber(Sprite->CustomPivotPoint.Y, &NoDigitGroupingFormat));
			FCanvasTextItem TextItem(FVector2D(6, YPos), PivotStr, GEngine->GetSmallFont(), FLinearColor::White);
			TextItem.EnableShadow(FLinearColor::Black);
			TextItem.Draw(&Canvas);
			YPos += 18;
		}

		// Baked collision data
		if (Sprite->BodySetup != nullptr)
		{
			DrawCollisionStats(Viewport, View, Canvas, Sprite->BodySetup, /*inout*/ YPos);
		}

		// Baked render data
		DrawRenderStats(Viewport, View, Canvas, Sprite, /*inout*/ YPos);

		// And bounds
		DrawBoundsAsText(Viewport, View, Canvas, /*inout*/ YPos);

		break;
	case ESpriteEditorMode::EditCollisionMode:
		{
 			// Display tool help
			{
				const FText* HelpStr;
				if (IsAddingPolygon())
				{
					HelpStr = (AddingPolygonIndex == -1) ? &GeomAddCollisionPolygonHelpStr : &GeomAddVerticesHelpStr;
				}
				else
				{
					HelpStr = &GeomHelpStr;
				}

				FCanvasTextItem TextItem(FVector2D(6, YPos), *HelpStr, GEngine->GetSmallFont(), FLinearColor::White);
				TextItem.EnableShadow(FLinearColor::Black);
				TextItem.Draw(&Canvas);
				YPos += 54;
			}

			// Draw the custom collision geometry
			DrawGeometry_CanvasPass(Viewport, View, Canvas, Sprite->CollisionGeometry, SpriteEditingConstants::CollisionShapeColor, FLinearColor::White, false);
			if (Sprite->BodySetup != nullptr)
			{
				DrawGeometryStats(Viewport, View, Canvas, Sprite->CollisionGeometry, false, /*inout*/ YPos);
				DrawCollisionStats(Viewport, View, Canvas, Sprite->BodySetup, /*inout*/ YPos);
			}
			else
			{
				FCanvasTextItem TextItem(FVector2D(6, YPos), LOCTEXT("NoCollisionDataMainScreen", "No collision data"), GEngine->GetSmallFont(), FLinearColor::White);
				TextItem.EnableShadow(FLinearColor::Black);
				TextItem.Draw(&Canvas);
			}
		}
		break;
	case ESpriteEditorMode::EditRenderingGeomMode:
		{
 			// Display tool help
			{
				const FText* HelpStr;
				if (IsAddingPolygon())
				{
					HelpStr = (AddingPolygonIndex == -1) ? &GeomAddPolygonHelpStr : &GeomAddVerticesHelpStr;
				}
				else
				{
					HelpStr = &GeomHelpStr;
				}

				FCanvasTextItem TextItem(FVector2D(6, YPos), *HelpStr, GEngine->GetSmallFont(), FLinearColor::White);
				TextItem.EnableShadow(FLinearColor::Black);
				TextItem.Draw(&Canvas);
				YPos += 54;
			}

			// Draw the custom render geometry
			DrawGeometry_CanvasPass(Viewport, View, Canvas, Sprite->RenderGeometry, SpriteEditingConstants::RenderShapeColor, SpriteEditingConstants::SubtractiveRenderShapeColor, true);
			DrawGeometryStats(Viewport, View, Canvas, Sprite->RenderGeometry, true, /*inout*/ YPos);
			DrawRenderStats(Viewport, View, Canvas, Sprite, /*inout*/ YPos);

			// And bounds
			DrawBoundsAsText(Viewport, View, Canvas, /*inout*/ YPos);
		}
		break;
	case ESpriteEditorMode::EditSourceRegionMode:
		{
			// Display tool help
			{
				FCanvasTextItem TextItem(FVector2D(6, YPos), SourceRegionHelpStr, GEngine->GetSmallFont(), FLinearColor::White);
				TextItem.EnableShadow(FLinearColor::Black);
				TextItem.Draw(&Canvas);
				YPos += 18;
			}

			if (bShowRelatedSprites)
			{
				DrawRelatedSprites(Viewport, View, Canvas, SpriteEditingConstants::SourceRegionRelatedBoundsColor);
			}

			DrawSourceRegion(Viewport, View, Canvas, SpriteEditingConstants::SourceRegionBoundsColor);
		}
		break;
	case ESpriteEditorMode::AddSpriteMode:
		//@TODO:
		break;
	}

	if (bIsMarqueeTracking)
	{
		DrawMarquee(Viewport, View, Canvas, SpriteEditingConstants::MarqueeColor);
	}

	if (bShowSockets && !IsInSourceRegionEditMode())
	{
		FSocketEditingHelper::DrawSocketNames(RenderSpriteComponent, Viewport, View, Canvas);
	}

	FEditorViewportClient::DrawCanvas(Viewport, View, Canvas);
}

void FSpriteEditorViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	// We don't draw the pivot when showing the source region
	// The pivot may be outside the actual texture bounds there
	if (bShowPivot && !bShowSourceTexture && !IsInSourceRegionEditMode())
	{
		const bool bCanSelectPivot = false;
		const bool bHitTestingForPivot = PDI->IsHitTesting() && bCanSelectPivot;
		FUnrealEdUtils::DrawWidget(View, PDI, RenderSpriteComponent->ComponentToWorld.ToMatrixWithScale(), 0, 0, EAxisList::XZ, EWidgetMovementMode::WMM_Translate, bHitTestingForPivot);
	}

	if (bShowSockets && !IsInSourceRegionEditMode())
	{
		FSocketEditingHelper::DrawSockets(RenderSpriteComponent, View, PDI);
	}

	UPaperSprite* Sprite = GetSpriteBeingEdited();
	switch (CurrentMode)
	{
	case ESpriteEditorMode::ViewMode:
	default:
		break;
	case ESpriteEditorMode::EditCollisionMode:
		// Draw the custom collision geometry
		SpriteGeometryHelper.DrawGeometry(*View, *PDI, Sprite->CollisionGeometry, SpriteEditingConstants::CollisionShapeColor, FLinearColor::White, /*bIsRenderGeometry=*/ false);
		break;
	case ESpriteEditorMode::EditRenderingGeomMode:
		// Draw the custom render geometry
		SpriteGeometryHelper.DrawGeometry(*View, *PDI, Sprite->RenderGeometry, SpriteEditingConstants::RenderShapeColor, SpriteEditingConstants::SubtractiveRenderShapeColor, /*bIsRenderGeometry=*/ true);
		break;
	}
}

void FSpriteEditorViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	FEditorViewportClient::Draw(Viewport, Canvas);
}

void FSpriteEditorViewportClient::Tick(float DeltaSeconds)
{
	if (UPaperSprite* Sprite = RenderSpriteComponent->GetSprite())
	{
		//@TODO: Doesn't need to happen every frame, only when properties are updated

		// Update the source texture view sprite (in case the texture has changed)
		UpdateSourceTextureSpriteFromSprite(Sprite);

		// Reposition the sprite (to be at the correct relative location to it's parent, undoing the pivot behavior)
		const FVector2D PivotInTextureSpace = Sprite->ConvertPivotSpaceToTextureSpace(FVector2D::ZeroVector);
		const FVector PivotInWorldSpace = TextureSpaceToWorldSpace(PivotInTextureSpace);
		RenderSpriteComponent->SetRelativeLocation(PivotInWorldSpace);

		bool bSourceTextureViewComponentVisibility = bShowSourceTexture || IsInSourceRegionEditMode();
		if (bSourceTextureViewComponentVisibility != SourceTextureViewComponent->IsVisible())
		{
			bDeferZoomToSprite = true;
			SourceTextureViewComponent->SetVisibility(bSourceTextureViewComponentVisibility);
		}

		bool bRenderTextureViewComponentVisibility = !IsInSourceRegionEditMode();
		if (bRenderTextureViewComponentVisibility != RenderSpriteComponent->IsVisible())
		{
			bDeferZoomToSprite = true;
			RenderSpriteComponent->SetVisibility(bRenderTextureViewComponentVisibility);
		}

		// Zoom in on the sprite
		//@TODO: Fix this properly so it doesn't need to be deferred, or wait for the viewport to initialize
		FIntPoint Size = Viewport->GetSizeXY();
		if (bDeferZoomToSprite && (Size.X > 0) && (Size.Y > 0))
		{
			UPaperSpriteComponent* ComponentToFocusOn = SourceTextureViewComponent->IsVisible() ? SourceTextureViewComponent : RenderSpriteComponent;
			FocusViewportOnBox(ComponentToFocusOn->Bounds.GetBox(), true);
			bDeferZoomToSprite = false;
		}		
	}

	if (bIsMarqueeTracking)
	{
		int32 HitX = Viewport->GetMouseX();
		int32 HitY = Viewport->GetMouseY();
		MarqueeEndPos = FVector2D(HitX, HitY);
	}

	FPaperEditorViewportClient::Tick(DeltaSeconds);

	if (!GIntraFrameDebuggingGameThread)
	{
		OwnedPreviewScene.GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

void FSpriteEditorViewportClient::ToggleShowSourceTexture()
{
	bShowSourceTexture = !bShowSourceTexture;
	SourceTextureViewComponent->SetVisibility(bShowSourceTexture);
	
	Invalidate();
}

void FSpriteEditorViewportClient::ToggleShowMeshEdges()
{
	EngineShowFlags.MeshEdges = !EngineShowFlags.MeshEdges;
	Invalidate(); 
}

bool FSpriteEditorViewportClient::IsShowMeshEdgesChecked() const
{
	return EngineShowFlags.MeshEdges;
}

// Find all related sprites (not including self)
void FSpriteEditorViewportClient::UpdateRelatedSpritesList()
{
	UPaperSprite* Sprite = GetSpriteBeingEdited();
	UTexture2D* Texture = Sprite->GetSourceTexture();
	if (Texture != nullptr)
	{
		FARFilter Filter;
		Filter.ClassNames.Add(UPaperSprite::StaticClass()->GetFName());
		const FString TextureString = FAssetData(Texture).GetExportTextName();
		const FName SourceTexturePropName(TEXT("SourceTexture"));
		Filter.TagsAndValues.Add(SourceTexturePropName, TextureString);
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> SpriteAssetData;
		AssetRegistryModule.Get().GetAssets(Filter, SpriteAssetData);

		FAssetData CurrentAssetData(Sprite);

		RelatedSprites.Empty();
		for (int32 i = 0; i < SpriteAssetData.Num(); ++i)
		{
			FAssetData& SpriteAsset = SpriteAssetData[i];
			if (SpriteAsset == Sprite)
			{
				continue;
			}

			const FString* SourceUVString = SpriteAsset.TagsAndValues.Find("SourceUV");
			const FString* SourceDimensionString = SpriteAsset.TagsAndValues.Find("SourceDimension");
			FVector2D SourceUV, SourceDimension;
			if (SourceUVString != nullptr && SourceDimensionString != nullptr)
			{
				if (SourceUV.InitFromString(*SourceUVString) && SourceDimension.InitFromString(*SourceDimensionString))
				{
					FRelatedSprite RelatedSprite;
					RelatedSprite.AssetData = SpriteAsset;
					RelatedSprite.SourceUV = SourceUV;
					RelatedSprite.SourceDimension = SourceDimension;
					
					RelatedSprites.Add(RelatedSprite);
				}
			}
		}
	}
}

void FSpriteEditorViewportClient::UpdateMouseDelta()
{
	FPaperEditorViewportClient::UpdateMouseDelta();
}

void FSpriteEditorViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	const FViewportClick Click(&View, this, Key, Event, HitX, HitY);
	const bool bIsCtrlKeyDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	const bool bIsShiftKeyDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);
	const bool bIsAltKeyDown = Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);
	bool bHandled = false;

	const bool bAllowSelectVertex = !(IsEditingGeometry() && IsAddingPolygon()) && !bIsShiftKeyDown;

	const bool bClearSelectionModifier = bIsCtrlKeyDown;
	const bool bDeleteClickedVertex = bIsAltKeyDown;
	const bool bInsertVertexModifier = bIsShiftKeyDown;
	HSpriteSelectableObjectHitProxy* SelectedItemProxy = HitProxyCast<HSpriteSelectableObjectHitProxy>(HitProxy);

	if (bAllowSelectVertex && (SelectedItemProxy != nullptr))
	{
		if (!bClearSelectionModifier)
		{
			ClearSelectionSet();
		}

		if (bDeleteClickedVertex)
		{
			// Delete selection
			if (const FSpriteSelectedVertex* SelectedVertex = SelectedItemProxy->Data->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex))
			{
				ClearSelectionSet();
				SelectedItemSet.Add(SelectedItemProxy->Data);
				SelectedIDSet.Add(FShapeVertexPair(SelectedVertex->ShapeIndex, SelectedVertex->VertexIndex));
				DeleteSelection();
			}
			else if (const FSpriteSelectedShape* SelectedShape = SelectedItemProxy->Data->CastTo<const FSpriteSelectedShape>(FSelectionTypes::GeometryShape))
			{
				ClearSelectionSet();
				SelectedItemSet.Add(SelectedItemProxy->Data);
				SelectedIDSet.Add(FShapeVertexPair(SelectedShape->ShapeIndex, INDEX_NONE));
				DeleteSelection();
			}
		}
		else if (Event == EInputEvent::IE_DoubleClick)
		{
			// Double click to select a polygon
			if (const FSpriteSelectedVertex* SelectedVertex = SelectedItemProxy->Data->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex))
			{
				if (SelectedVertex->IsValidInEditor(GetSpriteBeingEdited(), IsInRenderingEditMode()))
				{
					//findme
					SelectShape(SelectedVertex->ShapeIndex);
				}
			}
		}
		else
		{
			//@TODO: This needs to be generalized!
			if (const FSpriteSelectedEdge* SelectedEdge = SelectedItemProxy->Data->CastTo<const FSpriteSelectedEdge>(FSelectionTypes::Edge))
			{
				// Add the next vertex defined by this edge
				AddPolygonEdgeToSelection(SelectedEdge->ShapeIndex, SelectedEdge->VertexIndex);
			}
			else if (const FSpriteSelectedVertex* SelectedVertex = SelectedItemProxy->Data->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex))
			{
				AddPolygonVertexToSelection(SelectedVertex->ShapeIndex, SelectedVertex->VertexIndex);
			}
			else if (const FSpriteSelectedShape* SelectedShape = SelectedItemProxy->Data->CastTo<const FSpriteSelectedShape>(FSelectionTypes::GeometryShape))
			{
				SelectShape(SelectedShape->ShapeIndex);
			}
			else
			{
				SelectedItemSet.Add(SelectedItemProxy->Data);
			}
		}

		bHandled = true;
	}
// 	else if (HWidgetUtilProxy* PivotProxy = HitProxyCast<HWidgetUtilProxy>(HitProxy))
// 	{
// 		//const bool bUserWantsPaint = bIsLeftButtonDown && ( !GetDefault<ULevelEditorViewportSettings>()->bLeftMouseDragMovesCamera ||  bIsCtrlDown );
// 		//findme
// 		WidgetAxis = WidgetProxy->Axis;
// 
// 			// Calculate the screen-space directions for this drag.
// 			FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues( Viewport, GetScene(), EngineShowFlags ));
// 			FSceneView* View = CalcSceneView(&ViewFamily);
// 			WidgetProxy->CalcVectors(View, FViewportClick(View, this, Key, Event, HitX, HitY), LocalManipulateDir, WorldManipulateDir, DragX, DragY);
// 			bHandled = true;
// 	}
	else
	{
		if (IsInSourceRegionEditMode())
		{
			ClearSelectionSet();

			if (Event == EInputEvent::IE_DoubleClick)
			{
				FVector4 WorldPoint = View.PixelToWorld(HitX, HitY, 0);
				UPaperSprite* Sprite = GetSpriteBeingEdited();
				FVector2D TexturePoint = SourceTextureViewComponent->GetSprite()->ConvertWorldSpaceToTextureSpace(WorldPoint);
				if (bIsCtrlKeyDown)
				{
					UPaperSprite* NewSprite = CreateNewSprite(Sprite->GetSourceUV(), Sprite->GetSourceSize());
					if (NewSprite != nullptr)
					{
						NewSprite->ExtractSourceRegionFromTexturePoint(TexturePoint);
						bHandled = true;
					}
				}
				else
				{
					Sprite->ExtractSourceRegionFromTexturePoint(TexturePoint);
					bHandled = true;
				}
			}
			else
			{
				FVector4 WorldPoint = View.PixelToWorld(HitX, HitY, 0);
				FVector2D TexturePoint = SourceTextureViewComponent->GetSprite()->ConvertWorldSpaceToTextureSpace(WorldPoint);
				for (int32 RelatedSpriteIndex = 0; RelatedSpriteIndex < RelatedSprites.Num(); ++RelatedSpriteIndex)
				{
					FRelatedSprite& RelatedSprite = RelatedSprites[RelatedSpriteIndex];
					if ((TexturePoint.X >= RelatedSprite.SourceUV.X) && (TexturePoint.Y >= RelatedSprite.SourceUV.Y) &&
						(TexturePoint.X < (RelatedSprite.SourceUV.X + RelatedSprite.SourceDimension.X)) &&
						(TexturePoint.Y < (RelatedSprite.SourceUV.Y + RelatedSprite.SourceDimension.Y)))
					{
						// Select this sprite
						if (UPaperSprite* LoadedSprite = Cast<UPaperSprite>(RelatedSprite.AssetData.GetAsset()))
						{
							if (SpriteEditorPtr.IsValid())
							{
								SpriteEditorPtr.Pin()->SetSpriteBeingEdited(LoadedSprite);
								break;
							}
						}
						bHandled = true;
					}
				}
			}
		}
		else if (IsEditingGeometry() && !IsAddingPolygon())
		{
			if (bInsertVertexModifier)
			{
				FVector4 WorldPoint = View.PixelToWorld(HitX, HitY, 0);
				UPaperSprite* Sprite = RenderSpriteComponent->GetSprite();
				if (Sprite != nullptr)
				{
					FVector2D SpriteSpaceClickPoint = Sprite->ConvertWorldSpaceToTextureSpace(WorldPoint);

					// find a polygon to add vert to
					if (SelectedItemSet.Num() > 0)
					{
						TSharedPtr<FSelectedItem> SelectedItemPtr = *SelectedItemSet.CreateIterator();

						if (const FSpriteSelectedVertex* SelectedVertex = SelectedItemPtr->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex)) //@TODO:  Inflexible?
						{
							if (SelectedVertex->IsValidInEditor(Sprite, IsInRenderingEditMode()))
							{
								AddPointToGeometry(SpriteSpaceClickPoint, SelectedVertex->ShapeIndex);
							}
						}
					}
					else
					{
						AddPointToGeometry(SpriteSpaceClickPoint);
					}
					bHandled = true;
				}
			}
		}
		else if (!IsEditingGeometry())
		{
			// Clicked on the background (missed any proxies), deselect the socket or whatever was selected
			ClearSelectionSet();
		}

		if (!bHandled)
		{
			FPaperEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
		}
	}
}

// Create a new sprite and return this sprite. The sprite editor will now be editing this new sprite
// Returns nullptr if failed
UPaperSprite* FSpriteEditorViewportClient::CreateNewSprite(FVector2D TopLeft, FVector2D Dimensions)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	UPaperSprite* CurrentSprite = GetSpriteBeingEdited();
	UPaperSprite* CreatedSprite = nullptr;

	// Create the factory used to generate the sprite
	UPaperSpriteFactory* SpriteFactory = NewObject<UPaperSpriteFactory>();
	SpriteFactory->InitialTexture = CurrentSprite->SourceTexture;
	SpriteFactory->bUseSourceRegion = true;
	SpriteFactory->InitialSourceUV = TopLeft;
	SpriteFactory->InitialSourceDimension = Dimensions;


	// Get a unique name for the sprite
	FString Name, PackageName;
	AssetToolsModule.Get().CreateUniqueAssetName(CurrentSprite->GetOutermost()->GetName(), TEXT(""), /*out*/ PackageName, /*out*/ Name);
	const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
	if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, PackagePath, UPaperSprite::StaticClass(), SpriteFactory))
	{
		TArray<UObject*> Objects;
		Objects.Add(NewAsset);
		ContentBrowserModule.Get().SyncBrowserToAssets(Objects);

		UPaperSprite* NewSprite = Cast<UPaperSprite>(NewAsset);
		if (SpriteEditorPtr.IsValid() && NewSprite != nullptr)
		{
			SpriteEditorPtr.Pin()->SetSpriteBeingEdited(NewSprite);
		}

		CreatedSprite = NewSprite;
	}

	return CreatedSprite;
}

bool FSpriteEditorViewportClient::ProcessMarquee(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, bool bMarqueeStartModifierPressed)
{
	bool bMarqueeReady = false;

	if (Key == EKeys::LeftMouseButton)
	{
		int32 HitX = Viewport->GetMouseX();
		int32 HitY = Viewport->GetMouseY();
		if (Event == IE_Pressed && bMarqueeStartModifierPressed)
		{
			HHitProxy* HitResult = Viewport->GetHitProxy(HitX, HitY);
			
			if ((HitResult == nullptr) || (HitResult->Priority == HPP_World))
			{
				bIsMarqueeTracking = true;
				MarqueeStartPos = FVector2D(HitX, HitY);
				MarqueeEndPos = MarqueeStartPos;
			}
		}
		else if (bIsMarqueeTracking && (Event == IE_Released))
		{
			MarqueeEndPos = FVector2D(HitX, HitY);
			bIsMarqueeTracking = false;
			bMarqueeReady = true;
		}
	}
	else if (bIsMarqueeTracking && (Key == EKeys::Escape))
	{
		// Cancel marquee selection
		bIsMarqueeTracking = false;
	}

	return bMarqueeReady;
}

bool FSpriteEditorViewportClient::InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad)
{
	bool bHandled = false;
	FInputEventState InputState(Viewport, Key, Event);

	// Handle marquee tracking in source region edit mode
	if (IsInSourceRegionEditMode())
	{
		bool bMarqueeStartModifier = InputState.IsCtrlButtonPressed();
		if (ProcessMarquee(Viewport, ControllerId, Key, Event, bMarqueeStartModifier))
		{
			FVector2D TextureSpaceStartPos, TextureSpaceDimensions;
			if (ConvertMarqueeToSourceTextureSpace(/*out*/TextureSpaceStartPos, /*out*/TextureSpaceDimensions))
			{
				//@TODO: Warn if overlapping with another sprite
				CreateNewSprite(TextureSpaceStartPos, TextureSpaceDimensions);
			}
		}
	}
	else if (IsEditingGeometry())
	{
		if (IsAddingPolygon())
		{
			if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
			{
				int32 HitX = Viewport->GetMouseX();
				int32 HitY = Viewport->GetMouseY();

				// Calculate world space positions
				FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, GetScene(), EngineShowFlags));
				FSceneView* View = CalcSceneView(&ViewFamily);
				FVector WorldPoint = View->PixelToWorld(HitX, HitY, 0);

				UPaperSprite* Sprite = GetSpriteBeingEdited();
				FVector2D TexturePoint = Sprite->ConvertWorldSpaceToTextureSpace(WorldPoint);

				BeginTransaction(LOCTEXT("AddPolygonVertexTransaction", "Add Vertex to Polygon"));

				FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited();
				if (AddingPolygonIndex == -1)
				{
					FSpriteGeometryShape NewPolygon;
					
					const bool bIsCtrlKeyDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
					NewPolygon.bNegativeWinding = CanAddSubtractivePolygon() && bIsCtrlKeyDown;

					Geometry->Shapes.Add(NewPolygon);
					AddingPolygonIndex = Geometry->Shapes.Num() - 1;
				}

				if (Geometry->Shapes.IsValidIndex(AddingPolygonIndex))
				{
					FSpriteGeometryShape& Shape = Geometry->Shapes[AddingPolygonIndex];
					Shape.ShapeType = ESpriteShapeType::Polygon;
					Shape.Vertices.Add(Shape.ConvertTextureSpaceToShapeSpace(TexturePoint));
				}
				else
				{
					ResetAddPolygonMode();
				}

				Geometry->GeometryType = ESpritePolygonMode::FullyCustom;

				bManipulationDirtiedSomething = true;
				EndTransaction();
			}
			else if (Key == EKeys::Enter)
			{
				ResetAddPolygonMode();
			}
			else if ((Key == EKeys::Escape) && (AddingPolygonIndex != -1))
			{
				FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited();
				if (Geometry->Shapes.IsValidIndex(AddingPolygonIndex))
				{
					BeginTransaction(LOCTEXT("DeletePolygon", "Delete Polygon"));
					Geometry->Shapes.RemoveAt(AddingPolygonIndex);
					bManipulationDirtiedSomething = true;
					EndTransaction();
				}
				ResetAddPolygonMode();
			}
		}
		else
		{
			if (ProcessMarquee(Viewport, ControllerId, Key, Event, true))
			{
				bool bAddingToSelection = InputState.IsShiftButtonPressed(); //@TODO: control button moves widget? Hopefully make this more consistent when that is changed
				SelectVerticesInMarquee(bAddingToSelection);
			}
		}
	}

	// Start the drag
	//@TODO: EKeys::LeftMouseButton
	//@TODO: Event.IE_Pressed
	// Implement InputAxis
	// StartTracking

	// Pass keys to standard controls, if we didn't consume input
	return (bHandled) ? true : FEditorViewportClient::InputKey(Viewport,  ControllerId, Key, Event, AmountDepressed, bGamepad);
}

bool FSpriteEditorViewportClient::InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
	bool bHandled = false;
	if (bManipulating && (CurrentAxis != EAxisList::None))
	{
		bHandled = true;

		const FWidget::EWidgetMode MoveMode = GetWidgetMode();

		// Negate Y because vertices are in source texture space, not world space
		const FVector2D Drag2D(FVector::DotProduct(Drag, PaperAxisX), -FVector::DotProduct(Drag, PaperAxisY));

		// Apply the delta to all of the selected objects
		for (auto SelectionIt = SelectedItemSet.CreateConstIterator(); SelectionIt; ++SelectionIt)
		{
			TSharedPtr<FSelectedItem> SelectedItem = *SelectionIt;
			SelectedItem->ApplyDelta(Drag2D, Rot, Scale, MoveMode);
		}

		if (SelectedItemSet.Num() > 0)
		{
			if (!IsInSourceRegionEditMode())
			{
				UPaperSprite* Sprite = GetSpriteBeingEdited();
				// Sprite->RebuildCollisionData();
				// Sprite->RebuildRenderData();
				Sprite->PostEditChange();
				Invalidate();
			}

			bManipulationDirtiedSomething = true;
		}
	}

	return bHandled;
}

void FSpriteEditorViewportClient::TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge)
{
	if (!bManipulating && bIsDragging)
	{
		BeginTransaction(LOCTEXT("ModificationInViewport", "Modification in Viewport"));
		bManipulating = true;
		bManipulationDirtiedSomething = false;
	}
}

void FSpriteEditorViewportClient::TrackingStopped() 
{
	if (bManipulating)
	{
		EndTransaction();
		bManipulating = false;
	}
}

FWidget::EWidgetMode FSpriteEditorViewportClient::GetWidgetMode() const
{
	return (SelectedItemSet.Num() > 0) ? DesiredWidgetMode : FWidget::WM_None;
}

void FSpriteEditorViewportClient::SetWidgetMode(FWidget::EWidgetMode NewMode)
{
	DesiredWidgetMode = NewMode;
}

bool FSpriteEditorViewportClient::CanSetWidgetMode(FWidget::EWidgetMode NewMode) const
{
	return CanCycleWidgetMode();
}

bool FSpriteEditorViewportClient::CanCycleWidgetMode() const
{
	if (!Widget->IsDragging())
	{
		return (SelectedItemSet.Num() > 0);
	}
	return false;
}

FVector FSpriteEditorViewportClient::GetWidgetLocation() const
{
	if (SelectedItemSet.Num() > 0)
	{
		UPaperSprite* Sprite = GetSpriteBeingEdited();

		// Find the center of the selection set
		FVector SummedPos(ForceInitToZero);
		for (auto SelectionIt = SelectedItemSet.CreateConstIterator(); SelectionIt; ++SelectionIt)
		{
			TSharedPtr<FSelectedItem> SelectedItem = *SelectionIt;
			SummedPos += SelectedItem->GetWorldPos();
		}
		return (SummedPos / SelectedItemSet.Num());
	}

	return FVector::ZeroVector;
}

FMatrix FSpriteEditorViewportClient::GetWidgetCoordSystem() const
{
	return FMatrix::Identity;
}

ECoordSystem FSpriteEditorViewportClient::GetWidgetCoordSystemSpace() const
{
	return COORD_World;
}

FLinearColor FSpriteEditorViewportClient::GetBackgroundColor() const
{
	return FEditorViewportClient::GetBackgroundColor();
}

FVector2D FSpriteEditorViewportClient::SelectedItemConvertWorldSpaceDeltaToLocalSpace(const FVector& WorldSpaceDelta) const
{
	UPaperSprite* Sprite = GetSpriteBeingEdited();
	return Sprite->ConvertWorldSpaceDeltaToTextureSpace(WorldSpaceDelta);
}

FVector FSpriteEditorViewportClient::TextureSpaceToWorldSpace(const FVector2D& SourcePoint) const
{
	UPaperSprite* Sprite = RenderSpriteComponent->GetSprite();
	return Sprite->ConvertTextureSpaceToWorldSpace(SourcePoint);
}

bool FSpriteEditorViewportClient::SelectedItemIsSelected(const struct FShapeVertexPair& Item) const
{
	return SelectedIDSet.Contains(Item);
}

float FSpriteEditorViewportClient::SelectedItemGetUnitsPerPixel() const
{
	UPaperSprite* Sprite = GetSpriteBeingEdited();
	return Sprite->GetUnrealUnitsPerPixel();
}

void FSpriteEditorViewportClient::BeginTransaction(const FText& SessionName)
{
	if (ScopedTransaction == nullptr)
	{
		ScopedTransaction = new FScopedTransaction(SessionName);

		UPaperSprite* Sprite = GetSpriteBeingEdited();
		Sprite->Modify();
	}
}

void FSpriteEditorViewportClient::EndTransaction()
{
	if (bManipulationDirtiedSomething)
	{
		UPaperSprite* Sprite = RenderSpriteComponent->GetSprite();

		if (IsInSourceRegionEditMode())
		{
			// Snap to pixel grid at the end of the drag
			Sprite->SourceUV.X = FMath::Max(FMath::RoundToFloat(Sprite->SourceUV.X), 0.0f);
			Sprite->SourceUV.Y = FMath::Max(FMath::RoundToFloat(Sprite->SourceUV.Y), 0.0f);
			Sprite->SourceDimension.X = FMath::Max(FMath::RoundToFloat(Sprite->SourceDimension.X), 0.0f);
			Sprite->SourceDimension.Y = FMath::Max(FMath::RoundToFloat(Sprite->SourceDimension.Y), 0.0f);
		}

		Sprite->PostEditChange();
	}
	
	bManipulationDirtiedSomething = false;

	if (ScopedTransaction != nullptr)
	{
		delete ScopedTransaction;
		ScopedTransaction = nullptr;
	}
}

void FSpriteEditorViewportClient::NotifySpriteBeingEditedHasChanged()
{
	//@TODO: Ideally we do this before switching
	EndTransaction();
	ClearSelectionSet();
	ResetAddPolygonMode();

	// Update components to know about the new sprite being edited
	UPaperSprite* Sprite = GetSpriteBeingEdited();

	RenderSpriteComponent->SetSprite(Sprite);
	UpdateSourceTextureSpriteFromSprite(Sprite);

	if (IsInSourceRegionEditMode()) 
	{
		UpdateRelatedSpritesList();
	}

	//
	bDeferZoomToSprite = true;
}

void FSpriteEditorViewportClient::FocusOnSprite()
{
	FocusViewportOnBox(RenderSpriteComponent->Bounds.GetBox());
}

// Be sure to call this with polygonIndex and VertexIndex in descending order
// Returns true if the shape went to zero points and should be deleted itself
static bool DeleteVertexInPolygonInternal(UPaperSprite* Sprite, FSpriteGeometryCollection* Geometry, const int32 ShapeIndex, const int32 VertexIndex)
{
	if (Geometry->Shapes.IsValidIndex(ShapeIndex))
	{
		FSpriteGeometryShape& Shape = Geometry->Shapes[ShapeIndex];
		if (Shape.Vertices.IsValidIndex(VertexIndex))
		{
			Shape.ShapeType = ESpriteShapeType::Polygon;
			Shape.Vertices.RemoveAt(VertexIndex);
			
			// Delete polygon
			if (Shape.Vertices.Num() == 0)
			{
				return true;
			}
		}
	}

	return false;
}

void FSpriteEditorViewportClient::DeleteSelection()
{
	// Make a set of polygon-vertices "pairs" to delete
	TSet<FShapeVertexPair> CompositeIndicesSet;
	TSet<int32> ShapesToDeleteSet;

	UPaperSprite* Sprite = GetSpriteBeingEdited();
	FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited();
	for (TSharedPtr<FSelectedItem> SelectionIt : SelectedItemSet)
	{
		if (const FSpriteSelectedVertex* SelectedVertex = SelectionIt->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex))
		{
			const bool bValidSelectedVertex = SelectedVertex->IsValidInEditor(Sprite, IsInRenderingEditMode());
			if (bValidSelectedVertex)
			{
				CompositeIndicesSet.Add(FShapeVertexPair(SelectedVertex->ShapeIndex, SelectedVertex->VertexIndex));
	
				if (SelectedVertex->IsA(FSelectionTypes::Edge)) // add the "next" point for the edge
				{
					const int32 NextIndex = (SelectedVertex->VertexIndex + 1) % Geometry->Shapes[SelectedVertex->ShapeIndex].Vertices.Num();
					CompositeIndicesSet.Add(FShapeVertexPair(SelectedVertex->ShapeIndex, NextIndex));
				}
			}
		}
		else if (const FSpriteSelectedShape* SelectedShape = SelectionIt->CastTo<const FSpriteSelectedShape>(FSelectionTypes::GeometryShape))
		{
			ShapesToDeleteSet.Add(SelectedShape->ShapeIndex);
		}
	}

	if ((CompositeIndicesSet.Num() > 0) || (ShapesToDeleteSet.Num() > 0))
	{
		BeginTransaction(LOCTEXT("DeleteSelectionTransaction", "Delete Selection"));

		// Delete the selected vertices first, as they may cause entire shapes to need to be deleted (sort so we delete from the back first)
		TArray<FShapeVertexPair> CompositeIndices = CompositeIndicesSet.Array();
		CompositeIndices.Sort([](const FShapeVertexPair& A, const FShapeVertexPair& B) { return (A.VertexIndex > B.VertexIndex); });
		for (const FShapeVertexPair& Composite : CompositeIndices)
		{
			const int32 ShapeIndex = Composite.ShapeIndex;
			const int32 VertexIndex = Composite.VertexIndex;
			UE_LOG(LogPaper2DEditor, Verbose, TEXT("Deleting vertex %d from shape %d"), VertexIndex, ShapeIndex);
			if (DeleteVertexInPolygonInternal(Sprite, Geometry, ShapeIndex, VertexIndex))
			{
				ShapesToDeleteSet.Add(ShapeIndex);
			}
		}

		// Delete the selected shapes (plus any shapes that became empty due to selected vertices)
		if (ShapesToDeleteSet.Num() > 0)
		{
			TArray<int32> ShapesToDeleteIndicies = ShapesToDeleteSet.Array();
			ShapesToDeleteIndicies.Sort([](const int32& A, const int32& B) { return (A > B); });
			for (const int32 ShapeToDeleteIndex : ShapesToDeleteIndicies)
			{
				UE_LOG(LogPaper2DEditor, Verbose, TEXT("Deleting shape %d"), ShapeToDeleteIndex);
				Geometry->Shapes.RemoveAt(ShapeToDeleteIndex);
			}
		}

		Geometry->GeometryType = ESpritePolygonMode::FullyCustom;
		bManipulationDirtiedSomething = true;

		ClearSelectionSet();
		EndTransaction();
		ResetAddPolygonMode();
	}
}

void FSpriteEditorViewportClient::SplitEdge()
{
	BeginTransaction(LOCTEXT("SplitEdgeTransaction", "Split Edge"));

	//@TODO: Really need a proper data structure!!!
	if (SelectedItemSet.Num() == 1)
	{
		// Split this item
		TSharedPtr<FSelectedItem> SelectedItem = *SelectedItemSet.CreateIterator();
		SelectedItem->SplitEdge();
		ClearSelectionSet();
		bManipulationDirtiedSomething = true;
	}
	else
	{
		// Can't handle this without a better DS, as the indices will get screwed up
	}

	EndTransaction();
}

void FSpriteEditorViewportClient::ToggleAddPolygonMode()
{
	if (IsAddingPolygon())
	{
		ResetAddPolygonMode();
	}
	else
	{
		ClearSelectionSet();
		
		bIsAddingPolygon = true; 
		AddingPolygonIndex = -1; 
		Invalidate();
	}
}

void FSpriteEditorViewportClient::AddBoxShape()
{
	if (FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited())
	{
		UPaperSprite* Sprite = GetSpriteBeingEdited();
		check(Sprite);

		BeginTransaction(LOCTEXT("AddBoxShapeTransaction", "Add Box Shape"));

		const FVector2D BoxSize(Sprite->GetSourceSize());
		const FVector2D BoxLocation(BoxSize * 0.5f);
		Geometry->GeometryType = ESpritePolygonMode::FullyCustom;
		Geometry->AddRectangleShape(BoxLocation, BoxSize);

		// Select the new geometry
		ClearSelectionSet();
		SelectShape(Geometry->Shapes.Num() - 1);

		Sprite->PostEditChange();
		EndTransaction();
	}
}

bool FSpriteEditorViewportClient::CanAddBoxShape() const
{
	return IsEditingGeometry();
}

void FSpriteEditorViewportClient::AddCircleShape()
{
	check(IsInCollisionEditMode());
	if (FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited())
	{
		UPaperSprite* Sprite = GetSpriteBeingEdited();
		check(Sprite);

		BeginTransaction(LOCTEXT("AddCircleShapeTransaction", "Add Circle Shape"));

		const FVector2D SpriteBounds = Sprite->GetSourceSize();
		const float SmallerBoundingAxisSize = SpriteBounds.GetMin();
		const FVector2D EllipseSize(SmallerBoundingAxisSize, SmallerBoundingAxisSize);
		const FVector2D EllipseLocation = SpriteBounds * 0.5f;

		Geometry->GeometryType = ESpritePolygonMode::FullyCustom;
		Geometry->AddCircleShape(EllipseLocation, EllipseSize);

		// Select the new geometry
		ClearSelectionSet();
		SelectShape(Geometry->Shapes.Num() - 1);

		Sprite->PostEditChange();
		EndTransaction();
	}
}

bool FSpriteEditorViewportClient::CanAddCircleShape() const
{
	return IsInCollisionEditMode();
}

void FSpriteEditorViewportClient::SnapAllVerticesToPixelGrid()
{
	if (FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited())
	{
		BeginTransaction(LOCTEXT("SnapAllVertsToPixelGridTransaction", "Snap All Verts to Pixel Grid"));

		for (FSpriteGeometryShape& Shape : Geometry->Shapes)
		{
			bManipulationDirtiedSomething = true;

			Shape.BoxPosition.X = FMath::RoundToInt(Shape.BoxPosition.X);
			Shape.BoxPosition.Y = FMath::RoundToInt(Shape.BoxPosition.Y);
			
			if ((Shape.ShapeType == ESpriteShapeType::Box) || (Shape.ShapeType == ESpriteShapeType::Circle))
			{
				//@TODO: Should we snap BoxPosition also, or just the verts?
				const FVector2D OldHalfSize = Shape.BoxSize * 0.5f;
				FVector2D TopLeft = Shape.BoxPosition - OldHalfSize;
				FVector2D BottomRight = Shape.BoxPosition + OldHalfSize;
				TopLeft.X = FMath::RoundToInt(TopLeft.X);
				TopLeft.Y = FMath::RoundToInt(TopLeft.Y);
				BottomRight.X = FMath::RoundToInt(BottomRight.X);
				BottomRight.Y = FMath::RoundToInt(BottomRight.Y);
				Shape.BoxPosition = (TopLeft + BottomRight) * 0.5f;
				Shape.BoxSize = BottomRight - TopLeft;
			}

			for (FVector2D& Vertex : Shape.Vertices)
			{
				FVector2D TextureSpaceVertex = Shape.ConvertShapeSpaceToTextureSpace(Vertex);
				TextureSpaceVertex.X = FMath::RoundToInt(TextureSpaceVertex.X);
				TextureSpaceVertex.Y = FMath::RoundToInt(TextureSpaceVertex.Y);
				Vertex = Shape.ConvertTextureSpaceToShapeSpace(TextureSpaceVertex);
			}
		}

		EndTransaction();
	}
}

void FSpriteEditorViewportClient::InternalActivateNewMode(ESpriteEditorMode::Type NewMode)
{
	CurrentMode = NewMode;
	Viewport->InvalidateHitProxy();
	ClearSelectionSet();
	ResetMarqueeTracking();
	ResetAddPolygonMode();
}

FSpriteGeometryCollection* FSpriteEditorViewportClient::GetGeometryBeingEdited() const
{
	UPaperSprite* Sprite = GetSpriteBeingEdited();
	switch (CurrentMode)
	{
	case ESpriteEditorMode::EditCollisionMode:
		return &(Sprite->CollisionGeometry);
	case ESpriteEditorMode::EditRenderingGeomMode:
		return &(Sprite->RenderGeometry);
	default:
		return nullptr;
	}
}

void FSpriteEditorViewportClient::SelectShape(const int32 ShapeIndex)
{
	FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited();
	if (ensure((Geometry != nullptr) && Geometry->Shapes.IsValidIndex(ShapeIndex)))
	{
		if (!IsShapeSelected(ShapeIndex))
		{
			FSpriteGeometryShape& Shape = Geometry->Shapes[ShapeIndex];

			TSharedPtr<FSpriteSelectedShape> SelectedShape = MakeShareable(new FSpriteSelectedShape(*this, *Geometry, ShapeIndex));

			SelectedItemSet.Add(SelectedShape);
			SelectedIDSet.Add(FShapeVertexPair(ShapeIndex, INDEX_NONE));
		}
	}
}

bool FSpriteEditorViewportClient::IsShapeSelected(const int32 ShapeIndex) const
{
	return SelectedIDSet.Contains(FShapeVertexPair(ShapeIndex, INDEX_NONE));
}

bool FSpriteEditorViewportClient::IsPolygonVertexSelected(const int32 ShapeIndex, const int32 VertexIndex) const
{
	return SelectedIDSet.Contains(FShapeVertexPair(ShapeIndex, VertexIndex));
}

void FSpriteEditorViewportClient::AddPolygonVertexToSelection(const int32 ShapeIndex, const int32 VertexIndex)
{
	FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited();
	if (ensure((Geometry != nullptr) && Geometry->Shapes.IsValidIndex(ShapeIndex)))
	{
		FSpriteGeometryShape& Shape = Geometry->Shapes[ShapeIndex];
		if (Shape.Vertices.IsValidIndex(VertexIndex))
		{
			if (!IsPolygonVertexSelected(ShapeIndex, VertexIndex))
			{
				TSharedPtr<FSpriteSelectedVertex> Vertex = MakeShareable(new FSpriteSelectedVertex());
				Vertex->SpritePtr = GetSpriteBeingEdited();
				Vertex->ShapeIndex = ShapeIndex;
				Vertex->VertexIndex = VertexIndex;
				Vertex->bRenderData = IsInRenderingEditMode();

				SelectedItemSet.Add(Vertex);
				SelectedIDSet.Add(FShapeVertexPair(ShapeIndex, VertexIndex));
			}
		}
	}
}

void FSpriteEditorViewportClient::AddPolygonEdgeToSelection(const int32 ShapeIndex, const int32 FirstVertexIndex)
{
	FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited();
	if (ensure((Geometry != nullptr) && Geometry->Shapes.IsValidIndex(ShapeIndex)))
	{
		FSpriteGeometryShape& Shape = Geometry->Shapes[ShapeIndex];
		const int32 NextVertexIndex = (FirstVertexIndex + 1) % Shape.Vertices.Num();

		AddPolygonVertexToSelection(ShapeIndex, FirstVertexIndex);
		AddPolygonVertexToSelection(ShapeIndex, NextVertexIndex);
	}
}

void FSpriteEditorViewportClient::ClearSelectionSet()
{
	SelectedItemSet.Empty();
	SelectedIDSet.Empty();
}

static bool ClosestPointOnLine(const FVector2D& Point, const FVector2D& LineStart, const FVector2D& LineEnd, FVector2D& OutClosestPoint)
{
	FVector2D DL = LineEnd - LineStart;
	FVector2D DP = Point - LineStart;
	float DotL = FVector2D::DotProduct(DP, DL);
	float DotP = FVector2D::DotProduct(DL, DL);
	if (DotP > 0.0001f)
	{
		float T = DotL / DotP;
		if (T >= -0.0001f && T <= 1.0001f)
		{
			T = FMath::Clamp(T, 0.0f, 1.0f);
			OutClosestPoint = LineStart + T * DL;
			return true;
		}
	}

	return false;
}

// Adds a point to the currently edited index
// If SelectedPolygonIndex is set, only that polygon is considered
void FSpriteEditorViewportClient::AddPointToGeometry(const FVector2D& TextureSpacePoint, const int32 SelectedPolygonIndex)
{
	FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited();

	int32 ClosestShapeIndex = INDEX_NONE;
	int32 ClosestVertexInsertIndex = INDEX_NONE;
	float ClosestDistanceSquared = MAX_FLT;

	int32 StartPolygonIndex = 0;
	int32 EndPolygonIndex = Geometry->Shapes.Num();
	if ((SelectedPolygonIndex >= 0) && (SelectedPolygonIndex < Geometry->Shapes.Num()))
	{
		StartPolygonIndex = SelectedPolygonIndex;
		EndPolygonIndex = SelectedPolygonIndex + 1;
	}

	// Determine where we should insert the vertex
	for (int32 PolygonIndex = StartPolygonIndex; PolygonIndex < EndPolygonIndex; ++PolygonIndex)
	{
		FSpriteGeometryShape& Polygon = Geometry->Shapes[PolygonIndex];
		if (Polygon.Vertices.Num() >= 3)
		{
			for (int32 VertexIndex = 0; VertexIndex < Polygon.Vertices.Num(); ++VertexIndex)
			{
				FVector2D ClosestPoint;
				const FVector2D LineStart = Polygon.ConvertShapeSpaceToTextureSpace(Polygon.Vertices[VertexIndex]);
				int32 NextVertexIndex = (VertexIndex + 1) % Polygon.Vertices.Num();
				const FVector2D LineEnd = Polygon.ConvertShapeSpaceToTextureSpace(Polygon.Vertices[NextVertexIndex]);
				if (ClosestPointOnLine(TextureSpacePoint, LineStart, LineEnd, /*out*/ ClosestPoint))
				{
					const float CurrentDistanceSquared = FVector2D::DistSquared(ClosestPoint, TextureSpacePoint);
					if (CurrentDistanceSquared < ClosestDistanceSquared)
					{
						ClosestShapeIndex = PolygonIndex;
						ClosestDistanceSquared = CurrentDistanceSquared;
						ClosestVertexInsertIndex = NextVertexIndex;
					}
				}
			}
		}
		else
		{
			// Simply insert the vertex
			for (int32 VertexIndex = 0; VertexIndex < Polygon.Vertices.Num(); ++VertexIndex)
			{
				const FVector2D CurrentVertexTS = Polygon.ConvertShapeSpaceToTextureSpace(Polygon.Vertices[VertexIndex]);
				const float CurrentDistanceSquared = FVector2D::DistSquared(CurrentVertexTS, TextureSpacePoint);
				if (CurrentDistanceSquared < ClosestDistanceSquared)
				{
					ClosestShapeIndex = PolygonIndex;
					ClosestDistanceSquared = CurrentDistanceSquared;
					ClosestVertexInsertIndex = VertexIndex + 1;
				}
			}
		}
	}

	if ((ClosestVertexInsertIndex != INDEX_NONE) && (ClosestShapeIndex != INDEX_NONE))
	{
		FSpriteGeometryShape& Shape = Geometry->Shapes[ClosestShapeIndex];
		if (Shape.ShapeType != ESpriteShapeType::Circle)
		{
			BeginTransaction(LOCTEXT("AddPolygonVertexTransaction", "Add Vertex to Polygon"));
			Shape.Vertices.Insert(Shape.ConvertTextureSpaceToShapeSpace(TextureSpacePoint), ClosestVertexInsertIndex);
			Shape.ShapeType = ESpriteShapeType::Polygon;
			Geometry->GeometryType = ESpritePolygonMode::FullyCustom;
			bManipulationDirtiedSomething = true;
			EndTransaction();

			// Select this vertex
			ClearSelectionSet();
			AddPolygonVertexToSelection(ClosestShapeIndex, ClosestVertexInsertIndex);
		}
	}
}

void FSpriteEditorViewportClient::ResetMarqueeTracking()
{
	bIsMarqueeTracking = false;
}

bool FSpriteEditorViewportClient::ConvertMarqueeToSourceTextureSpace(/*out*/FVector2D& OutStartPos, /*out*/FVector2D& OutDimension)
{
	bool bSuccessful = false;
	UPaperSprite* Sprite = SourceTextureViewComponent->GetSprite();
	UTexture2D* SpriteSourceTexture = Sprite->GetSourceTexture();
	if (SpriteSourceTexture != nullptr)
	{
		// Calculate world space positions
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, GetScene(), EngineShowFlags));
		FSceneView* View = CalcSceneView(&ViewFamily);
		const FVector StartPos = View->PixelToWorld(MarqueeStartPos.X, MarqueeStartPos.Y, 0);
		const FVector EndPos = View->PixelToWorld(MarqueeEndPos.X, MarqueeEndPos.Y, 0);

		// Convert to source texture space to work out the pixels dragged
		FVector2D TextureSpaceStartPos = Sprite->ConvertWorldSpaceToTextureSpace(StartPos);
		FVector2D TextureSpaceEndPos = Sprite->ConvertWorldSpaceToTextureSpace(EndPos);

		if (TextureSpaceStartPos.X > TextureSpaceEndPos.X)
		{
			Swap(TextureSpaceStartPos.X, TextureSpaceEndPos.X);
		}
		if (TextureSpaceStartPos.Y > TextureSpaceEndPos.Y)
		{
			Swap(TextureSpaceStartPos.Y, TextureSpaceEndPos.Y);
		}

		const int32 SourceTextureWidth = SpriteSourceTexture->GetSizeX();
		const int32 SourceTextureHeight = SpriteSourceTexture->GetSizeY();
		TextureSpaceStartPos.X = FMath::Clamp((int)TextureSpaceStartPos.X, 0, SourceTextureWidth - 1);
		TextureSpaceStartPos.Y = FMath::Clamp((int)TextureSpaceStartPos.Y, 0, SourceTextureHeight - 1);
		TextureSpaceEndPos.X = FMath::Clamp((int)TextureSpaceEndPos.X, 0, SourceTextureWidth - 1);
		TextureSpaceEndPos.Y = FMath::Clamp((int)TextureSpaceEndPos.Y, 0, SourceTextureHeight - 1);

		const FVector2D TextureSpaceDimensions = TextureSpaceEndPos - TextureSpaceStartPos;
		if (TextureSpaceDimensions.X > 0 || TextureSpaceDimensions.Y > 0)
		{
			OutStartPos = TextureSpaceStartPos;
			OutDimension = TextureSpaceDimensions;
			bSuccessful = true;
		}
	}

	return bSuccessful;
}

void FSpriteEditorViewportClient::SelectVerticesInMarquee(bool bAddToSelection)
{
	if (!bAddToSelection)
	{
		ClearSelectionSet();
	}

	if (UPaperSprite* Sprite = GetSpriteBeingEdited())
	{
		// Calculate world space positions
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, GetScene(), EngineShowFlags));
		FSceneView* View = CalcSceneView(&ViewFamily);
		const FVector StartPos = View->PixelToWorld(MarqueeStartPos.X, MarqueeStartPos.Y, 0);
		const FVector EndPos = View->PixelToWorld(MarqueeEndPos.X, MarqueeEndPos.Y, 0);

		// Convert to source texture space to work out the pixels dragged
		FVector2D TextureSpaceStartPos = Sprite->ConvertWorldSpaceToTextureSpace(StartPos);
		FVector2D TextureSpaceEndPos = Sprite->ConvertWorldSpaceToTextureSpace(EndPos);

		if (TextureSpaceStartPos.X > TextureSpaceEndPos.X)
		{
			Swap(TextureSpaceStartPos.X, TextureSpaceEndPos.X);
		}
		if (TextureSpaceStartPos.Y > TextureSpaceEndPos.Y)
		{
			Swap(TextureSpaceStartPos.Y, TextureSpaceEndPos.Y);
		}

		const FBox2D QueryBounds(TextureSpaceStartPos, TextureSpaceEndPos);

		if (FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited())
		{
			for (int32 ShapeIndex = 0; ShapeIndex < Geometry->Shapes.Num(); ++ShapeIndex)
			{
				const FSpriteGeometryShape& Shape = Geometry->Shapes[ShapeIndex];

				bool bSelectWholeShape = false;

				if ((Shape.ShapeType == ESpriteShapeType::Circle) || (Shape.ShapeType == ESpriteShapeType::Box))
				{
					// First see if we are fully contained
					const FBox2D ShapeBoxBounds(Shape.BoxPosition - Shape.BoxSize * 0.5f, Shape.BoxPosition + Shape.BoxSize * 0.5f);
					if (QueryBounds.IsInside(ShapeBoxBounds))
					{
						bSelectWholeShape = true;
					}
				}

				//@TODO: Try intersecting with the circle if it wasn't entirely enclosed
				
				if (bSelectWholeShape)
				{
					SelectShape(ShapeIndex);
				}
				else
				{
					// Try to select some subset of the vertices
					for (int32 VertexIndex = 0; VertexIndex < Shape.Vertices.Num(); ++VertexIndex)
					{
						const FVector2D TextureSpaceVertex = Shape.ConvertShapeSpaceToTextureSpace(Shape.Vertices[VertexIndex]);
						if (QueryBounds.IsInside(TextureSpaceVertex))
						{
							AddPolygonVertexToSelection(ShapeIndex, VertexIndex);
						}
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE