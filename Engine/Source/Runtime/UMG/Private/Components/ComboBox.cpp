// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UComboBox

UComboBox::UComboBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

TSharedRef<SWidget> UComboBox::RebuildWidget()
{
	TSharedRef< SComboBox<UObject*> > NewComboBox =
		SNew(SComboBox<UObject*>)
		.OptionsSource(&Items)
		.OnGenerateWidget(BIND_UOBJECT_DELEGATE(SComboBox<UObject*>::FOnGenerateWidget, HandleGenerateWidget));

	return NewComboBox;
}

TSharedRef<SWidget> UComboBox::HandleGenerateWidget(UObject* Item) const
{
	// Call the user's delegate to see if they want to generate a custom widget bound to the data source.
	if ( OnGenerateWidget.IsBound() )
	{
		UWidget* Widget = OnGenerateWidget.Execute(Item);
		if ( Widget != NULL )
		{
			return Widget->GetWidget();
		}
	}

	// If a row wasn't generated just create the default one, a simple text block of the item's name.
	return SNew(STextBlock).Text(Item ? FText::FromString(Item->GetName()) : LOCTEXT("null", "null"));
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
