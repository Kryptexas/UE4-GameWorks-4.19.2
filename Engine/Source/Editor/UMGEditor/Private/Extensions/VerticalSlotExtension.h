// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DesignerExtension.h"

class FVerticalSlotExtension : public FDesignerExtension
{
public:
	FVerticalSlotExtension();

	bool IsActive(const TArray< FWidgetReference >& Selection);
	
	virtual void BuildWidgetsForSelection(const TArray< FWidgetReference >& Selection, TArray< TSharedRef<SWidget> >& Widgets) override;

private:

	bool CanShift(int32 ShiftAmount) const;

	FReply HandleShiftVertical(int32 ShiftAmount);

	void ShiftVertical(UWidget* Widget, int32 ShiftAmount);
};