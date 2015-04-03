// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Paper2DEditorPrivatePCH.h"
#include "SpriteEditor.h"
#include "SSingleObjectDetailsPanel.h"
#include "SceneViewport.h"
#include "PaperEditorViewportClient.h"
#include "PreviewScene.h"
#include "ScopedTransaction.h"
#include "SpriteEditorSelections.h"
#include "AssetData.h"
#include "PaperEditorShared/SpriteGeometryEditing.h"

//////////////////////////////////////////////////////////////////////////
// FRelatedSprite

struct FRelatedSprite
{
	FAssetData AssetData;
	FVector2D SourceUV;
	FVector2D SourceDimension;
};

//////////////////////////////////////////////////////////////////////////
// FSpriteEditorViewportClient

class FSpriteEditorViewportClient : public FPaperEditorViewportClient, public ISpriteSelectionContext
{
public:
	/** Constructor */
	FSpriteEditorViewportClient(TWeakPtr<FSpriteEditor> InSpriteEditor, TWeakPtr<class SSpriteEditorViewport> InSpriteEditorViewportPtr);

	// FViewportClient interface
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual void Tick(float DeltaSeconds) override;
	// End of FViewportClient interface

	// FEditorViewportClient interface
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad) override;
	virtual bool InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override;
	virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) override;
	virtual void TrackingStopped() override;
	virtual FWidget::EWidgetMode GetWidgetMode() const override;
	virtual void SetWidgetMode(FWidget::EWidgetMode NewMode) override;
	virtual bool CanSetWidgetMode(FWidget::EWidgetMode NewMode) const override;
	virtual bool CanCycleWidgetMode() const override;
	virtual FVector GetWidgetLocation() const override;
	virtual FMatrix GetWidgetCoordSystem() const override;
	virtual ECoordSystem GetWidgetCoordSystemSpace() const override;
	virtual FLinearColor GetBackgroundColor() const override;
	// End of FEditorViewportClient interface

	// ISpriteSelectionContext interface
	virtual FVector2D SelectedItemConvertWorldSpaceDeltaToLocalSpace(const FVector& WorldSpaceDelta) const override;
	virtual FVector TextureSpaceToWorldSpace(const FVector2D& SourcePoint) const override;
	virtual bool SelectedItemIsSelected(const struct FShapeVertexPair& Item) const override;
	virtual float SelectedItemGetUnitsPerPixel() const override;
	virtual void BeginTransaction(const FText& SessionName) override;
	virtual void MarkTransactionAsDirty() override;
	virtual void EndTransaction() override;
	virtual void InvalidateViewportAndHitProxies() override;
	// End of ISpriteSelectionContext interface

	// Process marquee selection, return true of a selection has been performed
	bool ProcessMarquee(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, bool bMarqueeStartModifierPressed);

	void ToggleShowSockets() { bShowSockets = !bShowSockets; Invalidate(); }
	bool IsShowSocketsChecked() const { return bShowSockets; }

	void ToggleShowPivot() { bShowPivot = !bShowPivot; Invalidate(); }
	bool IsShowPivotChecked() const { return bShowPivot; }

	void ToggleShowMeshEdges();
	bool IsShowMeshEdgesChecked() const;

	void EnterViewMode() { InternalActivateNewMode(ESpriteEditorMode::ViewMode); }
	void EnterSourceRegionEditMode() { InternalActivateNewMode(ESpriteEditorMode::EditSourceRegionMode); }
	void EnterCollisionEditMode() { InternalActivateNewMode(ESpriteEditorMode::EditCollisionMode); }
	void EnterRenderingEditMode() { InternalActivateNewMode(ESpriteEditorMode::EditRenderingGeomMode); }

	bool IsInViewMode() const { return CurrentMode == ESpriteEditorMode::ViewMode; }
	bool IsInSourceRegionEditMode() const { return CurrentMode == ESpriteEditorMode::EditSourceRegionMode; }
	bool IsInCollisionEditMode() const { return CurrentMode == ESpriteEditorMode::EditCollisionMode; }
	bool IsInRenderingEditMode() const { return CurrentMode == ESpriteEditorMode::EditRenderingGeomMode; }

	//
	bool IsEditingGeometry() const { return IsInCollisionEditMode() || IsInRenderingEditMode(); }

	void ToggleShowSourceTexture();
	bool IsShowSourceTextureChecked() const { return bShowSourceTexture; }
	bool CanShowSourceTexture() const { return !IsInSourceRegionEditMode(); }

	void FocusOnSprite();

	// Geometry editing commands
	void DeleteSelection();
	bool CanDeleteSelection() const { return IsEditingGeometry(); } //@TODO: Need a selection

	void AddBoxShape();
	void AddCircleShape();

	void SnapAllVerticesToPixelGrid();
	bool CanSnapVerticesToPixelGrid() const { return IsEditingGeometry(); }

	// Invalidate any references to the sprite being edited; it has changed
	void NotifySpriteBeingEditedHasChanged();

	void UpdateRelatedSpritesList();

	UPaperSprite* CreateNewSprite(FVector2D TopLeft, FVector2D Dimensions);

	ESpriteEditorMode::Type GetCurrentMode() const
	{
		return CurrentMode;
	}

	static void AnalyzeSpriteMaterialType(UPaperSprite* Sprite, int32& OutNumOpaque, int32& OutNumMasked, int32& OutNumTranslucent);

