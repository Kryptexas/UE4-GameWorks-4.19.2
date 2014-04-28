// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "SNodePanel.h"
#include "SDesignSurface.h"

class SUMGDesigner : public SDesignSurface
{
public:

	SLATE_BEGIN_ARGS( SUMGDesigner ) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class FBlueprintEditor> InBlueprintEditor);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;

	// SWidget interface
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	// End of SWidget interface

private:
	USlateWrapperComponent* GetTemplateAtCursor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FArrangedWidget& ArrangedWidget);

	UWidgetBlueprint* GetBlueprint() const;
	void OnBlueprintChanged(UBlueprint* InBlueprint);
	void OnObjectPropertyChanged(UObject* ObjectBeingModified);
	void ShowDetailsForObjects(TArray<USlateWrapperComponent*> Widgets);

	bool GetLocallyArrangedWidget(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget) const;

private:
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
	TWeakObjectPtr<AUserWidget> PreviewWidgetActor;
	TWeakPtr<SWidget> PreviewWidget;

	TWeakPtr<SWidget> SelectedWidget;

	TSharedPtr<SBorder> PreviewSurface;
};
