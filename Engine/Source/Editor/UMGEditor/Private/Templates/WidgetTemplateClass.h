// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetTemplate.h"

/**
 * A template that can spawn any widget derived from the UWidget class.
 */
class FWidgetTemplateClass : public FWidgetTemplate
{
public:
	/** Constructor */
	FWidgetTemplateClass(TSubclassOf<UWidget> InWidgetClass);

	/** Destructor */
	virtual ~FWidgetTemplateClass() {}

	/** Gets the category for the widget */
	virtual FText GetCategory() const override;

	/** Creates an instance of the widget for the tree. */
	virtual UWidget* Create(UWidgetTree* Tree) override;

	/** The icon coming from the default object of the class */
	virtual const FSlateBrush* GetIcon() const override;

	/** Gets the tooltip widget for this palette item. */
	virtual TSharedRef<IToolTip> GetToolTip() const override;

protected:
	/** The widget class that will be created by this template */
	TSubclassOf<UWidget> WidgetClass;

	/** The cached category for the widget. */
	mutable FText Category;
};
