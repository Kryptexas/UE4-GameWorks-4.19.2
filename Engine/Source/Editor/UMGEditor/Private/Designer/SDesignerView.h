// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "SNodePanel.h"
#include "SDesignSurface.h"
#include "DesignerExtension.h"
#include "IUMGDesigner.h"

namespace EDesignerMessage
{
	enum Type
	{
		None,
		MoveFromParent,
	};
}

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
	virtual float GetPreviewScale() const override;
	virtual FWidgetReference GetSelectedWidget() const override;
	virtual ETransformMode::Type GetTransformMode() const override;
	// End of IUMGDesigner interface

private:
	/** Establishes the resolution and aspect ratio to use on construction from config settings */
	void SetStartupResolution();

	/** The width of the preview screen for the UI */
	FOptionalSize GetPreviewWidth() const;

	/** The height of the preview screen for the UI */
	FOptionalSize GetPreviewHeight() const;

	/** Gets the DPI scale that would be applied given the current preview width and height */
	float GetPreviewDPIScale() const;

	virtual FSlateRect ComputeAreaBounds() const override;

	/** Adds any pending selected widgets to the selection set */
	void ResolvePendingSelectedWidgets();

	/** Updates the designer to display the latest preview widget */
	void UpdatePreviewWidget();

	void ClearExtensionWidgets();
	void CreateExtensionWidgetsForSelection();

	EVisibility GetInfoBarVisibility() const;
	FText GetInfoBarText() const;

	/** Displays the context menu when you right click */
	void ShowContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	void OnEditorSelectionChanged();

	/** @returns Gets the widget under the cursor based on a mouse pointer event. */
	FWidgetReference GetWidgetAtCursor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FArrangedWidget& ArrangedWidget);

	/** Gets the blueprint being edited by the designer */
	UWidgetBlueprint* GetBlueprint() const;

	/** Called whenever the blueprint is structurally changed. */
	void OnBlueprintChanged(UBlueprint* InBlueprint);

        /** Called whenever the blueprint is recompiled */
	void OnBlueprintCompiled(UBlueprint* InBlueprint);

	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

	void CacheSelectedWidgetGeometry();

	/** @return Formatted text for the given resolution params */
	FText GetResolutionText(int32 Width, int32 Height, const FString& AspectRatio) const;

	FText GetCurrentResolutionText() const;
	FSlateColor GetResolutionTextColorAndOpacity() const;
	
	// Handles selecting a common screen resolution.
	void HandleOnCommonResolutionSelected(int32 Width, int32 Height, FString AspectRatio);
	bool HandleIsCommonResolutionSelected(int32 Width, int32 Height) const;
	void AddScreenResolutionSection(FMenuBuilder& MenuBuilder, const TArray<FPlayScreenResolution>& Resolutions, const FText& SectionName);
	TSharedRef<SWidget> GetAspectMenu();

	void BeginTransaction(const FText& SessionName);
	void EndTransaction();

private:
	FReply HandleZoomToFitClicked();

private:
	static const FString ConfigSectionName;
	static const uint32 DefaultResolutionWidth;
	static const uint32 DefaultResolutionHeight;
	static const FString DefaultAspectRatio;

	/** Extensions for the designer to allow for custom widgets to be inserted onto the design surface as selection changes. */
	TArray< TSharedRef<FDesignerExtension> > DesignerExtensions;

private:
	void BindCommands();

	void SetTransformMode(ETransformMode::Type InTransformMode);
	bool CanSetTransformMode(ETransformMode::Type InTransformMode) const;
	bool IsTransformModeActive(ETransformMode::Type InTransformMode) const;

	UWidget* ProcessDropAndAddWidget(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, bool bIsPreview);

	FVector2D GetExtensionPosition(TSharedRef<FDesignerSurfaceElement> ExtensionElement) const;

	FVector2D GetExtensionSize(TSharedRef<FDesignerSurfaceElement> ExtensionElement) const;
	
private:
	/** A reference to the BP Editor that owns this designer */
	TWeakPtr<FWidgetBlueprintEditor> BlueprintEditor;

	/** The designer command list */
	TSharedPtr<FUICommandList> CommandList;

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

	/**
	 * Holds onto a temporary widget that the user may be getting ready to select, or may just 
	 * be the widget that got hit on the initial mouse down before moving the parent.
	 */
	FWidgetReference PendingSelectedWidget;

	/**  */
	bool bMouseDown;

	/**  */
	FVector2D ScreenMouseDownLocation;

	/** An existing widget is being moved in its current container, or in to a new container. */
	bool bMovingExistingWidget;

	/** The wall clock time the user has been hovering over a single widget */
	float HoverTime;

	/** The current widget being hovered */
	FWidgetReference HoveredWidget;

	/** The current slate widget being hovered, this is refreshed every frame in case it changes */
	TWeakPtr<SWidget> HoveredSlateWidget;

	/** The configured Width of the preview area, simulates screen size. */
	int32 PreviewWidth;

	/** The configured Height of the preview area, simulates screen size. */
	int32 PreviewHeight;

	// Resolution Info
	FString PreviewAspectRatio;

	/** Curve to handle fading of the resolution */
	FCurveSequence ResolutionTextFade;

	/**  */
	FWeakWidgetPath SelectedWidgetPath;

	/** */
	EDesignerMessage::Type DesignerMessage;

	/** */
	ETransformMode::Type TransformMode;
};
