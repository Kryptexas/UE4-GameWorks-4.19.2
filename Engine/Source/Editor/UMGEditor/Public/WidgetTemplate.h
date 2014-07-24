// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * The widget template represents a widget or a set of widgets to create and spawn into the widget tree.
 * More complicated defaults could be created by deriving from this class and registering new templates with the module.
 */
class UMGEDITOR_API FWidgetTemplate : public TSharedFromThis<FWidgetTemplate>
{
public:
	/** Constructor */
	FWidgetTemplate();

	/** The category this template fits into. */
	virtual FText GetCategory() const = 0;

	/** Constructs the widget template. */
	virtual UWidget* Create(class UWidgetTree* Tree) = 0;

	/** Gets the icon to display in the template palate for this template. */
	virtual const FSlateBrush* GetIcon() const
	{
		static FSlateNoResource NullBrush;
		return &NullBrush;
	}

	/** Gets tooltip widget for this palette item. */
	virtual TSharedRef<IToolTip> GetToolTip() const = 0;

public:
	/** The name of the widget template. */
	FText Name;
};
