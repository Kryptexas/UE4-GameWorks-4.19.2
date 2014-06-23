// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UHorizontalBox

UHorizontalBox::UHorizontalBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

UClass* UHorizontalBox::GetSlotClass() const
{
	return UHorizontalBoxSlot::StaticClass();
}

void UHorizontalBox::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyHorizontalBox.IsValid() )
	{
		Cast<UHorizontalBoxSlot>(Slot)->BuildSlot(MyHorizontalBox.ToSharedRef());
	}
}

void UHorizontalBox::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyHorizontalBox.IsValid() )
	{
		MyHorizontalBox->RemoveSlot(Slot->Content->GetWidget());
	}
}

UHorizontalBoxSlot* UHorizontalBox::AddChild(UWidget* Content)
{
	return Cast<UHorizontalBoxSlot>( Super::AddChild(Content, FVector2D(0, 0)) );
}

TSharedRef<SWidget> UHorizontalBox::RebuildWidget()
{
	MyHorizontalBox = SNew(SHorizontalBox);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UHorizontalBoxSlot* TypedSlot = Cast<UHorizontalBoxSlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyHorizontalBox.ToSharedRef());
		}
	}

	return BuildDesignTimeWidget( MyHorizontalBox.ToSharedRef() );
}

#if WITH_EDITOR

const FSlateBrush* UHorizontalBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.HorizontalBox");
}

#endif
