// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Paper2DEditorPrivatePCH.h"
#include "TileMapEditor.h"
#include "SSingleObjectDetailsPanel.h"
#include "SceneViewport.h"
#include "PaperEditorViewportClient.h"
#include "PreviewScene.h"
#include "ScopedTransaction.h"

//////////////////////////////////////////////////////////////////////////
// FTileMapEditorViewportClient

class FTileMapEditorViewportClient : public FPaperEditorViewportClient
{
public:
	/** Constructor */
	FTileMapEditorViewportClient(TWeakPtr<FTileMapEditor> InTileMapEditor, TWeakPtr<class STileMapEditorViewport> InTileMapEditorViewportPtr);

	// FViewportClient interface
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI);
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual void Tick(float DeltaSeconds) override;
	// End of FViewportClient interface

	// FEditorViewportClient interface
	virtual void UpdateMouseDelta() override;
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad) override;
	virtual bool InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override;
	virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) override;
	virtual void TrackingStopped() override;
	virtual FWidget::EWidgetMode GetWidgetMode() const override;
	virtual FVector GetWidgetLocation() const;
	virtual FMatrix GetWidgetCoordSystem() const;
	virtual ECoordSystem GetWidgetCoordSystemSpace() const;
	// End of FEditorViewportClient interface

	void ToggleShowPivot() { bShowPivot = !bShowPivot; Invalidate(); }
	bool IsShowPivotChecked() const { return bShowPivot; }

	void ToggleShowMeshEdges();
	bool IsShowMeshEdgesChecked() const;

	//
	void FocusOnTileMap();

	// Invalidate any references to the tile map being edited; it has changed
	void NotifyTileMapBeingEditedHasChanged();
private:
	// The preview scene
	FPreviewScene OwnedPreviewScene;

	// Tile map editor that owns this viewport
	TWeakPtr<FTileMapEditor> TileMapEditorPtr;

	// Render component for the tile map being edited
	UPaperTileMapRenderComponent* RenderTileMapComponent;

	// Widget mode
	FWidget::EWidgetMode WidgetMode;

	// Are we currently manipulating something?
	bool bManipulating;

	// Did we dirty something during manipulation?
	bool bManipulationDirtiedSomething;

	// Pointer back to the tile map editor viewport control that owns us
	TWeakPtr<class STileMapEditorViewport> TileMapEditorViewportPtr;

	// The current transaction for undo/redo
	class FScopedTransaction* ScopedTransaction;

	// Should we show the sprite pivot?
	bool bShowPivot;

	// Should we zoom to the tile map next tick?
	bool bDeferZoomToTileMap;
private:
	UPaperTileMap* GetTileMapBeingEdited() const
	{
		return TileMapEditorPtr.Pin()->GetTileMapBeingEdited();
	}

	void DrawTriangleList(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const TArray<FVector2D>& Triangles);
	void DrawBoundsAsText(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, int32& YPos);
	
	void BeginTransaction(const FText& SessionName);
	void EndTransaction();

	void ClearSelectionSet();
};
