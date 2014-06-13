// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace ETileMapEditorTool
{
	enum Type
	{
		Paintbrush,
		Eraser,
		PaintBucket
	};
}

namespace ETileMapLayerPaintingMode
{
	enum Type
	{
		VisualLayers,
		CollisionLayers
	};
}

//////////////////////////////////////////////////////////////////////////
// FEdModeTileMap

class FEdModeTileMap : public FEdMode
{
public:
	static const FEditorModeID EM_TileMap;

public:
	FEdModeTileMap();
	virtual ~FEdModeTileMap();

	// FSerializableObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FSerializableObject

	// FEdMode interface
	virtual bool UsesToolkits() const override;
	virtual void Enter() override;
	virtual void Exit() override;
	//virtual bool BoxSelect(FBox& InBox, bool bInSelect) override;

	//	virtual void PostUndo() override;
	virtual bool MouseMove(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;
	virtual bool CapturedMouseMove(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	virtual bool StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	// 	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime) override;
	virtual bool InputKey(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;
	virtual bool InputDelta(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	// 	virtual bool Select(AActor* InActor, bool bInSelected) override;
	// 	virtual bool IsSelectionAllowed(AActor* InActor) const override;
	// 	virtual void ActorSelectionChangeNotify() override;
	// 	virtual FVector GetWidgetLocation() const override;
	virtual bool AllowWidgetMove();
	virtual bool ShouldDrawWidget() const override;
	virtual bool UsesTransformWidget() const override;
	virtual void PeekAtSelectionChangedEvent(UObject* ItemUndergoingChange) override;
	// 	virtual int32 GetWidgetAxisToDraw(FWidget::EWidgetMode InWidgetMode) const override;
	// 	virtual bool DisallowMouseDeltaTracking() const override;
	// End of FEdMode interface

	void SetActiveTool(ETileMapEditorTool::Type NewTool);
	ETileMapEditorTool::Type GetActiveTool() const;

	void SetActiveLayerPaintingMode(ETileMapLayerPaintingMode::Type NewMode);
	ETileMapLayerPaintingMode::Type GetActiveLayerPaintingMode() const;

	void SetActivePaint(UPaperTileSet* TileSet, FIntPoint TopLeft, FIntPoint Dimensions);

	void RefreshBrushSize();
protected:
	bool UseActiveToolAtLocation(const FViewportCursorLocation& Ray);

	bool PaintTiles(const FViewportCursorLocation& Ray);
	bool EraseTiles(const FViewportCursorLocation& Ray);
	bool FloodFillTiles(const FViewportCursorLocation& Ray);


	void UpdatePreviewCursor(const FViewportCursorLocation& Ray);

	// Returns the selected layer under the cursor, and the intersection tile coordinates
	// Note: The tile coordinates can be negative if the brush is off the top or left of the tile map, but still overlaps the map!!!
	UPaperTileLayer* GetSelectedLayerUnderCursor(const FViewportCursorLocation& Ray, int32& OutTileX, int32& OutTileY) const;

	// Compute a world space ray from the screen space mouse coordinates
	FViewportCursorLocation CalculateViewRay(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport);

	static AActor* GetFirstSelectedActorContainingTileMapComponent();

	void CreateModeButtonInModeTray(FToolBarBuilder& Builder);
	
	TSharedRef<FExtender> AddCreationModeExtender(const TSharedRef<FUICommandList> InCommandList);

	void EnableTileMapEditMode();
	bool IsTileMapEditModeActive() const;
protected:
	bool bIsPainting;
	TWeakObjectPtr<UPaperTileSet> PaintSourceTileSet;

	FIntPoint PaintSourceTopLeft;
	FIntPoint PaintSourceDimensions;

	FTransform DrawPreviewSpace;
	FVector DrawPreviewLocation;
	FVector DrawPreviewDimensionsLS;

	int32 EraseBrushSize;

	int32 CursorWidth;
	int32 CursorHeight;
	int32 BrushWidth;
	int32 BrushHeight;

	ETileMapEditorTool::Type ActiveTool;
	ETileMapLayerPaintingMode::Type LayerPaintingMode;
	mutable FTransform ComponentToWorld;
};

