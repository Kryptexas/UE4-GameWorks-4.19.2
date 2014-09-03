// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UComboBox

UComboBox::UComboBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UComboBox::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyComboBox.Reset();
}

TSharedRef<SWidget> UComboBox::RebuildWidget()
{
	TSet<UObject*> UniqueItems(Items);
	Items = UniqueItems.Array();

	MyComboBox =
		SNew(SComboBox<UObject*>)
		.OptionsSource(&Items)
		.OnGenerateWidget(BIND_UOBJECT_DELEGATE(SComboBox<UObject*>::FOnGenerateWidget, HandleGenerateWidget));

	return MyComboBox.ToSharedRef();
}

TSharedRef<SWidget> UComboBox::HandleGenerateWidget(UObject* Item) const
{
	// Call the user's delegate to see if they want to generate a custom widget bound to the data source.
	if ( OnGenerateWidgetEvent.IsBound() )
	{
		UWidget* Widget = OnGenerateWidgetEvent.Execute(Item);
		if ( Widget != NULL )
		{
			return Widget->TakeWidget();
		}
	}

	// If a row wasn't generated just create the default one, a simple text block of the item's name.
	return SNew(STextBlock).Text(Item ? FText::FromString(Item->GetName()) : LOCTEXT("null", "null"));
}

#if WITH_EDITOR

const FSlateBrush* UComboBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ComboBox");
}

const FText UComboBox::GetToolboxCategory()
{
	return LOCTEXT("Misc", "Misc");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
