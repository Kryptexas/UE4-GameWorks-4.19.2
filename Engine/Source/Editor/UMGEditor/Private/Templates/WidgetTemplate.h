// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FWidgetTemplate : public TSharedFromThis<FWidgetTemplate>
{
public:
	FWidgetTemplate();

	virtual FText GetCategory() = 0;

	virtual UWidget* Create(class UWidgetTree* Tree) = 0;

public:
	FText Name;

	FText ToolTip;

	FSlateIcon Icon;
};
