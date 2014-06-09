// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UScrollBox

UScrollBox::UScrollBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UScrollBox::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UScrollBox::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

int32 UScrollBox::GetChildIndex(UWidget* Content) const
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UScrollBoxSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Content )
		{
			return SlotIndex;
		}
	}

	return -1;
}

bool UScrollBox::AddChild(UWidget* Child, FVector2D Position)
{
	AddSlot(Child);
	return true;
}

bool UScrollBox::RemoveChild(UWidget* Child)
{
	int32 SlotIndex = GetChildIndex(Child);
	if ( SlotIndex != -1 )
	{
		Slots.RemoveAt(SlotIndex);
		return true;
	}

	return false;
}

void UScrollBox::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UScrollBoxSlot* Slot = Slots[Index];
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
}

void UScrollBox::InsertChildAt(int32 Index, UWidget* Content)
{
	UScrollBoxSlot* Slot = ConstructObject<UScrollBoxSlot>(UScrollBoxSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif

	Slots.Insert(Slot, Index);
}

TSharedRef<SWidget> UScrollBox::RebuildWidget()
{
	TSharedRef<SScrollBox> NewCanvas = SNew(SScrollBox);
	MyScrollBox = NewCanvas;

	TSharedPtr<SScrollBox> Canvas = MyScrollBox.Pin();
	if ( Canvas.IsValid() )
	{
		Canvas->ClearChildren();

		for ( auto Slot : Slots )
		{
			Slot->Parent = this;
			Slot->BuildSlot(NewCanvas);
		}
	}

	return NewCanvas;
}

UScrollBoxSlot* UScrollBox::AddSlot(UWidget* Content)
{
	UScrollBoxSlot* Slot = ConstructObject<UScrollBoxSlot>(UScrollBoxSlot::StaticClass(), this);
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
void UScrollBox::ConnectEditorData()
{
	for ( UScrollBoxSlot* Slot : Slots )
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

void UScrollBox::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
}
#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
