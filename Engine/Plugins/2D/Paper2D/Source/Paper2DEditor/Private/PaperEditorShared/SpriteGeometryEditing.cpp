// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SpriteGeometryEditing.h"
#include "SpriteEditor/SpriteEditorSelections.h"
#include "PaperGeomTools.h"
#include "DynamicMeshBuilder.h"

//@TODO: Hackery
namespace SpriteEditingConstantsEX
{
	// The length and color to draw a normal tick in
// 	const FLinearColor GeometryNormalColor(0.0f, 1.0f, 0.0f, 0.5f);
// 	const float GeometryNormalLength = 15.0f;

	// Geometry edit rendering stuff
// 	const float GeometryVertexSize = 8.0f;
// 	const float GeometryBorderLineThickness = 2.0f;
	const int32 CircleShapeNumSides = 64;
	const FLinearColor GeometrySelectedColor(FLinearColor::White);
}

//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedShape

FSpriteSelectedShape::FSpriteSelectedShape(ISpriteSelectionContext& InEditorContext, FSpriteGeometryCollection& InGeometry, int32 InShapeIndex, bool bInIsBackground)
	: FSelectedItem(FSelectionTypes::GeometryShape)
	, EditorContext(InEditorContext)
	, Geometry(InGeometry)
	, ShapeIndex(InShapeIndex)
	, bIsBackground(bInIsBackground)
{
}

// bool FSpriteSelectedShape::IsValidInEditor(UPaperSprite* Sprite, bool bInRenderData) const
// {
// 	if ((Sprite != nullptr) && (&Geometry == (bInRenderData ? &(Sprite->RenderGeometry) : &(Sprite->CollisionGeometry))))
// 	{
// 		return Geometry.Shapes.IsValidIndex(ShapeIndex);
// 	}
// 	return false;
// }

uint32 FSpriteSelectedShape::GetTypeHash() const
{
	return (ShapeIndex * 311);
}

EMouseCursor::Type FSpriteSelectedShape::GetMouseCursor() const
{
	return EMouseCursor::GrabHand;
}

bool FSpriteSelectedShape::Equals(const FSelectedItem& OtherItem) const
{
	if (OtherItem.IsA(FSelectionTypes::Vertex))
	{
		const FSpriteSelectedShape& V1 = *this;
		const FSpriteSelectedShape& V2 = *(FSpriteSelectedShape*)(&OtherItem);

		return (V1.ShapeIndex == V2.ShapeIndex) && (&(V1.Geometry) == &(V2.Geometry));
	}
	else
	{
		return false;
	}
}

bool FSpriteSelectedShape::IsBackgroundObject() const
{
	return bIsBackground;
}

