// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UVerticalBoxComponent

UVerticalBoxComponent::UVerticalBoxComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UVerticalBoxComponent::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UVerticalBoxComponent::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

int32 UVerticalBoxComponent::GetChildIndex(UWidget* Content) const
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UVerticalBoxSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Content )
		{
			return SlotIndex;
		}
	}

	return -1;
}

bool UVerticalBoxComponent::AddChild(UWidget* Child, FVector2D Position)
{
	AddSlot(Child);
	return true;
}

bool UVerticalBoxComponent::RemoveChild(UWidget* Child)
{
	int32 SlotIndex = GetChildIndex(Child);
	if ( SlotIndex != -1 )
	{
		Slots.RemoveAt(SlotIndex);
		return true;
	}

	return false;
}

void UVerticalBoxComponent::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UVerticalBoxSlot* Slot = Slots[Index];
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
}

void UVerticalBoxComponent::InsertChildAt(int32 Index, UWidget* Content)
{
	UVerticalBoxSlot* Slot = ConstructObject<UVerticalBoxSlot>(UVerticalBoxSlot::StaticClass(), this);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif

	Slots.Insert(Slot, Index);
}

TSharedRef<SWidget> UVerticalBoxComponent::RebuildWidget()
{
	TSharedRef<SVerticalBox> NewCanvas = SNew(SVerticalBox);
	MyVerticalBox = NewCanvas;

	TSharedPtr<SVerticalBox> Canvas = MyVerticalBox.Pin();
	if ( Canvas.IsValid() )
	{
		Canvas->ClearChildren();

		for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
		{
			UVerticalBoxSlot* Slot = Slots[SlotIndex];
			if ( Slot == NULL )
			{
				Slots[SlotIndex] = Slot = ConstructObject<UVerticalBoxSlot>(UVerticalBoxSlot::StaticClass(), this);
			}

			Slot->Parent = this;
			Slot->BuildSlot(NewCanvas);
		}
	}

	return NewCanvas;
}

UVerticalBoxSlot* UVerticalBoxComponent::AddSlot(UWidget* Content)
{
	UVerticalBoxSlot* Slot = ConstructObject<UVerticalBoxSlot>(UVerticalBoxSlot::StaticClass(), this);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR
void UVerticalBoxComponent::ConnectEditorData()
{
	for ( UVerticalBoxSlot* Slot : Slots )
	{
		// TODO UMG Find a better way to assign the parent pointers for slots.
		Slot->Parent = this;

		if ( Slot->Content )
		{
			Slot->Content->Slot = Slot;
		}
		//TODO UMG Should we auto delete empty slots?
	}
}

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
