// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UOverlay

UOverlay::UOverlay(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

UClass* UOverlay::GetSlotClass() const
{
	return UOverlaySlot::StaticClass();
}

void UOverlay::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyOverlay.IsValid() )
	{
		Cast<UOverlaySlot>(Slot)->BuildSlot(MyOverlay.ToSharedRef());
	}
}

void UOverlay::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyOverlay.IsValid() )
	{
		MyOverlay->RemoveSlot(Slot->Content->GetWidget());
	}
}

TSharedRef<SWidget> UOverlay::RebuildWidget()
{
	MyOverlay = SNew(SOverlay);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UOverlaySlot* TypedSlot = Cast<UOverlaySlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyOverlay.ToSharedRef());
		}
	}

	return BuildDesignTimeWidget( MyOverlay.ToSharedRef() );
}
