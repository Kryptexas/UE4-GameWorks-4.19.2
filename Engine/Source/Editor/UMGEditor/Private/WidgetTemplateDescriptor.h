// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "IUMGEditor.h"
#include "BlueprintEditor.h"

struct FWidgetTemplateDescriptor
{
	FText Name;

	FText ToolTip;

	FSlateIcon Icon;

	TSubclassOf<USlateWrapperComponent> WidgetClass;

	FWidgetTemplateDescriptor()
	{
	}
};
