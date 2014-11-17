// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DesignerExtension.h"

class FHorizontalSlotExtension : public FDesignerExtension
{
public:
	FHorizontalSlotExtension();

	bool IsActive(const TArray< FWidgetReference >& Selection);
	
	virtual void BuildWidgetsForSelection(const TArray< FWidgetReference >& Selection, TArray< TSharedRef<SWidget> >& Widgets) override;

private:

	FReply HandleShift(int32 ShiftAmount);
	
	void ShiftHorizontal(UWidget* Widget, int32 ShiftAmount);
};