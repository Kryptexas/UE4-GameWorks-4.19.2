// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DesignerExtension.h"

/**
 * The canvas slot extension provides design time widgets for widgets that are selected in the canvas.
 */
class FCanvasSlotExtension : public FDesignerExtension
{
public:
	FCanvasSlotExtension();

	virtual ~FCanvasSlotExtension() {}

	virtual bool CanExtendSelection(const TArray< FWidgetReference >& Selection) const override;
	
	virtual void ExtendSelection(const TArray< FWidgetReference >& Selection, TArray< TSharedRef<FDesignerSurfaceElement> >& SurfaceElements) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);
	virtual void Paint(const TSet< FWidgetReference >& Selection, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:

	FReply HandleBeginDrag(const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleEndDrag(const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleDragging(const FGeometry& Geometry, const FPointerEvent& Event);

	void MoveByAmount(FWidgetReference& WidgetRef, FVector2D Delta);

	static bool GetCollisionSegmentsForSlot(UCanvasPanel* Canvas, int32 SlotIndex, TArray<FVector2D>& Segments);
	static bool GetCollisionSegmentsForSlot(UCanvasPanel* Canvas, UCanvasPanelSlot* Slot, TArray<FVector2D>& Segments);
	static void GetCollisionSegmentsFromGeometry(FGeometry ArrangedGeometry, TArray<FVector2D>& Segments);

	void PaintCollisionLines(const TSet< FWidgetReference >& Selection, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;

private:

	bool bDragging;

	TSharedPtr<SBorder> MoveHandle;
};