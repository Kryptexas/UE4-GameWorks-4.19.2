// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UUniformGridPanel

UUniformGridPanel::UUniformGridPanel(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UUniformGridPanel::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UUniformGridPanel::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

bool UUniformGridPanel::AddChild(UWidget* Child, FVector2D Position)
{
	AddSlot(Child);
	return true;
}

bool UUniformGridPanel::RemoveChild(UWidget* Child)
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UUniformGridSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Child )
		{
			Slots.RemoveAt(SlotIndex);
			return true;
		}
	}

	return false;
}

void UUniformGridPanel::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UUniformGridSlot* Slot = Slots[Index];
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
}

TSharedRef<SWidget> UUniformGridPanel::RebuildWidget()
{
	TSharedRef<SUniformGridPanel> NewPanel =
		SNew(SUniformGridPanel)
		.SlotPadding(SlotPadding)
		.MinDesiredSlotWidth(MinDesiredSlotWidth)
		.MinDesiredSlotHeight(MinDesiredSlotHeight);
	
	MyUniformGridPanel = NewPanel;

	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UUniformGridSlot* Slot = Slots[SlotIndex];
		if ( Slot == NULL )
		{
			Slots[SlotIndex] = Slot = ConstructObject<UUniformGridSlot>(UUniformGridSlot::StaticClass(), this);
		}

		Slot->BuildSlot(NewPanel);
	}

	return NewPanel;
}

UUniformGridSlot* UUniformGridPanel::AddSlot(UWidget* Content)
{
	UUniformGridSlot* Slot = ConstructObject<UUniformGridSlot>(UUniformGridSlot::StaticClass(), this);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR

void UUniformGridPanel::ConnectEditorData()
{
	for ( UUniformGridSlot* Slot : Slots )
	{
		Slot->Content->Slot = Slot;
		Slot->Parent = this;
	}
}

void UUniformGridPanel::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if ( Slots[SlotIndex] == NULL )
		{
			Slots[SlotIndex] = ConstructObject<UUniformGridSlot>(UUniformGridSlot::StaticClass(), this);
		}
	}
}

#endif
