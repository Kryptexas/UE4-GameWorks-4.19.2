// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperEditorShared/AssetEditorSelectedItem.h"

//////////////////////////////////////////////////////////////////////////
// FSelectionTypes

class FSelectionTypes
{
public:
	static const FName GeometryShape;
	static const FName Vertex;
	static const FName Edge;
	static const FName Pivot;
	static const FName SourceRegion;
private:
	FSelectionTypes() {}
};

//////////////////////////////////////////////////////////////////////////
// ISpriteSelectionContext

class ISpriteSelectionContext
{
public:
	virtual FVector2D SelectedItemConvertWorldSpaceDeltaToLocalSpace(const FVector& WorldSpaceDelta) const = 0;
	virtual FVector TextureSpaceToWorldSpace(const FVector2D& SourcePoint) const = 0;
	virtual bool SelectedItemIsSelected(const struct FShapeVertexPair& Item) const = 0;
	virtual float SelectedItemGetUnitsPerPixel() const = 0;
};

//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedSourceRegion

class FSpriteSelectedSourceRegion : public FSelectedItem
{
public:
	int32 VertexIndex;
	TWeakObjectPtr<UPaperSprite> SpritePtr;

public:
	FSpriteSelectedSourceRegion()
		: FSelectedItem(FSelectionTypes::SourceRegion)
		, VertexIndex(0)
	{
	}

	virtual uint32 GetTypeHash() const override
	{
		return VertexIndex;
	}

	virtual bool Equals(const FSelectedItem& OtherItem) const override
	{
		if (OtherItem.IsA(FSelectionTypes::SourceRegion))
		{
			const FSpriteSelectedSourceRegion& V1 = *this;
			const FSpriteSelectedSourceRegion& V2 = *(FSpriteSelectedSourceRegion*)(&OtherItem);

			return (V1.VertexIndex == V2.VertexIndex) && (V1.SpritePtr == V2.SpritePtr);
		}
		else
		{
			return false;
		}
	}

