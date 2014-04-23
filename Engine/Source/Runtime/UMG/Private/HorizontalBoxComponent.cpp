// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UHorizontalBoxComponent

UHorizontalBoxComponent::UHorizontalBoxComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

TSharedRef<SWidget> UHorizontalBoxComponent::RebuildWidget()
{
	TSharedRef<SHorizontalBox> NewCanvas = SNew(SHorizontalBox);
	MyHorizontalBox = NewCanvas;

	OnKnownChildrenChanged();

	return NewCanvas;
}

void UHorizontalBoxComponent::OnKnownChildrenChanged()
{
	TSharedPtr<SHorizontalBox> Canvas = MyHorizontalBox.Pin();
	if (Canvas.IsValid())
	{
		Canvas->ClearChildren();

		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			UHorizontalBoxSlot* Slot = Slots[SlotIndex];
			if ( Slot == NULL )
			{
				Slots[SlotIndex] = Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
			}

			auto NewSlot = Canvas->AddSlot()
				.AutoWidth()
				.HAlign(Slot->HorizontalAlignment)
				.VAlign(Slot->VerticalAlignment)
			[
				Slot->Content == NULL ? SNullWidget::NullWidget : Slot->Content->GetWidget()
			];
		}
	}
}

UHorizontalBoxSlot* UHorizontalBoxComponent::AddSlot(USlateWrapperComponent* Content)
{
	UHorizontalBoxSlot* Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
	Slot->Content = Content;
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR
void UHorizontalBoxComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if ( Slots[SlotIndex] == NULL )
		{
			Slots[SlotIndex] = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
		}
	}
}
#endif
