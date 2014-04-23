// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UVerticalBoxComponent

UVerticalBoxComponent::UVerticalBoxComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

int32 UVerticalBoxComponent::GetChildrenCount() const
{
	return Slots.Num();
}

USlateWrapperComponent* UVerticalBoxComponent::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

TSharedRef<SWidget> UVerticalBoxComponent::RebuildWidget()
{
	TSharedRef<SVerticalBox> NewCanvas = SNew(SVerticalBox);
	MyVerticalBox = NewCanvas;

	OnKnownChildrenChanged();

	return NewCanvas;
}

void UVerticalBoxComponent::OnKnownChildrenChanged()
{
	TSharedPtr<SVerticalBox> Canvas = MyVerticalBox.Pin();
	if (Canvas.IsValid())
	{
		Canvas->ClearChildren();

		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			UVerticalBoxSlot* Slot = Slots[SlotIndex];
			if ( Slot == NULL )
			{
				Slots[SlotIndex] = Slot = ConstructObject<UVerticalBoxSlot>(UVerticalBoxSlot::StaticClass(), this);
			}

			auto NewSlot = Canvas->AddSlot()
				.AutoHeight()
				.HAlign(Slot->HorizontalAlignment)
				.VAlign(Slot->VerticalAlignment)
			[
				Slot->Content == NULL ? SNullWidget::NullWidget : Slot->Content->GetWidget()
			];
		}
	}
}

UVerticalBoxSlot* UVerticalBoxComponent::AddSlot(USlateWrapperComponent* Content)
{
	UVerticalBoxSlot* Slot = ConstructObject<UVerticalBoxSlot>(UVerticalBoxSlot::StaticClass(), this);
	Slot->Content = Content;
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR
void UVerticalBoxComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if ( Slots[SlotIndex] == NULL )
		{
			Slots[SlotIndex] = ConstructObject<UVerticalBoxSlot>(UVerticalBoxSlot::StaticClass(), this);
		}
	}
}
#endif