	virtual void ApplyDeltaIndexed(const FVector2D& WorldSpaceDelta, int32 TargetVertexIndex)
	{
		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			UTexture2D* SourceTexture = Sprite->GetSourceTexture();
			const FVector2D SourceDims = (SourceTexture != nullptr) ? FVector2D(SourceTexture->GetSurfaceWidth(), SourceTexture->GetSurfaceHeight()) : FVector2D::ZeroVector;

			int32 XAxis = 0; // -1 = min, 0 = none, 1 = max
            int32 YAxis = 0; // ditto
            
			switch (VertexIndex)
			{
            case 0: // Top left
                XAxis = -1;
                YAxis = -1;
                break;
            case 1: // Top right
                XAxis = 1;
                YAxis = -1;
                break;
            case 2: // Bottom right
                XAxis = 1;
                YAxis = 1;
                break;
            case 3: // Bottom left
                XAxis = -1;
                YAxis = 1;
                break;
            case 4: // Top
                YAxis = -1;
                break;
            case 5: // Right
                XAxis = 1;
                break;
            case 6: // Bottom
                YAxis = 1;
                break;
            case 7: // Left
                XAxis = -1;
                break;
			default:
				break;
			}

			const FVector2D TextureSpaceDelta = Sprite->ConvertWorldSpaceDeltaToTextureSpace(PaperAxisX * WorldSpaceDelta.X + PaperAxisY * WorldSpaceDelta.Y, /*bIgnoreRotation=*/ true);

            if (XAxis == -1)
            {
                const float AllowedDelta = FMath::Clamp(TextureSpaceDelta.X, -Sprite->SourceUV.X, Sprite->SourceDimension.X - 1);
                Sprite->SourceUV.X += AllowedDelta;
                Sprite->SourceDimension.X -= AllowedDelta;
            }
            else if (XAxis == 1)
            {
				Sprite->SourceDimension.X = FMath::Clamp(Sprite->SourceDimension.X + TextureSpaceDelta.X, 1.0f, SourceDims.X - Sprite->SourceUV.X);
            }
            
            if (YAxis == -1)
            {
				const float AllowedDelta = FMath::Clamp(TextureSpaceDelta.Y, -Sprite->SourceUV.Y, Sprite->SourceDimension.Y - 1);
				Sprite->SourceUV.Y += AllowedDelta;
				Sprite->SourceDimension.Y -= AllowedDelta;
            }
            else if (YAxis == 1)
            {
				Sprite->SourceDimension.Y = FMath::Clamp(Sprite->SourceDimension.Y + TextureSpaceDelta.Y, 1.0f, SourceDims.Y - Sprite->SourceUV.Y);
            }
		}
	}

	FVector GetWorldPosIndexed(int32 TargetVertexIndex) const
	{
		FVector Result = FVector::ZeroVector;

		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			UTexture2D* SourceTexture = Sprite->GetSourceTexture();
			const FVector2D SourceDims = (SourceTexture != nullptr) ? FVector2D(SourceTexture->GetSurfaceWidth(), SourceTexture->GetSurfaceHeight()) : FVector2D::ZeroVector;
			FVector2D BoundsVertex = Sprite->SourceUV;
			switch (VertexIndex)
			{
            case 0:
                break;
            case 1:
                BoundsVertex.X += Sprite->SourceDimension.X;
                break;
            case 2:
                BoundsVertex.X += Sprite->SourceDimension.X;
                BoundsVertex.Y += Sprite->SourceDimension.Y;
                break;
            case 3:
                BoundsVertex.Y += Sprite->SourceDimension.Y;
                break;
			case 4:
				BoundsVertex.X += Sprite->SourceDimension.X * 0.5f;
				break;
			case 5:
				BoundsVertex.X += Sprite->SourceDimension.X;
				BoundsVertex.Y += Sprite->SourceDimension.Y * 0.5f;
				break;
			case 6:
				BoundsVertex.X += Sprite->SourceDimension.X * 0.5f;
				BoundsVertex.Y += Sprite->SourceDimension.Y;
				break;
			case 7:
				BoundsVertex.Y += Sprite->SourceDimension.Y * 0.5f;
				break;
			default:
				break;
			}

			const FVector PixelSpaceResult = BoundsVertex.X * PaperAxisX + (SourceDims.Y - BoundsVertex.Y) * PaperAxisY;
			Result = PixelSpaceResult * Sprite->GetUnrealUnitsPerPixel();
		}

		return Result;
	}

	virtual void ApplyDelta(const FVector2D& Delta, const FRotator& Rotation, const FVector& Scale3D, FWidget::EWidgetMode MoveMode) override
	{
		if (MoveMode == FWidget::WM_Translate)
		{
			ApplyDeltaIndexed(Delta, VertexIndex);
		}
	}

	FVector GetWorldPos() const override
	{
		return GetWorldPosIndexed(VertexIndex);
	}

	virtual void SplitEdge() override
	{
		// Nonsense operation on a vertex, do nothing
	}
};


//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedShape

class FSpriteSelectedShape : public FSelectedItem
{
public:
	// The editor context
	ISpriteSelectionContext& EditorContext;

	// The geometry that this shape belongs to
	FSpriteGeometryCollection& Geometry;

	// The index of this shape in the geometry above
	int32 ShapeIndex;

	// Is this a background object that should have lower priority?
	bool bIsBackground;

	TWeakObjectPtr<UPaperSprite> SpritePtr;
public:
	FSpriteSelectedShape(ISpriteSelectionContext& InEditorContext, FSpriteGeometryCollection& InGeometry, int32 InShapeIndex, bool bInIsBackground = false);

	// FSelectedItem interface
//	virtual bool IsValidInEditor(UPaperSprite* Sprite, bool bInRenderData) const override;
	virtual uint32 GetTypeHash() const override;
	virtual EMouseCursor::Type GetMouseCursor() const override;
	virtual bool Equals(const FSelectedItem& OtherItem) const override;
	virtual bool IsBackgroundObject() const override;
	virtual void ApplyDelta(const FVector2D& Delta, const FRotator& Rotation, const FVector& Scale3D, FWidget::EWidgetMode MoveMode) override;
	virtual FVector GetWorldPos() const override;
	virtual void SplitEdge() override;
	// End of FSelectedItem interface
};

