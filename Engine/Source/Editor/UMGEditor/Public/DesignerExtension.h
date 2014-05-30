// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class UWidget;
class SWidget;
class UWidgetBlueprint;

struct FSelectedWidget
{
public:
	FSelectedWidget()
		: Preview(NULL)
		, Template(NULL)
	{ }

	bool IsValid() const
	{
		return Template != NULL && Preview != NULL;
	}

	UWidget* Template;
	UWidget* Preview;
};

class FDesignerExtension : public TSharedFromThis<FDesignerExtension>
{
public:
	virtual void Initialize(UWidgetBlueprint* InBlueprint);

	virtual void BuildWidgetsForSelection(const TArray< FSelectedWidget >& Selection, TArray< TSharedRef<SWidget> >& Widgets) = 0;

	FName GetExtensionId() const;

protected:
	FName ExtensionId;
	UWidgetBlueprint* Blueprint;
};