protected:
	// FPaperEditorViewportClient interface
	virtual FBox GetDesiredFocusBounds() const override;
	// End of FPaperEditorViewportClient interface

private:
	// Editor mode
	ESpriteEditorMode::Type CurrentMode;

	// The preview scene
	FPreviewScene OwnedPreviewScene;

	// Sprite editor that owns this viewport
	TWeakPtr<FSpriteEditor> SpriteEditorPtr;

	// Render component for the source texture view
	UPaperSpriteComponent* SourceTextureViewComponent;

	// Render component for the sprite being edited
	UPaperSpriteComponent* RenderSpriteComponent;

	// Widget mode
	FWidget::EWidgetMode DesiredWidgetMode;

	// Are we currently manipulating something?
	bool bManipulating;

	// Did we dirty something during manipulation?
	bool bManipulationDirtiedSomething;

	// Pointer back to the sprite editor viewport control that owns us
	TWeakPtr<class SSpriteEditorViewport> SpriteEditorViewportPtr;

	// The current transaction for undo/redo
	class FScopedTransaction* ScopedTransaction;

	// Should we show the source texture?
	bool bShowSourceTexture;

	// Should we show sockets?
	bool bShowSockets;

	// Should we show the sprite pivot?
	bool bShowPivot;

	// Should we show related sprites in the source texture?
	bool bShowRelatedSprites;

	// Marquee tracking
	bool bIsMarqueeTracking;
	FVector2D MarqueeStartPos, MarqueeEndPos;

	// Other sprites that share the same source texture
	TArray<FRelatedSprite> RelatedSprites;

	// Sprite geometry editing/rendering helper
	FSpriteGeometryEditingHelper SpriteGeometryHelper;

public:
	FSpriteGeometryEditingHelper* GetGeometryEditHelper()
	{
		return &SpriteGeometryHelper;
	}

private:
	UPaperSprite* GetSpriteBeingEdited() const
	{
		return SpriteEditorPtr.Pin()->GetSpriteBeingEdited();
	}

	// Position relative to source texture (ignoring rotation and other transformations applied to extract the sprite)
	FVector2D SourceTextureSpaceToScreenSpace(const FSceneView& View, const FVector2D& SourcePoint) const;
	FVector SourceTextureSpaceToWorldSpace(const FVector2D& SourcePoint) const;

	void DrawBoundsAsText(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, int32& YPos);

	static void DrawGeometryStats(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FSpriteGeometryCollection& Geometry, bool bIsRenderGeometry, int32& YPos);
	static void DrawCollisionStats(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, class UBodySetup* BodySetup, int32& YPos);
	void DrawRenderStats(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, class UPaperSprite* Sprite, int32& YPos);
	void DrawSourceRegion(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FLinearColor& GeometryVertexColor);
	void DrawRelatedSprites(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FLinearColor& GeometryVertexColor);
	void DrawMarquee(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, const FLinearColor& MarqueeColor);

	void UpdateSourceTextureSpriteFromSprite(UPaperSprite* SourceSprite);
	
	// Selection handling

	// Indicates if the specified shape is currently selected
	bool IsShapeSelected(const int32 ShapeIndex) const;

	// Indicates if the specified vertex is selected
	bool IsPolygonVertexSelected(const int32 PolygonIndex, const int32 VertexIndex) const;

	void ResetMarqueeTracking();
	bool ConvertMarqueeToSourceTextureSpace(/*out*/FVector2D& OutStartPos, /*out*/FVector2D& OutDimension);
	void SelectVerticesInMarquee(bool bAddToSelection);

	// Activates a new mode, clearing selection set, etc...
	void InternalActivateNewMode(ESpriteEditorMode::Type NewMode);

	// Can return null
	FSpriteGeometryCollection* GetGeometryBeingEdited() const;
};
