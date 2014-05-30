// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DesignerExtension.h"

class FHorizontalSlotExtension : public FDesignerExtension
{
public:
	FHorizontalSlotExtension();

	bool IsActive(const TArray< FSelectedWidget >& Selection);
	
	virtual void BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets) OVERRIDE;

private:

	FReply HandleShift(int32 ShiftAmount);
	
	void ShiftHorizontal(UWidget* Widget, int32 ShiftAmount);

	TArray< FSelectedWidget > SelectionCache;
};