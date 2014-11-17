// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * The widget template represents a widget or a set of widgets to create and spawn into the widget tree.
 * More complicated defaults could be created by deriving from this class and registering new templates with the module.
 */
class FWidgetTemplate : public TSharedFromThis<FWidgetTemplate>
{
public:
	/** Constructor */
	FWidgetTemplate();

	/** The category this template fits into. */
	virtual FText GetCategory() = 0;

	/** Constructs the widget template. */
	virtual UWidget* Create(class UWidgetTree* Tree) = 0;

public:
	/** The name of the widget template. */
	FText Name;

	/** A tooltip to display for the template. */
	FText ToolTip;

	/** An icon to display for the template. */
	FSlateIcon Icon;
};
