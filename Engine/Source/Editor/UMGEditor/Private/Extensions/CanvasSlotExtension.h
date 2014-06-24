// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DesignerExtension.h"

class FCanvasSlotExtension : public FDesignerExtension
{
public:
	FCanvasSlotExtension();

	bool IsActive(const TArray< FWidgetReference >& Selection);
	
	virtual void BuildWidgetsForSelection(const TArray< FWidgetReference >& Selection, TArray< TSharedRef<SWidget> >& Widgets) override;
	virtual void Paint(const TSet< FWidgetReference >& Selection, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:

	FReply HandleBeginDrag(const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleEndDrag(const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleDragging(const FGeometry& Geometry, const FPointerEvent& Event);

	void MoveByAmount(FWidgetReference& WidgetRef, FVector2D Delta);

	static bool GetCollisionSegmentsForSlot(UCanvasPanel* Canvas, int32 SlotIndex, TArray<FVector2D>& Segments);
	static bool GetCollisionSegmentsForSlot(UCanvasPanel* Canvas, UCanvasPanelSlot* Slot, TArray<FVector2D>& Segments);
	static void GetCollisionSegmentsFromGeometry(FGeometry ArrangedGeometry, TArray<FVector2D>& Segments);

	bool bDragging;

	TSharedPtr<SBorder> MoveHandle;
};