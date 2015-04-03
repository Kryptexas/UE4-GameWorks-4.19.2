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

	const FLinearColor MarqueeColor(1.0f, 1.0f, 1.0f, 0.5f);

	const FLinearColor SourceRegionBoundsColor(1.0f, 1.0f, 1.0f, 0.8f);
	// Alpha is being ignored in FBatchedElements::AddLine :(
	const FLinearColor SourceRegionRelatedBoundsColor(0.3f, 0.3f, 0.3f, 0.2f);

	const FLinearColor CollisionShapeColor(0.0f, 0.7f, 1.0f, 1.0f);
	const FLinearColor RenderShapeColor(1.0f, 0.2f, 0.0f, 1.0f);
	const FLinearColor SubtractiveRenderShapeColor(0.0f, 0.2f, 1.0f, 1.0f);
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
	bShowPivot = true;
	bShowRelatedSprites = true;

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

			RequestFocusOnSelection(/*bInstant=*/ true);
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

		// Draw the edge
		if (bIsHitTesting)
		{
			TSharedPtr<FSpriteSelectedSourceRegion> Data = MakeShareable(new FSpriteSelectedSourceRegion());
			Data->SpritePtr = Sprite;
			Data->VertexIndex = 4 + VertexIndex;
			Canvas.SetHitProxy(new HSpriteSelectableObjectHitProxy(Data));
		}

		FCanvasLineItem LineItem(BoundsVertices[VertexIndex], BoundsVertices[NextVertexIndex]);
		LineItem.SetColor(GeometryVertexColor);
		Canvas.DrawItem(LineItem);

        // Add edge hit proxy
        if (bDrawEdgeHitProxies)
        {
			const FVector2D MidPoint = (BoundsVertices[VertexIndex] + BoundsVertices[NextVertexIndex]) * 0.5f;
			Canvas.DrawTile(MidPoint.X - EdgeCollisionVertexSize*0.5f, MidPoint.Y - EdgeCollisionVertexSize*0.5f, EdgeCollisionVertexSize, EdgeCollisionVertexSize, 0.f, 0.f, 1.f, 1.f, GeometryVertexColor, GWhiteTexture);
        }

		if (bIsHitTesting)
		{
			Canvas.SetHitProxy(nullptr);
		}

        // Add corner hit proxy
        if (bDrawCornerHitProxies)
        {
			const FVector2D CornerPoint = BoundsVertices[VertexIndex];
			
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

void FSpriteEditorViewportClient::AnalyzeSpriteMaterialType(UPaperSprite* Sprite, int32& OutNumOpaque, int32& OutNumMasked, int32& OutNumTranslucent)
{
	struct Local
	{
		static void AttributeTrianglesByMaterialType(int32 NumTriangles, UMaterialInterface* Material, int32& NumOpaqueTriangles, int32& NumMaskedTriangles, int32& NumTranslucentTriangles)
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
	};

	OutNumOpaque = 0;
	OutNumMasked = 0;
	OutNumTranslucent = 0;

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

	Local::AttributeTrianglesByMaterialType(DefaultTriangles, Sprite->GetDefaultMaterial(), /*inout*/ OutNumOpaque, /*inout*/ OutNumMasked, /*inout*/ OutNumTranslucent);
	Local::AttributeTrianglesByMaterialType(AlternateTriangles, Sprite->GetAlternateMaterial(), /*inout*/ OutNumOpaque, /*inout*/ OutNumMasked, /*inout*/ OutNumTranslucent);
}

void FSpriteEditorViewportClient::DrawRenderStats(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, class UPaperSprite* Sprite, int32& YPos)
{
	FCanvasTextItem TextItem(FVector2D(6, YPos), LOCTEXT("RenderGeomBaked", "Render Geometry (baked)"), GEngine->GetSmallFont(), FLinearColor::White);
	TextItem.EnableShadow(FLinearColor::Black);

	TextItem.Draw(&Canvas);
	TextItem.Position += FVector2D(6.0f, 18.0f);

	int32 NumOpaqueTriangles = 0;
	int32 NumMaskedTriangles = 0;
	int32 NumTranslucentTriangles = 0;
	AnalyzeSpriteMaterialType(Sprite, /*out*/ NumOpaqueTriangles, /*out*/ NumMaskedTriangles, /*out*/ NumTranslucentTriangles);

	int32 NumSections = (Sprite->AlternateMaterialSplitIndex != INDEX_NONE) ? 2 : 1;
	if (NumSections > 1)
	{
		TextItem.Text = FText::Format(LOCTEXT("SectionCount", "Sections: {0}"), FText::AsNumber(NumSections));
		TextItem.Draw(&Canvas);
		TextItem.Position.Y += 18.0f;
	}

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

	static const FText SourceRegionHelpStr = LOCTEXT("SourceRegionHelp", "Drag handles to adjust source region\nDouble-click on an image region to select all connected pixels\nHold down Ctrl and drag a rectangle to create a new sprite at that position\nDouble-click to create a new sprite under the cursor\nClick on other sprite rectangles to change the active sprite");

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
			// Draw the custom collision geometry
			SpriteGeometryHelper.DrawGeometry_CanvasPass(Viewport, View, Canvas, /*inout*/ YPos, SpriteEditingConstants::CollisionShapeColor, FLinearColor::White);
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
			// Draw the custom render geometry
			SpriteGeometryHelper.DrawGeometry_CanvasPass(Viewport, View, Canvas, /*inout*/ YPos, SpriteEditingConstants::RenderShapeColor, SpriteEditingConstants::SubtractiveRenderShapeColor);
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

	switch (CurrentMode)
	{
	case ESpriteEditorMode::EditCollisionMode:
		// Draw the custom collision geometry
		SpriteGeometryHelper.DrawGeometry(*View, *PDI, SpriteEditingConstants::CollisionShapeColor, FLinearColor::White);
		break;
	case ESpriteEditorMode::EditRenderingGeomMode:
		// Draw the custom render geometry
		SpriteGeometryHelper.DrawGeometry(*View, *PDI, SpriteEditingConstants::RenderShapeColor, SpriteEditingConstants::SubtractiveRenderShapeColor);
		break;
	default:
		break;
	}
}

void FSpriteEditorViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	// Skipping the parent on purpose
	FEditorViewportClient::Draw(Viewport, Canvas);
}

