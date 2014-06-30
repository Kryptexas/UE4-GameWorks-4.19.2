// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UVerticalBox

UVerticalBox::UVerticalBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	SVerticalBox::FArguments Defaults;
	Visiblity = UWidget::ConvertRuntimeToSerializedVisiblity( Defaults._Visibility.Get() );
}

UClass* UVerticalBox::GetSlotClass() const
{
	return UVerticalBoxSlot::StaticClass();
}

void UVerticalBox::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyVerticalBox.IsValid() )
	{
		Cast<UVerticalBoxSlot>(Slot)->BuildSlot(MyVerticalBox.ToSharedRef());
	}
}

void UVerticalBox::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyVerticalBox.IsValid() )
	{
		MyVerticalBox->RemoveSlot(Slot->Content->GetWidget());
	}
}

UVerticalBoxSlot* UVerticalBox::Add(UWidget* Content)
{
	return Cast<UVerticalBoxSlot>( Super::AddChild(Content) );
}

TSharedRef<SWidget> UVerticalBox::RebuildWidget()
{
	MyVerticalBox = SNew(SVerticalBox);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UVerticalBoxSlot* TypedSlot = Cast<UVerticalBoxSlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyVerticalBox.ToSharedRef());
		}
	}

	return BuildDesignTimeWidget( MyVerticalBox.ToSharedRef() );
}

#if WITH_EDITOR

const FSlateBrush* UVerticalBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.VerticalBox");
}

#endif