//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedVertex

class FSpriteSelectedVertex : public FSelectedItem
{
public:
	int32 VertexIndex;
	int32 ShapeIndex;

	// If true, it's render data, otherwise it's collision data
	bool bRenderData;

	TWeakObjectPtr<UPaperSprite> SpritePtr;
public:
	FSpriteSelectedVertex()
		: FSelectedItem(FSelectionTypes::Vertex)
		, VertexIndex(0)
		, ShapeIndex(0)
		, bRenderData(false)
	{
	}

	virtual bool IsValidInEditor(UPaperSprite* Sprite, bool bInRenderData) const
	{
		if (Sprite && SpritePtr == Sprite && bRenderData == bInRenderData)
		{
			FSpriteGeometryCollection& Geometry = bRenderData ? Sprite->RenderGeometry : Sprite->CollisionGeometry;
			if (Geometry.Shapes.IsValidIndex(ShapeIndex) && Geometry.Shapes[ShapeIndex].Vertices.IsValidIndex(VertexIndex))
			{
				return true;
			}
		}
		return false;
	}

	virtual uint32 GetTypeHash() const override
	{
		return VertexIndex + (ShapeIndex * 311) + (bRenderData ? 1063 : 0);
	}

	virtual bool Equals(const FSelectedItem& OtherItem) const override
	{
		if (OtherItem.IsA(FSelectionTypes::Vertex))
		{
			const FSpriteSelectedVertex& V1 = *this;
			const FSpriteSelectedVertex& V2 = *(FSpriteSelectedVertex*)(&OtherItem);

			return (V1.VertexIndex == V2.VertexIndex) && (V1.ShapeIndex == V2.ShapeIndex) && (V1.bRenderData == V2.bRenderData) && (V1.SpritePtr == V2.SpritePtr);
		}
		else
		{
			return false;
		}
	}

	virtual void ApplyDeltaIndexed(const FVector2D& Delta, int32 TargetVertexIndex)
	{
		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			FSpriteGeometryCollection& Geometry = bRenderData ? Sprite->RenderGeometry : Sprite->CollisionGeometry;
			if (Geometry.Shapes.IsValidIndex(ShapeIndex))
			{
				FSpriteGeometryShape& Shape = Geometry.Shapes[ShapeIndex];
				TArray<FVector2D>& Vertices = Shape.Vertices;
				TargetVertexIndex = (Vertices.Num() > 0) ? (TargetVertexIndex % Vertices.Num()) : 0;
				if (Vertices.IsValidIndex(TargetVertexIndex))
				{
					FVector2D& ShapeSpaceVertex = Vertices[TargetVertexIndex];

					const FVector WorldSpaceDelta = PaperAxisX * Delta.X + PaperAxisY * Delta.Y;
					const FVector2D TextureSpaceDelta = Sprite->ConvertWorldSpaceDeltaToTextureSpace(WorldSpaceDelta);
					const FVector2D NewTextureSpacePos = Shape.ConvertShapeSpaceToTextureSpace(ShapeSpaceVertex) + TextureSpaceDelta;
					ShapeSpaceVertex = Shape.ConvertTextureSpaceToShapeSpace(NewTextureSpacePos);

					Geometry.GeometryType = ESpritePolygonMode::FullyCustom;
					Shape.ShapeType = ESpriteShapeType::Polygon;
				}
			}
		}
	}

	FVector GetWorldPosIndexed(int32 TargetVertexIndex) const
	{
		FVector Result = FVector::ZeroVector;

		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			FSpriteGeometryCollection& Geometry = bRenderData ? Sprite->RenderGeometry : Sprite->CollisionGeometry;

			if (Geometry.Shapes.IsValidIndex(ShapeIndex))
			{
				const FSpriteGeometryShape& Shape = Geometry.Shapes[ShapeIndex];
				const TArray<FVector2D>& Vertices = Shape.Vertices;
				TargetVertexIndex = (Vertices.Num() > 0) ? (TargetVertexIndex % Vertices.Num()) : 0;
				if (Vertices.IsValidIndex(TargetVertexIndex))
				{
					const FVector2D& ShapeSpacePos = Vertices[TargetVertexIndex];
					const FVector2D TextureSpacePos = Shape.ConvertShapeSpaceToTextureSpace(ShapeSpacePos);
					Result = Sprite->ConvertTextureSpaceToWorldSpace(TextureSpacePos);
				}
			}
		}

		return Result;
	}

	virtual void ApplyDelta(const FVector2D& Delta, const FRotator& Rotation, const FVector& Scale3D, FWidget::EWidgetMode MoveMode) override
	{
		if (MoveMode == FWidget::WM_Translate)
		{
			ApplyDeltaIndexed(Delta, VertexIndex);
		}
	}

	FVector GetWorldPos() const override
	{
		return GetWorldPosIndexed(VertexIndex);
	}

	virtual void SplitEdge() override
	{
		// Nonsense operation on a vertex, do nothing
	}
};


//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedEdge
//@TODO: Much hackery lies here; need a real data structure!


class FSpriteSelectedEdge : public FSpriteSelectedVertex
{
	// Note: Defined based on a vertex index; this is the edge between the vertex and the next one.
public:
	FSpriteSelectedEdge()
	{
		TypeName = FSelectionTypes::Edge;
	}

	virtual bool IsA(FName TestType) const
	{
		return (TestType == TypeName) || (TestType == FSelectionTypes::Vertex);
	}

	virtual bool Equals(const FSelectedItem& OtherItem) const override
	{
		if (OtherItem.IsA(FSelectionTypes::Edge))
		{
			const FSpriteSelectedEdge& E1 = *this;
			const FSpriteSelectedEdge& E2 = *(FSpriteSelectedEdge*)(&OtherItem);

			return (E1.VertexIndex == E2.VertexIndex) && (E1.ShapeIndex == E2.ShapeIndex) && (E1.bRenderData == E2.bRenderData) && (E1.SpritePtr == E2.SpritePtr);
		}
		else
		{
			return false;
		}
	}

	virtual void ApplyDelta(const FVector2D& Delta, const FRotator& Rotation, const FVector& Scale3D, FWidget::EWidgetMode MoveMode) override
	{
		ApplyDeltaIndexed(Delta, VertexIndex);
		ApplyDeltaIndexed(Delta, VertexIndex+1);
	}

	FVector GetWorldPos() const override
	{
		const FVector Pos1 = GetWorldPosIndexed(VertexIndex);
		const FVector Pos2 = GetWorldPosIndexed(VertexIndex+1);

		return (Pos1 + Pos2) * 0.5f;
	}

	virtual void SplitEdge() override
	{
		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			FSpriteGeometryCollection& Geometry = bRenderData ? Sprite->RenderGeometry : Sprite->CollisionGeometry;

			if (Geometry.Shapes.IsValidIndex(ShapeIndex))
			{
				FSpriteGeometryShape& Shape = Geometry.Shapes[ShapeIndex];
				TArray<FVector2D>& Vertices = Shape.Vertices;
				if (Vertices.IsValidIndex(VertexIndex))
				{
					const int32 NextVertexIndex = (VertexIndex + 1) % Vertices.Num();

					const FVector2D NewShapeSpacePos = (Vertices[VertexIndex] + Vertices[NextVertexIndex]) * 0.5f;
					
					Vertices.Insert(NewShapeSpacePos, VertexIndex + 1);

					Geometry.GeometryType = ESpritePolygonMode::FullyCustom;
					Shape.ShapeType = ESpriteShapeType::Polygon;
				}
			}
		}
	}
};
