// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "SNodePanel.h"
#include "SDesignSurface.h"
#include "DesignerExtension.h"

class FDesignerExtension;

class SUMGDesigner : public SDesignSurface
{
public:

	SLATE_BEGIN_ARGS( SUMGDesigner ) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor);
	virtual ~SUMGDesigner();

	// SWidget interface
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const OVERRIDE;

	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	// End of SWidget interface

	void Register(TSharedRef<FDesignerExtension> Extension);

private:
	FSelectedWidget GetWidgetAtCursor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FArrangedWidget& ArrangedWidget);

	UWidgetBlueprint* GetBlueprint() const;
	void OnBlueprintChanged(UBlueprint* InBlueprint);
	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
	void ShowDetailsForObjects(TArray<UWidget*> Widgets);

	bool GetArrangedWidget(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget) const;
	bool GetArrangedWidgetRelativeToWindow(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget) const;
	bool GetArrangedWidgetRelativeToDesigner(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget) const;

	FVector2D GetSelectionDesignerWidgetsLocation() const;

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

	/** Temporary widgets that are destroyed every time selection changes and are built from the current selection by extensions. */
	TArray< TSharedRef<SWidget> > ExtensionWidgets;

private:
	void DrawDragHandles(const FPaintGeometry& SelectionGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle) const;
	DragHandle HitTestDragHandles(const FGeometry& AllottedGeometry, const FPointerEvent& PointerEvent) const;

private:
	TWeakPtr<FWidgetBlueprintEditor> BlueprintEditor;

	TWeakPtr<SWidget> PreviewWidget;
	UUserWidget* PreviewWidgetActor;

	FSelectedWidget CurrentSelection;
	TWeakPtr<SWidget> SelectedWidget;

	TSharedPtr<SBorder> PreviewSurface;
	TSharedPtr<SCanvas> ExtensionWidgetCanvas;

	DragHandle CurrentHandle;
};