void FSpriteSelectedShape::ApplyDelta(const FVector2D& Delta, const FRotator& Rotation, const FVector& Scale3D, FWidget::EWidgetMode MoveMode)
{
	if (Geometry.Shapes.IsValidIndex(ShapeIndex))
	{
		FSpriteGeometryShape& Shape = Geometry.Shapes[ShapeIndex];

		const bool bDoRotation = (MoveMode == FWidget::WM_Rotate) || (MoveMode == FWidget::WM_TranslateRotateZ);
		const bool bDoTranslation = (MoveMode == FWidget::WM_Translate) || (MoveMode == FWidget::WM_TranslateRotateZ);
		const bool bDoScale = MoveMode == FWidget::WM_Scale;

		if (bDoTranslation)
		{
			const FVector WorldSpaceDelta = (PaperAxisX * Delta.X) + (PaperAxisY * Delta.Y);
			const FVector2D TextureSpaceDelta = EditorContext.SelectedItemConvertWorldSpaceDeltaToLocalSpace(WorldSpaceDelta);

			Shape.BoxPosition += TextureSpaceDelta;

			Geometry.GeometryType = ESpritePolygonMode::FullyCustom;
		}

		if (bDoScale)
		{
			const float ScaleDeltaX = FVector::DotProduct(Scale3D, PaperAxisX);
			const float ScaleDeltaY = FVector::DotProduct(Scale3D, PaperAxisY);

			const FVector2D OldSize = Shape.BoxSize;
			const FVector2D NewSize(OldSize.X + ScaleDeltaX, OldSize.Y + ScaleDeltaY);

			if (!FMath::IsNearlyZero(NewSize.X, KINDA_SMALL_NUMBER) && !FMath::IsNearlyZero(NewSize.Y, KINDA_SMALL_NUMBER))
			{
				const FVector2D ScaleFactor(NewSize.X / OldSize.X, NewSize.Y / OldSize.Y);
				Shape.BoxSize = NewSize;

				// Now apply it to the verts
				for (FVector2D& Vertex : Shape.Vertices)
				{
					Vertex.X *= ScaleFactor.X;
					Vertex.Y *= ScaleFactor.Y;
				}

				Geometry.GeometryType = ESpritePolygonMode::FullyCustom;
			}
		}

		if (bDoRotation)
		{
			//@TODO: This stuff should probably be wrapped up into a utility method (also used for socket editing)
			const FRotator CurrentRot(Shape.Rotation, 0.0f, 0.0f);
			FRotator SocketWinding;
			FRotator SocketRotRemainder;
			CurrentRot.GetWindingAndRemainder(SocketWinding, SocketRotRemainder);

			const FQuat ActorQ = SocketRotRemainder.Quaternion();
			const FQuat DeltaQ = Rotation.Quaternion();
			const FQuat ResultQ = DeltaQ * ActorQ;
			const FRotator NewSocketRotRem = FRotator(ResultQ);
			FRotator DeltaRot = NewSocketRotRem - SocketRotRemainder;
			DeltaRot.Normalize();

			const FRotator NewRotation(CurrentRot + DeltaRot);

			Shape.Rotation = NewRotation.Pitch;
			Geometry.GeometryType = ESpritePolygonMode::FullyCustom;
		}
	}
}

FVector FSpriteSelectedShape::GetWorldPos() const
{
	FVector Result = FVector::ZeroVector;

	if (Geometry.Shapes.IsValidIndex(ShapeIndex))
	{
		const FSpriteGeometryShape& Shape = Geometry.Shapes[ShapeIndex];

		switch (Shape.ShapeType)
		{
		case ESpriteShapeType::Box:
		case ESpriteShapeType::Circle:
			Result = EditorContext.TextureSpaceToWorldSpace(Shape.BoxPosition);
			break;
		case ESpriteShapeType::Polygon:
			// Average the vertex positions
			//@TODO: Eventually this will just be BoxPosition as well once the vertex positions are relative
			Result = EditorContext.TextureSpaceToWorldSpace(Shape.GetPolygonCentroid());
			break;
		default:
			check(false);
		}
	}

	return Result;
}

void FSpriteSelectedShape::SplitEdge()
{
	// Nonsense operation on a whole shape, do nothing
}

//////////////////////////////////////////////////////////////////////////
// FSpriteGeometryEditingHelper

FSpriteGeometryEditingHelper::FSpriteGeometryEditingHelper(ISpriteSelectionContext& InEditorContext)
	: EditorContext(InEditorContext)
{
	WidgetVertexColorMaterial = (UMaterial*)StaticLoadObject(UMaterial::StaticClass(), NULL, TEXT("/Engine/EditorMaterials/WidgetVertexColorMaterial.WidgetVertexColorMaterial"), NULL, LOAD_None, NULL);
}

void FSpriteGeometryEditingHelper::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(WidgetVertexColorMaterial);
}

