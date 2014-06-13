// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DesignerExtension.h"

class FCanvasSlotExtension : public FDesignerExtension
{
public:
	FCanvasSlotExtension();

	bool IsActive(const TArray< FSelectedWidget >& Selection);
	
	virtual void BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets) override;

private:

	FReply HandleBeginDrag(const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleEndDrag(const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleDragging(const FGeometry& Geometry, const FPointerEvent& Event);

	void MoveByAmount(UWidget* Widget, FVector2D Delta);

	TArray< FSelectedWidget > SelectionCache;
	bool bDragging;

	TSharedPtr<SBorder> MoveHandle;
};