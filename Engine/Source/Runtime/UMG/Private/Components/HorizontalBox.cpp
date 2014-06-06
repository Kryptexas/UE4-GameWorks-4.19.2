// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UHorizontalBox

UHorizontalBox::UHorizontalBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UHorizontalBox::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UHorizontalBox::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

int32 UHorizontalBox::GetChildIndex(UWidget* Content) const
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UHorizontalBoxSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Content )
		{
			return SlotIndex;
		}
	}

	return -1;
}

bool UHorizontalBox::AddChild(UWidget* Child, FVector2D Position)
{
	AddSlot(Child);
	return true;
}

bool UHorizontalBox::RemoveChild(UWidget* Child)
{
	int32 SlotIndex = GetChildIndex(Child);
	if ( SlotIndex != -1 )
	{
		Slots.RemoveAt(SlotIndex);
		return true;
	}

	return false;
}

void UHorizontalBox::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UHorizontalBoxSlot* Slot = Slots[Index];
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
}

void UHorizontalBox::InsertChildAt(int32 Index, UWidget* Content)
{
	UHorizontalBoxSlot* Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif

	Slots.Insert(Slot, Index);
}

TSharedRef<SWidget> UHorizontalBox::RebuildWidget()
{
	TSharedRef<SHorizontalBox> NewWidget = SNew(SHorizontalBox);
	MyHorizontalBox = NewWidget;

	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UHorizontalBoxSlot* Slot = Slots[SlotIndex];
		if ( Slot == NULL )
		{
			Slots[SlotIndex] = Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
		}

		Slot->Parent = this;
		Slot->BuildSlot(NewWidget);
	}

	return NewWidget;
}

UHorizontalBoxSlot* UHorizontalBox::AddSlot(UWidget* Content)
{
	UHorizontalBoxSlot* Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR
void UHorizontalBox::ConnectEditorData()
{
	for ( UHorizontalBoxSlot* Slot : Slots )
	{
		Slot->Content->Slot = Slot;
		Slot->Parent = this;
	}
}

void UHorizontalBox::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
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
