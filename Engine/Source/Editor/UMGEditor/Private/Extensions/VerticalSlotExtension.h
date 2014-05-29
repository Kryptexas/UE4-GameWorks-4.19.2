// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DesignerExtension.h"

class FVerticalSlotExtension : public FDesignerExtension
{
public:
	FVerticalSlotExtension();

	bool IsActive(const TArray< UWidget* >& Selection);
	
	virtual void BuildWidgetsForSelection(const TArray< UWidget* >& Selection, TArray< TSharedRef<SWidget> >& Widgets) OVERRIDE;

private:

	FReply HandleUpPressed();
	FReply HandleDownPressed();

	TArray< UWidget* > SelectionCache;
};