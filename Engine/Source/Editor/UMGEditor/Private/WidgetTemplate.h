// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "IUMGEditor.h"
#include "BlueprintEditor.h"

class FWidgetTemplate : public TSharedFromThis<FWidgetTemplate>
{
public:
	FWidgetTemplate();

	virtual FText GetCategory() = 0;

	virtual USlateWrapperComponent* Create(UWidgetTree* Tree) = 0;

public:
	FText Name;

	FText ToolTip;

	FSlateIcon Icon;
};
