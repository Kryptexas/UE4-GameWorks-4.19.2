// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetReference.h"

class UWidget;
class SWidget;
class UWidgetBlueprint;

/**
 * The Designer extension allows developers to provide additional widgets and custom painting to the designer surface for
 * specific widgets.  Allowing for a more customized and specific editors for the different widgets.
 */
class FDesignerExtension : public TSharedFromThis<FDesignerExtension>
{
public:
	/** Constructor */
	FDesignerExtension();

	/** Initializes the designer extension, this is called the first time a designer extension is registered */
	virtual void Initialize(UWidgetBlueprint* InBlueprint);

	/** Called every time the selection in the designer changes. */
	virtual void BuildWidgetsForSelection(const TArray< FWidgetReference >& Selection, TArray< TSharedRef<SWidget> >& Widgets) = 0;

	/** Gets the ID identifying this extension. */
	FName GetExtensionId() const;

protected:
	void BeginTransaction(const FText& SessionName);

	void EndTransaction();

protected:
	FName ExtensionId;
	UWidgetBlueprint* Blueprint;

	TArray< FWidgetReference > SelectionCache;

private:
	FScopedTransaction* ScopedTransaction;
};