void FSpriteGeometryEditingHelper::DrawGeometry(const FSceneView& View, FPrimitiveDrawInterface& PDI, FSpriteGeometryCollection& Geometry, const FLinearColor& GeometryVertexColor, const FLinearColor& NegativeGeometryVertexColor, bool bIsRenderGeometry)
{
	const bool bIsHitTesting = PDI.IsHitTesting();
	const float UnitsPerPixel = EditorContext.SelectedItemGetUnitsPerPixel();

	// Run thru the geometry shapes and draw hit proxies for them
	for (int32 ShapeIndex = 0; ShapeIndex < Geometry.Shapes.Num(); ++ShapeIndex)
	{
		const FSpriteGeometryShape& Shape = Geometry.Shapes[ShapeIndex];

		const bool bIsShapeSelected = EditorContext.SelectedItemIsSelected(FShapeVertexPair(ShapeIndex, INDEX_NONE));
		const FLinearColor LineColorRaw = Shape.bNegativeWinding ? NegativeGeometryVertexColor : GeometryVertexColor;
		const FLinearColor VertexColor = Shape.bNegativeWinding ? NegativeGeometryVertexColor : GeometryVertexColor;

		const FLinearColor LineColor = Shape.IsShapeValid() ? LineColorRaw : FMath::Lerp(LineColorRaw, FLinearColor::Red, 0.8f);

		// Draw the interior (allowing selection of the whole shape)
		if (bIsHitTesting)
		{
			TSharedPtr<FSpriteSelectedShape> Data = MakeShareable(new FSpriteSelectedShape(EditorContext, Geometry, ShapeIndex, /*bIsBackground=*/ true));
			PDI.SetHitProxy(new HSpriteSelectableObjectHitProxy(Data));
		}

		FColor BackgroundColor(bIsShapeSelected ? SpriteEditingConstantsEX::GeometrySelectedColor : LineColor);
		BackgroundColor.A = 4;
		FMaterialRenderProxy* ShapeMaterialProxy = WidgetVertexColorMaterial->GetRenderProxy(bIsShapeSelected);

		if (Shape.ShapeType == ESpriteShapeType::Circle)
		{
			//@TODO: This is going to have issues if we ever support ellipses
			const FVector2D PixelSpaceRadius = Shape.BoxSize * 0.5f;
			const float WorldSpaceRadius = PixelSpaceRadius.X * UnitsPerPixel;

			const FVector CircleCenterWorldPos = EditorContext.TextureSpaceToWorldSpace(Shape.BoxPosition);

			DrawDisc(&PDI, CircleCenterWorldPos, PaperAxisX, PaperAxisY, BackgroundColor, WorldSpaceRadius, SpriteEditingConstantsEX::CircleShapeNumSides, ShapeMaterialProxy, SDPG_Foreground);
		}
		else
		{
			TArray<FVector2D> SourceTextureSpaceVertices;
			Shape.GetTextureSpaceVertices(/*out*/ SourceTextureSpaceVertices);

			TArray<FVector2D> TriangulatedPolygonVertices;
			PaperGeomTools::TriangulatePoly(/*out*/ TriangulatedPolygonVertices, SourceTextureSpaceVertices, /*bKeepColinearVertices=*/ true);

			if (((TriangulatedPolygonVertices.Num() % 3) == 0) && (TriangulatedPolygonVertices.Num() > 0))
			{
				FDynamicMeshBuilder MeshBuilder;

				FDynamicMeshVertex MeshVertex;
				MeshVertex.Color = BackgroundColor;
				MeshVertex.TextureCoordinate = FVector2D::ZeroVector;
				MeshVertex.SetTangents(PaperAxisX, PaperAxisY, PaperAxisZ);

				for (const FVector2D& SrcTriangleVertex : TriangulatedPolygonVertices)
				{
					MeshVertex.Position = EditorContext.TextureSpaceToWorldSpace(SrcTriangleVertex);
					MeshBuilder.AddVertex(MeshVertex);
				}

				for (int32 TriangleIndex = 0; TriangleIndex < TriangulatedPolygonVertices.Num(); TriangleIndex += 3)
				{
					MeshBuilder.AddTriangle(TriangleIndex, TriangleIndex + 1, TriangleIndex + 2);
				}

				MeshBuilder.Draw(&PDI, FMatrix::Identity, ShapeMaterialProxy, SDPG_Foreground);
			}
		}

		if (bIsHitTesting)
		{
			PDI.SetHitProxy(nullptr);
		}
	}
}
