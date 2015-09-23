// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class UStaticMesh;
class FSlateInstanceBufferUpdate;

/**
 * A widget that draws vertexes provided by a 2.5D StaticMesh.
 * The Mesh's material is used.
 * Hardware instancing is supported.
 */
class UMG_API SMeshWidget : public SLeafWidget
{

public:
	SLATE_BEGIN_ARGS(SMeshWidget)
		: _StaticMeshSource(nullptr)
	{}
		/** The StaticMesh asset that should be drawn. */
		SLATE_ARGUMENT(UStaticMesh*, StaticMeshSource)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	/** Draw the InStaticMesh when this widget paints */
	void SetMesh(const UStaticMesh& InStaticMesh);

	/** Begin an update to the per instance buffer. Enables hardware instancing. */
	TSharedPtr<FSlateInstanceBufferUpdate> BeginPerInstanceBufferUpdate(int32 InitialSize);

protected:
	// BEGIN SLeafWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	// END SLeafWidget interface

private:
	/** Populate OutSlateVerts and OutIndexes with data from this static mesh such that Slate can render it. */
	static void StaticMeshToSlateRenderData(const UStaticMesh& DataSource, TArray<FSlateVertex>& OutSlateVerts, TArray<SlateIndex>& OutIndexes);

	/** Holds a copy of the Static Mesh's data converted to a format that Slate understands. */
	TArray<FSlateVertex> VertexData;
	/** Connectivity data: Order in which the vertexes occur to make up a series of triangles. */
	TArray<SlateIndex> IndexData;
	/** Holds on to the material that is found on the StaticMesh. */
	TSharedPtr<FSlateBrush> Brush;
	/** Per instance data that can be passed to */
	TSharedPtr<ISlateUpdatableInstanceBuffer> PerInstanceBuffer;
};