// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "SNodePanel.h"
#include "SDesignSurface.h"
#include "DesignerExtension.h"
#include "IUMGDesigner.h"

class FDesignerExtension;

/**
 * The designer for widgets.  Allows for laying out widgets in a drag and drop environment.
 */
class SDesignerView : public SDesignSurface, public IUMGDesigner
{
public:

	SLATE_BEGIN_ARGS( SDesignerView ) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor);
	virtual ~SDesignerView();

	// SWidget interface
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) override;

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	//virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	// End of SWidget interface

	void Register(TSharedRef<FDesignerExtension> Extension);

	// IUMGDesigner interface
	float GetPreviewScale() const override;
	// End of IUMGDesigner interface

private:
	/** The width of the preview screen for the UI */
	FOptionalSize GetPreviewWidth() const;

	/** The height of the preview screen for the UI */
	FOptionalSize GetPreviewHeight() const;

	/** Gets the DPI scale that would be applied given the current preview width and height */
	float GetPreviewDPIScale() const;

	virtual FSlateRect ComputeAreaBounds() const override;

	/** Updates the designer to display the latest preview widget */
	void UpdatePreviewWidget();

	void CreateExtensionWidgetsForSelection();

	/** Displays the context menu when you right click */
	void ShowContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	void OnEditorSelectionChanged();

	/** @returns Gets the widget under the cursor based on a mouse pointer event. */
	FWidgetReference GetWidgetAtCursor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FArrangedWidget& ArrangedWidget);

	/** Gets the blueprint being edited by the designer */
	UWidgetBlueprint* GetBlueprint() const;

	/** Called whenever the blueprint is structurally changed. */
	void OnBlueprintChanged(UBlueprint* InBlueprint);
	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

	void CacheSelectedWidgetGeometry();

	// Handles selecting a common screen resolution.
	void HandleCommonResolutionSelected(int32 Width, int32 Height);
	void AddScreenResolutionSection(FMenuBuilder& MenuBuilder, const TArray<FPlayScreenResolution>& Resolutions, const FText& SectionName);
	TSharedRef<SWidget> GetAspectMenu();

	void BeginTransaction(const FText& SessionName);
	void EndTransaction();

private:
	FReply HandleZoomToFitClicked();

private:
	enum DragHandle
	{
		DH_NONE = -1,

		DH_TOP_LEFT = 0,
		DH_TOP_CENTER,
		DH_TOP_RIGHT,

		DH_MIDDLE_LEFT,
		DH_MIDDLE_RIGHT,

		DH_BOTTOM_LEFT,
		DH_BOTTOM_CENTER,
		DH_BOTTOM_RIGHT,
		
		DH_MAX,
	};

	TArray< FVector2D > DragDirections;

	/** Extensions for the designer to allow for custom widgets to be inserted onto the design surface as selection changes. */
	TArray< TSharedRef<FDesignerExtension> > DesignerExtensions;

private:
	void DrawDragHandles(const FPaintGeometry& SelectionGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle) const;
	DragHandle HitTestDragHandles(const FGeometry& AllottedGeometry, const FPointerEvent& PointerEvent) const;

	UWidget* ProcessDropAndAddWidget(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, bool bIsPreview);

private:

	FVector2D GetExtensionPosition(TSharedRef<FDesignerSurfaceElement> ExtensionElement) const;

	FVector2D GetExtensionSize(TSharedRef<FDesignerSurfaceElement> ExtensionElement) const;
	
private:
	/** A reference to the BP Editor that owns this designer */
	TWeakPtr<FWidgetBlueprintEditor> BlueprintEditor;

	/** The transaction used to commit undoable actions from resize, move...etc */
	FScopedTransaction* ScopedTransaction;

	/** The current preview widget */
	UUserWidget* PreviewWidget;

	/** The current preview widget's slate widget */
	TWeakPtr<SWidget> PreviewSlateWidget;
	
	UWidget* DropPreviewWidget;
	UPanelWidget* DropPreviewParent;

	TSharedPtr<class SZoomPan> PreviewHitTestRoot;
	TSharedPtr<SDPIScaler> PreviewSurface;
	TSharedPtr<SCanvas> ExtensionWidgetCanvas;

	DragHandle CurrentHandle;

	FVector2D CachedDesignerWidgetLocation;
	FVector2D CachedDesignerWidgetSize;

	/** The currently selected set of widgets */
	TSet< FWidgetReference > SelectedWidgets;

	/** TODO UMG Remove, after getting multiselection working. */
	FWidgetReference SelectedWidget;

	/** The location in selected widget local space where the context menu was summoned. */
	FVector2D SelectedWidgetContextMenuLocation;

	/** The currently selected slate widget, this is refreshed every frame in case it changes */
	TArray< TWeakPtr<SWidget> > SelectedSlateWidgets;
	TWeakPtr<SWidget> SelectedSlateWidget;

	/**  */
	bool bMouseDown;

	/** An existing widget is being moved in its current container, or in to a new container. */
	bool bMovingExistingWidget;

	/** The wall clock time the user has been hovering over a single widget */
	float HoverTime;

	/** The current widget being hovered */
	FWidgetReference HoveredWidget;

	/** The current slate widget being hovered, this is refreshed every frame in case it changes */
	TWeakPtr<SWidget> HoveredSlateWidget;

	int32 PreviewWidth;
	int32 PreviewHeight;
};
