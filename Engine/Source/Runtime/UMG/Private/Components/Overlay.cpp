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
	AddSlot(Child);
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
	TSharedRef<SOverlay> NewCanvas = SNew(SOverlay);
	MyVerticalBox = NewCanvas;

	TSharedPtr<SOverlay> Canvas = MyVerticalBox.Pin();
	if ( Canvas.IsValid() )
	{
		Canvas->ClearChildren();

		for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
		{
			UOverlaySlot* Slot = Slots[SlotIndex];
			if ( Slot == NULL )
			{
				Slots[SlotIndex] = Slot = ConstructObject<UOverlaySlot>(UOverlaySlot::StaticClass(), this);
			}

			auto& NewSlot = Canvas->AddSlot()
				.Padding(Slot->Padding)
				.HAlign(Slot->HorizontalAlignment)
				.VAlign(Slot->VerticalAlignment)
				[
					Slot->Content == NULL ? SNullWidget::NullWidget : Slot->Content->GetWidget()
				];
		}
	}

	return NewCanvas;
}

UOverlaySlot* UOverlay::AddSlot(UWidget* Content)
{
	UOverlaySlot* Slot = ConstructObject<UOverlaySlot>(UOverlaySlot::StaticClass(), this);
	Slot->Content = Content;

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

void UOverlay::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if ( Slots[SlotIndex] == NULL )
		{
			Slots[SlotIndex] = ConstructObject<UOverlaySlot>(UOverlaySlot::StaticClass(), this);
		}
	}
}
#endif
