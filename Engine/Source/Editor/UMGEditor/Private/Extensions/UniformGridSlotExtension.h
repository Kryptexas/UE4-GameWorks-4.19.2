// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DesignerExtension.h"

class FUniformGridSlotExtension : public FDesignerExtension
{
public:
	FUniformGridSlotExtension();

	bool IsActive(const TArray< FSelectedWidget >& Selection);
	
	virtual void BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets) OVERRIDE;

private:

	FReply HandleShiftRow(int32 ShiftAmount);
	FReply HandleShiftColumn(int32 ShiftAmount);

	void ShiftRow(UWidget* Widget, int32 ShiftAmount);
	void ShiftColumn(UWidget* Widget, int32 ShiftAmount);

private:

	TArray< FSelectedWidget > SelectionCache;
};