// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UOverlay

UOverlay::UOverlay(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UOverlay::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UOverlay::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

bool UOverlay::AddChild(UWidget* Child, FVector2D Position)
{
	UOverlaySlot* Slot = AddSlot(Child);
	
	// Add the child to the live canvas if it already exists
	if (MyOverlay.IsValid())
	{
		Slot->BuildSlot(MyOverlay.ToSharedRef());
	}
	
	return true;
}

bool UOverlay::RemoveChild(UWidget* Child)
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UOverlaySlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Child )
		{
			Slots.RemoveAt(SlotIndex);
			
			// Remove the widget from the live slot if it exists.
			if (MyOverlay.IsValid())
			{
				MyOverlay->RemoveSlot(Child->GetWidget());
			}
			
			return true;
		}
	}

	return false;
}

void UOverlay::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UOverlaySlot* Slot = Slots[Index];
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
}

TSharedRef<SWidget> UOverlay::RebuildWidget()
{
	MyOverlay = SNew(SOverlay);

	for ( auto Slot : Slots )
	{
		Slot->Parent = this;
		Slot->BuildSlot(MyOverlay.ToSharedRef());
	}

	return BuildDesignTimeWidget( MyOverlay.ToSharedRef() );
}

UOverlaySlot* UOverlay::AddSlot(UWidget* Content)
{
	UOverlaySlot* Slot = ConstructObject<UOverlaySlot>(UOverlaySlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR

void UOverlay::ConnectEditorData()
{
	for ( UOverlaySlot* Slot : Slots )
	{
		if ( Slot->Content )
		{
			Slot->Content->Slot = Slot;
		}
		//TODO UMG Should we auto delete empty slots?
	}
}

#endif