FBox FSpriteEditorViewportClient::GetDesiredFocusBounds() const
{
	UPaperSpriteComponent* ComponentToFocusOn = SourceTextureViewComponent->IsVisible() ? SourceTextureViewComponent : RenderSpriteComponent;
	return ComponentToFocusOn->Bounds.GetBox();
}

void FSpriteEditorViewportClient::Tick(float DeltaSeconds)
{
	if (UPaperSprite* Sprite = GetSpriteBeingEdited())
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
			RequestFocusOnSelection(/*bInstant=*/ true);
			SourceTextureViewComponent->SetVisibility(bSourceTextureViewComponentVisibility);
		}

		bool bRenderTextureViewComponentVisibility = !IsInSourceRegionEditMode();
		if (bRenderTextureViewComponentVisibility != RenderSpriteComponent->IsVisible())
		{
			RequestFocusOnSelection(/*bInstant=*/ true);
			RenderSpriteComponent->SetVisibility(bRenderTextureViewComponentVisibility);
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

void FSpriteEditorViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	const FViewportClick Click(&View, this, Key, Event, HitX, HitY);
	const bool bIsCtrlKeyDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	const bool bIsShiftKeyDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);
	const bool bIsAltKeyDown = Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);
	bool bHandled = false;

	const bool bAllowSelectVertex = !(IsEditingGeometry() && SpriteGeometryHelper.IsAddingPolygon()) && !bIsShiftKeyDown;

	const bool bClearSelectionModifier = bIsCtrlKeyDown;
	const bool bDeleteClickedVertex = bIsAltKeyDown;
	const bool bInsertVertexModifier = bIsShiftKeyDown;
	HSpriteSelectableObjectHitProxy* SelectedItemProxy = HitProxyCast<HSpriteSelectableObjectHitProxy>(HitProxy);

	if (bAllowSelectVertex && (SelectedItemProxy != nullptr))
	{
		if (!bClearSelectionModifier)
		{
			SpriteGeometryHelper.ClearSelectionSet();
		}

		if (bDeleteClickedVertex)
		{
			// Delete selection
			if (const FSpriteSelectedVertex* SelectedVertex = SelectedItemProxy->Data->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex))
			{
				SpriteGeometryHelper.ClearSelectionSet();
				SpriteGeometryHelper.AddPolygonVertexToSelection(SelectedVertex->ShapeIndex, SelectedVertex->VertexIndex);
				DeleteSelection();
			}
			else if (const FSpriteSelectedShape* SelectedShape = SelectedItemProxy->Data->CastTo<const FSpriteSelectedShape>(FSelectionTypes::GeometryShape))
			{
				SpriteGeometryHelper.ClearSelectionSet();
				SpriteGeometryHelper.AddShapeToSelection(SelectedShape->ShapeIndex);
				DeleteSelection();
			}
		}
		else if (Event == EInputEvent::IE_DoubleClick)
		{
			// Double click to select a polygon
			if (const FSpriteSelectedVertex* SelectedVertex = SelectedItemProxy->Data->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex))
			{
				SpriteGeometryHelper.ClearSelectionSet();
				SpriteGeometryHelper.AddShapeToSelection(SelectedVertex->ShapeIndex);
			}
		}
		else
		{
			//@TODO: This needs to be generalized!
			if (const FSpriteSelectedEdge* SelectedEdge = SelectedItemProxy->Data->CastTo<const FSpriteSelectedEdge>(FSelectionTypes::Edge))
			{
				// Add the next vertex defined by this edge
				SpriteGeometryHelper.AddPolygonEdgeToSelection(SelectedEdge->ShapeIndex, SelectedEdge->VertexIndex);
			}
			else if (const FSpriteSelectedVertex* SelectedVertex = SelectedItemProxy->Data->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex))
			{
				SpriteGeometryHelper.AddPolygonVertexToSelection(SelectedVertex->ShapeIndex, SelectedVertex->VertexIndex);
			}
			else if (const FSpriteSelectedShape* SelectedShape = SelectedItemProxy->Data->CastTo<const FSpriteSelectedShape>(FSelectionTypes::GeometryShape))
			{
				SpriteGeometryHelper.AddShapeToSelection(SelectedShape->ShapeIndex);
			}
			else
			{
				SpriteGeometryHelper.SelectItem(SelectedItemProxy->Data);
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
			SpriteGeometryHelper.ClearSelectionSet();

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
		else if (IsEditingGeometry() && !SpriteGeometryHelper.IsAddingPolygon())
		{
			FSpriteGeometryCollection* Geometry = GetGeometryBeingEdited();

			if (bInsertVertexModifier)
			{
				FVector4 WorldPoint = View.PixelToWorld(HitX, HitY, 0);
				if (UPaperSprite* Sprite = GetSpriteBeingEdited())
				{
					FVector2D SpriteSpaceClickPoint = Sprite->ConvertWorldSpaceToTextureSpace(WorldPoint);

					// find a polygon to add vert to
					bool bFoundShapeToAddTo = false;
					for (TSharedPtr<FSelectedItem> SelectedItemPtr : SpriteGeometryHelper.GetSelectionSet())
					{
						if (const FSpriteSelectedVertex* SelectedVertex = SelectedItemPtr->CastTo<const FSpriteSelectedVertex>(FSelectionTypes::Vertex)) //@TODO:  Inflexible?
						{
							SpriteGeometryHelper.AddPointToGeometry(SpriteSpaceClickPoint, SelectedVertex->ShapeIndex);
							bFoundShapeToAddTo = true;
							break;
						}
					}

					if (!bFoundShapeToAddTo)
					{
						SpriteGeometryHelper.AddPointToGeometry(SpriteSpaceClickPoint);
					}
					bHandled = true;
				}
			}
		}
		else if (!IsEditingGeometry())
		{
			// Clicked on the background (missed any proxies), deselect the socket or whatever was selected
			SpriteGeometryHelper.ClearSelectionSet();
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
		if (SpriteGeometryHelper.IsAddingPolygon())
		{
			if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
			{
				const int32 HitX = Viewport->GetMouseX();
				const int32 HitY = Viewport->GetMouseY();

				// Calculate world space positions
				FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, GetScene(), EngineShowFlags));
				FSceneView* View = CalcSceneView(&ViewFamily);
				FVector WorldPoint = View->PixelToWorld(HitX, HitY, 0);

				UPaperSprite* Sprite = GetSpriteBeingEdited();
				FVector2D TexturePoint = Sprite->ConvertWorldSpaceToTextureSpace(WorldPoint);

				const bool bMakeSubtractiveIfAllowed = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
				SpriteGeometryHelper.TEMP_HandleAddPolygonClick(TexturePoint, bMakeSubtractiveIfAllowed);
			}
			else if (Key == EKeys::Enter)
			{
				SpriteGeometryHelper.ResetAddPolygonMode();
			}
			else if (Key == EKeys::Escape)
			{
				SpriteGeometryHelper.AbandonAddPolygonMode();
			}
		}
		else
		{
			if (ProcessMarquee(Viewport, ControllerId, Key, Event, true))
			{
				const bool bAddingToSelection = InputState.IsShiftButtonPressed(); //@TODO: control button moves widget? Hopefully make this more consistent when that is changed
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
		for (TSharedPtr<FSelectedItem> SelectedItem : SpriteGeometryHelper.GetSelectionSet())
		{
			SelectedItem->ApplyDelta(Drag2D, Rot, Scale, MoveMode);
		}

		if (SpriteGeometryHelper.HasAnySelectedItems())
		{
			if (!IsInSourceRegionEditMode())
			{
				UPaperSprite* Sprite = GetSpriteBeingEdited();
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
	return SpriteGeometryHelper.HasAnySelectedItems() ? DesiredWidgetMode : FWidget::WM_None;
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
		return SpriteGeometryHelper.HasAnySelectedItems();
	}
	return false;
}

FVector FSpriteEditorViewportClient::GetWidgetLocation() const
{
	FVector SummedPos(ForceInitToZero);

	if (SpriteGeometryHelper.HasAnySelectedItems())
	{
		// Find the center of the selection set
		for (TSharedPtr<FSelectedItem> SelectedItem : SpriteGeometryHelper.GetSelectionSet())
		{
			SummedPos += SelectedItem->GetWorldPos();
		}
		return (SummedPos / SpriteGeometryHelper.GetSelectionSet().Num());
	}

	return SummedPos;
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
	UPaperSprite* Sprite = GetSpriteBeingEdited();
	return Sprite->ConvertTextureSpaceToWorldSpace(SourcePoint);
}

bool FSpriteEditorViewportClient::SelectedItemIsSelected(const struct FShapeVertexPair& Item) const
{
	return SpriteGeometryHelper.IsGeometrySelected(Item);
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

void FSpriteEditorViewportClient::MarkTransactionAsDirty()
{
	bManipulationDirtiedSomething = true;
}

void FSpriteEditorViewportClient::EndTransaction()
{
	if (bManipulationDirtiedSomething)
	{
		UPaperSprite* Sprite = GetSpriteBeingEdited();

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

void FSpriteEditorViewportClient::InvalidateViewportAndHitProxies()
{
	Viewport->Invalidate();
}

void FSpriteEditorViewportClient::NotifySpriteBeingEditedHasChanged()
{
	//@TODO: Ideally we do this before switching
	EndTransaction();
	SpriteGeometryHelper.ClearSelectionSet();

	// Update components to know about the new sprite being edited
	UPaperSprite* Sprite = GetSpriteBeingEdited();

	RenderSpriteComponent->SetSprite(Sprite);
	UpdateSourceTextureSpriteFromSprite(Sprite);

	InternalActivateNewMode(CurrentMode);

	//@TODO: Only do this if the sprite isn't visible (may consider doing a flashing pulse around the source region rect?)
	RequestFocusOnSelection(/*bInstant=*/ true);
}

void FSpriteEditorViewportClient::FocusOnSprite()
{
	FocusViewportOnBox(RenderSpriteComponent->Bounds.GetBox());
}

void FSpriteEditorViewportClient::DeleteSelection()
{
	SpriteGeometryHelper.DeleteSelectedItems();
}

void FSpriteEditorViewportClient::AddBoxShape()
{
	UPaperSprite* Sprite = GetSpriteBeingEdited();
	check(Sprite);

	const FVector2D BoxSize(Sprite->GetSourceSize());
	const FVector2D BoxLocation(Sprite->GetSourceUV() + (BoxSize * 0.5f));

	SpriteGeometryHelper.AddNewBoxShape(BoxLocation, BoxSize);
}

void FSpriteEditorViewportClient::AddCircleShape()
{
	check(IsInCollisionEditMode());

	UPaperSprite* Sprite = GetSpriteBeingEdited();
	check(Sprite);

	const FVector2D SpriteBounds = Sprite->GetSourceSize();
	const float SmallerBoundingAxisSize = SpriteBounds.GetMin();
	const float CircleRadius = SmallerBoundingAxisSize * 0.5f;
	const FVector2D EllipseLocation = Sprite->GetSourceUV() + (SpriteBounds * 0.5f);

	SpriteGeometryHelper.AddNewCircleShape(EllipseLocation, CircleRadius);
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
	ResetMarqueeTracking();

	UPaperSprite* Sprite = GetSpriteBeingEdited();

	// Note: This has side effects (clearing the selection set, ensuring the geometry is correct if the sprite being edited changed, etc...).
	// Do not skip even if the mode is not really changing.
	SpriteGeometryHelper.SetGeometryBeingEdited(nullptr, /*bAllowCircles=*/ false, /*bAllowSubtractivePolygons=*/ false);

	switch (CurrentMode)
	{
	case ESpriteEditorMode::ViewMode:
		break;
	case ESpriteEditorMode::EditSourceRegionMode:
		UpdateRelatedSpritesList();
		break;
	case ESpriteEditorMode::EditCollisionMode:
		if (Sprite != nullptr)
		{
			SpriteGeometryHelper.SetGeometryBeingEdited(&(Sprite->CollisionGeometry), /*bAllowCircles=*/ true, /*bAllowSubtractivePolygons=*/ false);
		}
		break;
	case ESpriteEditorMode::EditRenderingGeomMode:
		if (Sprite != nullptr)
		{
			SpriteGeometryHelper.SetGeometryBeingEdited(&(Sprite->RenderGeometry), /*bAllowCircles=*/ false, /*bAllowSubtractivePolygons=*/ true);
		}
		break;
	}
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

bool FSpriteEditorViewportClient::IsShapeSelected(const int32 ShapeIndex) const
{
	return SpriteGeometryHelper.IsGeometrySelected(FShapeVertexPair(ShapeIndex, INDEX_NONE));
}

bool FSpriteEditorViewportClient::IsPolygonVertexSelected(const int32 ShapeIndex, const int32 VertexIndex) const
{
	return SpriteGeometryHelper.IsGeometrySelected(FShapeVertexPair(ShapeIndex, VertexIndex));
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
		SpriteGeometryHelper.ClearSelectionSet();
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
					SpriteGeometryHelper.AddShapeToSelection(ShapeIndex);
				}
				else
				{
					// Try to select some subset of the vertices
					for (int32 VertexIndex = 0; VertexIndex < Shape.Vertices.Num(); ++VertexIndex)
					{
						const FVector2D TextureSpaceVertex = Shape.ConvertShapeSpaceToTextureSpace(Shape.Vertices[VertexIndex]);
						if (QueryBounds.IsInside(TextureSpaceVertex))
						{
							SpriteGeometryHelper.AddPolygonVertexToSelection(ShapeIndex, VertexIndex);
						}
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE