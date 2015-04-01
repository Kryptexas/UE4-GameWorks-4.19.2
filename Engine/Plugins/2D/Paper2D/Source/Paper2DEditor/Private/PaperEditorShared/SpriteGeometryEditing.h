// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FShapeVertexPair

struct FShapeVertexPair
{
	int32 ShapeIndex;

	int32 VertexIndex;

	FShapeVertexPair()
		: ShapeIndex(INDEX_NONE)
		, VertexIndex(INDEX_NONE)
	{
	}

	FShapeVertexPair(int32 InShapeIndex, int32 InVertexIndex)
		: ShapeIndex(InShapeIndex)
		, VertexIndex(InVertexIndex)
	{
	}
};

inline bool operator==(const FShapeVertexPair& A, const FShapeVertexPair& B)
{
	return (A.ShapeIndex == B.ShapeIndex) && (A.VertexIndex == B.VertexIndex);
}

inline uint32 GetTypeHash(const FShapeVertexPair& Item)
{
	return Item.VertexIndex + (Item.ShapeIndex * 311);
}

//////////////////////////////////////////////////////////////////////////
// FSpriteGeometryEditingHelper

class FSpriteGeometryEditingHelper : public FGCObject
{
public:
	FSpriteGeometryEditingHelper(class ISpriteSelectionContext& InEditorContext);

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FGCObject interface

	void DrawGeometry(const FSceneView& View, FPrimitiveDrawInterface& PDI, FSpriteGeometryCollection& Geometry, const FLinearColor& GeometryVertexColor, const FLinearColor& NegativeGeometryVertexColor, bool bIsRenderGeometry);

private:
	class ISpriteSelectionContext& EditorContext;
	UMaterial* WidgetVertexColorMaterial;
};
