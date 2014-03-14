// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Paper2DEditorPrivatePCH.h"
#include "SpriteEditor.h"
#include "SSingleObjectDetailsPanel.h"
#include "SceneViewport.h"
#include "PaperEditorViewportClient.h"
#include "PreviewScene.h"
#include "ScopedTransaction.h"
#include "SpriteEditorSelections.h"

//////////////////////////////////////////////////////////////////////////
// 

namespace ESpriteEditorMode
{
	enum Type
	{
		ViewMode,
		EditCollisionMode,
		EditRenderingGeomMode,
		AddSpriteMode
	};
}

//////////////////////////////////////////////////////////////////////////
// FSpriteEditorViewportClient

class FSpriteEditorViewportClient : public FPaperEditorViewportClient
{
public:
	/** Constructor */
	FSpriteEditorViewportClient(TWeakPtr<FSpriteEditor> InSpriteEditor, TWeakPtr<class SSpriteEditorViewport> InSpriteEditorViewportPtr);

	// FViewportClient interface
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) OVERRIDE;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI);
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) OVERRIDE;
	virtual void Tick(float DeltaSeconds) OVERRIDE;
	// End of FViewportClient interface

	// FEditorViewportClient interface
	virtual void UpdateMouseDelta() OVERRIDE;
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) OVERRIDE;
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad) OVERRIDE;
	virtual bool InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) OVERRIDE;
	virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) OVERRIDE;
	virtual void TrackingStopped() OVERRIDE;
	virtual FWidget::EWidgetMode GetWidgetMode() const OVERRIDE;
	virtual FVector GetWidgetLocation() const;
	virtual FMatrix GetWidgetCoordSystem() const;
	virtual ECoordSystem GetWidgetCoordSystemSpace() const;
	// End of FEditorViewportClient interface

	void ToggleShowSourceTexture();
	bool IsShowSourceTextureChecked() const { return bShowSourceTexture; }

	void ToggleShowSockets() { bShowSockets = !bShowSockets; Invalidate(); }
	bool IsShowSocketsChecked() const { return bShowSockets; }

	void ToggleShowPivot() { bShowPivot = !bShowPivot; Invalidate(); }
	bool IsShowPivotChecked() const { return bShowPivot; }

	void ToggleShowNormals() { bShowNormals = !bShowNormals; Invalidate(); }
	bool IsShowNormalsChecked() const { return bShowNormals; }

	void EnterViewMode() { CurrentMode = ESpriteEditorMode::ViewMode; }
	void EnterCollisionEditMode() { CurrentMode = ESpriteEditorMode::EditCollisionMode; }
	void EnterRenderingEditMode() { CurrentMode = ESpriteEditorMode::EditRenderingGeomMode; }
	void EnterAddSpriteMode() { CurrentMode = ESpriteEditorMode::AddSpriteMode; }

	bool IsInViewMode() const { return CurrentMode == ESpriteEditorMode::ViewMode; }
	bool IsInCollisionEditMode() const { return CurrentMode == ESpriteEditorMode::EditCollisionMode; }
	bool IsInRenderingEditMode() const { return CurrentMode == ESpriteEditorMode::EditRenderingGeomMode; }
	bool IsInAddSpriteMode() const { return CurrentMode == ESpriteEditorMode::AddSpriteMode; }

	//
	bool IsEditingGeometry() const { return IsInCollisionEditMode() || IsInRenderingEditMode(); }

	void FocusOnSprite();

	// Geometry editing commands
	void DeleteSelection();
	bool CanDeleteSelection() const { return IsEditingGeometry(); } //@TODO: Need a selection

	void SplitEdge();
	bool CanSplitEdge() const { return IsEditingGeometry(); } //@TODO: Need an edge

	void AddPolygon();
	bool CanAddPolygon() const { return IsEditingGeometry(); }

	void SnapAllVerticesToPixelGrid();
	bool CanSnapVerticesToPixelGrid() const { return IsEditingGeometry(); }

	// Invalidate any references to the sprite being edited; it has changed
	void NotifySpriteBeingEditedHasChanged();
private:
	// Editor mode
	ESpriteEditorMode::Type CurrentMode;

	// The preview scene
	FPreviewScene OwnedPreviewScene;

	// Sprite editor that owns this viewport
	TWeakPtr<FSpriteEditor> SpriteEditorPtr;

	// Render component for the source texture view
	UPaperRenderComponent* SourceTextureViewComponent;

	// Render component for the sprite being edited
	UPaperRenderComponent* RenderSpriteComponent;

	// Widget mode
	FWidget::EWidgetMode WidgetMode;

	// Are we currently manipulating something?
	bool bManipulating;

	// Did we dirty something during manipulation?
	bool bManipulationDirtiedSomething;

	// Pointer back to the sprite editor viewport control that owns us
	TWeakPtr<class SSpriteEditorViewport> SpriteEditorViewportPtr;

	// Selection set of vertices
	TSet< TSharedPtr<FSelectedItem> > SelectionSet;

	// The current transaction for undo/redo
	class FScopedTransaction* ScopedTransaction;

	// Should we show the source texture?
	bool bShowSourceTexture;

	// Should we show sockets?
	bool bShowSockets;

	// Should we show polygon normals?
	bool bShowNormals;

	// Should we show the sprite pivot?
	bool bShowPivot;

	// Should we zoom to the sprite next tick?
	bool bDeferZoomToSprite;
private:
	UPaperSprite* GetSpriteBeingEdited() const
	{
		return SpriteEditorPtr.Pin()->GetSpriteBeingEdited();
	}

	FVector2D TextureSpaceToScreenSpace(const FSceneView& View, const FVector2D& SourcePoint) const;
	FVector TextureSpaceToWorldSpace(const FVector2D& SourcePoint) const;

	void DrawTriangleList(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const TArray<FVector2D>& Triangles);
	void DrawGeometry(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FSpritePolygonCollection& Geometry, const FLinearColor& GeometryVertexColor, bool bIsRenderGeometry);
	void DrawGeometryStats(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FSpritePolygonCollection& Geometry, bool bIsRenderGeometry, int32& YPos);
	void DrawSockets(const FSceneView* View, FPrimitiveDrawInterface* PDI);
	void DrawSocketNames(FViewport& InViewport, FSceneView& View, FCanvas& Canvas);

	void BeginTransaction(const FText& SessionName);
	void EndTransaction();

	void UpdateSourceTextureSpriteFromSprite(UPaperSprite* SourceSprite);

	void ClearSelectionSet();

	
	// Can return null
	FSpritePolygonCollection* GetGeometryBeingEdited() const;
};
